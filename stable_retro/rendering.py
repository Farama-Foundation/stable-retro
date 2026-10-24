import sys

import pyglet
from pyglet import gl
from pyglet.math import Mat4


def get_window(width, height, display, **kwargs):
    """Create a window on the first screen of the specified display."""
    window = pyglet.window.Window(
        width=width,
        height=height,
        display=display,
        screen=display.get_screens()[0],
        **kwargs,
    )
    if sys.platform == "darwin":
        # Our callers pump events themselves rather than calling app.run().
        # Complete Cocoa startup so windows receive native input and focus.
        pyglet.app.platform_event_loop.start()
    return window


def get_display(spec):
    """Resolve a display specification. Multiple displays are Linux-only."""
    if spec is None:
        return pyglet.display.get_display()
    if isinstance(spec, str):
        return pyglet.display.Display(spec)
    raise ValueError(
        f"Invalid display specification: {spec}. (Must be a string like :0 or None.)",
    )


class ImageRenderer:
    """Draw RGB observations using pyglet's shader-based sprite renderer."""

    def __init__(self, window):
        self.window = window
        self._texture = None
        self._sprite = None
        self._rotation = None
        self._framebuffer_size = window.get_framebuffer_size()

    def draw(self, arr, rotation=0):
        if arr.ndim != 3 or arr.shape[2] != 3:
            raise ValueError("Expected an RGB image with shape (height, width, 3)")
        height, width, _ = arr.shape
        rotation_steps = int(rotation) % 4
        self.window.switch_to()
        framebuffer_size = self.window.get_framebuffer_size()
        if (
            sys.platform.startswith("linux")
            and framebuffer_size != self._framebuffer_size
        ):
            # GLX/Mesa reallocates a resized drawable lazily at buffer swap.
            # Do that before drawing, or the first frame uses the old storage.
            self.window.flip()
        self._framebuffer_size = framebuffer_size
        if self._texture is None or (width, height) != (
            self._texture.width,
            self._texture.height,
        ):
            self.close()
            self._texture = pyglet.image.Texture.create(
                width,
                height,
                min_filter=gl.GL_NEAREST,
                mag_filter=gl.GL_NEAREST,
            )
        image = pyglet.image.ImageData(
            width,
            height,
            "RGB",
            arr.tobytes(),
            pitch=-width * 3,
        )
        self._texture.blit_into(image, 0, 0, 0)
        if self._sprite is None or self._rotation != rotation_steps:
            if self._sprite is not None:
                self._sprite.delete()
            texture = self._texture.get_transform(rotate=90 * rotation_steps)
            # get_transform rotates the anchor too; position the result at (0, 0).
            texture.anchor_x = texture.anchor_y = 0
            self._sprite = pyglet.sprite.Sprite(texture, subpixel=True)
            self._rotation = rotation_steps

        window_width, window_height = self.window.get_size()
        if window_width <= 0 or window_height <= 0:
            return
        # The drawable can be larger than the window's coordinates on Retina.
        framebuffer_width, framebuffer_height = self.window.get_framebuffer_size()
        gl.glViewport(0, 0, framebuffer_width, framebuffer_height)
        self.window.projection = Mat4.orthogonal_projection(
            0,
            window_width,
            0,
            window_height,
            -1,
            1,
        )
        self._sprite.scale_x = window_width / self._sprite.image.width
        self._sprite.scale_y = window_height / self._sprite.image.height
        self.window.clear()
        self._sprite.draw()

    def close(self):
        """Release GPU resources while their window context is still alive."""
        self.window.switch_to()
        if self._sprite is not None:
            self._sprite.delete()
            self._sprite = None
        if self._texture is not None:
            self._texture.delete()
            self._texture = None
        self._rotation = None


class SimpleImageViewer:
    def __init__(self, display=None, maxwidth=500):
        self.window = None
        self._renderer = None
        self.isopen = False
        self.display = get_display(display)
        self.maxwidth = maxwidth
        self.width = 0
        self.height = 0

    def imshow(self, arr, rotation=0):
        rotation_steps = int(rotation) % 4
        height, width, _channels = arr.shape
        if rotation_steps % 2:
            width, height = height, width
        if width > self.maxwidth:
            scale = self.maxwidth / width
            width = int(scale * width)
            height = int(scale * height)
        if self.window is None:
            self.window = get_window(
                width=width,
                height=height,
                display=self.display,
                vsync=False,
                resizable=True,
            )
            self._renderer = ImageRenderer(self.window)
            self.isopen = True

            @self.window.event
            def on_close():
                self.close()
                return pyglet.event.EVENT_HANDLED

        self.window.dispatch_events()
        # Dispatching a close event destroys the context. Do not draw afterward.
        if not self.isopen:
            return
        self.width, self.height = self.window.get_size()
        self._renderer.draw(arr, rotation_steps)
        self.window.flip()

    def close(self):
        if self.window is not None and sys.meta_path:
            if self._renderer is not None:
                self._renderer.close()
                self._renderer = None
            self.window.close()
            self.window = None
        self.isopen = False

    def __del__(self):
        self.close()
