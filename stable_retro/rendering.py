import sys

try:
    import pyglet
except ImportError as e:
    raise ImportError(
        """
    Cannot import pyglet.
    HINT: you can install pyglet directly via 'pip install pyglet'.
    But if you really just want to install all Gym dependencies and not have to think about it,
    'pip install -e .[all]' or 'pip install gym[all]' will do it.
    """,
    ) from e

try:
    from pyglet.gl import gl
except ImportError as e:
    raise ImportError(
        """
    Error occurred while running `from pyglet.gl import *`
    HINT: make sure you have OpenGL installed. On Ubuntu, you can run 'apt-get install python-opengl'.
    If you're running on a server, you may need a virtual frame buffer; something like this should work:
    'xvfb-run -s \"-screen 0 1400x900x24\" python <your_script.py>'
    """,
    ) from e


def get_window(width, height, display, **kwargs):
    """
    Will create a pyglet window from the display specification provided.
    """
    screen = display.get_screens()  # available screens
    config = screen[0].get_best_config()  # selecting the first screen
    context = config.create_context(None)  # create GL context

    return pyglet.window.Window(
        width=width,
        height=height,
        display=display,
        config=config,
        context=context,
        **kwargs,
    )


def get_display(spec):
    """Convert a display specification (such as :0) into an actual Display
    object.
    Pyglet only supports multiple Displays on Linux.
    """
    if spec is None:
        return pyglet.canvas.get_display()
        # returns already available pyglet_display,
        # if there is no pyglet display available then it creates one
    elif isinstance(spec, str):
        return pyglet.canvas.Display(spec)
    else:
        raise ValueError(
            "Invalid display specification: {}. (Must be a string like :0 or None.)".format(
                spec,
            ),
        )


class SimpleImageViewer:
    def __init__(self, display=None, maxwidth=500):
        self.window = None
        self.isopen = False
        self.display = get_display(display)
        self.maxwidth = maxwidth
        self.width = 0
        self.height = 0
        self._rotation = 0

    def imshow(self, arr, rotation=0):
        rotation_steps = int(rotation) % 4
        height, width, _channels = arr.shape
        display_width = width
        display_height = height
        if rotation_steps % 2:
            display_width, display_height = display_height, display_width
        if display_width > self.maxwidth:
            scale = self.maxwidth / display_width
            display_width = int(scale * display_width)
            display_height = int(scale * display_height)
        if self.window is None:
            self.window = get_window(
                width=display_width,
                height=display_height,
                display=self.display,
                vsync=False,
                resizable=True,
            )
            self.width = display_width
            self.height = display_height
            self.isopen = True

            @self.window.event
            def on_resize(width, height):
                self.width = width
                self.height = height

            @self.window.event
            def on_close():
                self.isopen = False

        self._rotation = rotation_steps

        assert len(arr.shape) == 3, "You passed in an image with the wrong number shape"
        image = pyglet.image.ImageData(
            arr.shape[1],
            arr.shape[0],
            "RGB",
            arr.tobytes(),
            pitch=arr.shape[1] * -3,
        )
        texture = image.get_texture()
        self.window.switch_to()
        self.window.dispatch_events()
        self.window.clear()
        gl.glViewport(0, 0, int(self.width), int(self.height))
        gl.glBindTexture(texture.target, texture.id)
        gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_NEAREST)
        gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_NEAREST)
        gl.glMatrixMode(gl.GL_TEXTURE)
        gl.glPushMatrix()
        gl.glLoadIdentity()
        if self._rotation:
            gl.glTranslatef(0.5, 0.5, 0.0)
            gl.glRotatef(-90.0 * self._rotation, 0.0, 0.0, 1.0)
            gl.glTranslatef(-0.5, -0.5, 0.0)
        gl.glMatrixMode(gl.GL_PROJECTION)
        gl.glPushMatrix()
        gl.glLoadIdentity()
        gl.glOrtho(0, self.width, self.height, 0, -1, 1)
        gl.glMatrixMode(gl.GL_MODELVIEW)
        gl.glPushMatrix()
        gl.glLoadIdentity()
        gl.glColor4f(1.0, 1.0, 1.0, 1.0)
        gl.glBegin(gl.GL_TRIANGLE_FAN)
        gl.glTexCoord2f(0.0, 0.0)
        gl.glVertex2f(0.0, 0.0)
        gl.glTexCoord2f(1.0, 0.0)
        gl.glVertex2f(float(self.width), 0.0)
        gl.glTexCoord2f(1.0, 1.0)
        gl.glVertex2f(float(self.width), float(self.height))
        gl.glTexCoord2f(0.0, 1.0)
        gl.glVertex2f(0.0, float(self.height))
        gl.glEnd()
        gl.glBindTexture(texture.target, 0)
        gl.glMatrixMode(gl.GL_MODELVIEW)
        gl.glPopMatrix()
        gl.glMatrixMode(gl.GL_PROJECTION)
        gl.glPopMatrix()
        gl.glMatrixMode(gl.GL_TEXTURE)
        gl.glPopMatrix()
        gl.glMatrixMode(gl.GL_MODELVIEW)
        self.window.flip()

    def close(self):
        if self.isopen and sys.meta_path:
            # ^^^ check sys.meta_path to avoid 'ImportError: sys.meta_path is None, Python is likely shutting down'
            self.window.close()
            self.isopen = False

    def __del__(self):
        self.close()
