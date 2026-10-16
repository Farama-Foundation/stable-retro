"""Real OpenGL checks. Enable with STABLE_RETRO_TEST_RENDERING=1 and a display."""

import os

import numpy as np
import pytest

pytestmark = pytest.mark.skipif(
    os.environ.get("STABLE_RETRO_TEST_RENDERING") != "1",
    reason="requires an OpenGL display (STABLE_RETRO_TEST_RENDERING=1)",
)


@pytest.fixture
def window():
    import pyglet

    win = pyglet.window.Window(width=160, height=100, visible=False)
    yield win
    win.close()


@pytest.fixture
def frame():
    # Distinct corners detect vertical flips, quarter turns, and cropped images.
    image = np.empty((12, 20, 3), dtype=np.uint8)
    image[:6, :10] = (255, 0, 0)
    image[:6, 10:] = (0, 255, 0)
    image[6:, :10] = (0, 0, 255)
    image[6:, 10:] = (255, 255, 0)
    return image


def assert_corners(window, image):
    from pyglet import gl

    window.switch_to()
    width, height = window.get_framebuffer_size()
    for x, y in ((0.25, 0.25), (0.75, 0.25), (0.25, 0.75), (0.75, 0.75)):
        pixel = (gl.GLubyte * 3)()
        gl.glReadPixels(
            int(x * width),
            int((1 - y) * height),
            1,
            1,
            gl.GL_RGB,
            gl.GL_UNSIGNED_BYTE,
            pixel,
        )
        expected = image[int(y * image.shape[0]), int(x * image.shape[1])]
        np.testing.assert_array_equal(list(pixel), expected)


@pytest.mark.parametrize("rotation", [0, 1, 2, 3])
@pytest.mark.parametrize("size", [(160, 100), (90, 150)])
def test_frame_orientation_and_resize(window, frame, rotation, size):
    from stable_retro.rendering import ImageRenderer

    renderer = ImageRenderer(window)
    try:
        renderer.draw(frame)
        window.set_size(*size)
        window.dispatch_events()
        renderer.draw(frame, rotation)
        assert_corners(window, np.rot90(frame, -rotation))
    finally:
        renderer.close()


def test_frame_updates_and_resolution_changes(window, frame):
    from stable_retro.rendering import ImageRenderer

    renderer = ImageRenderer(window)
    try:
        renderer.draw(frame)
        texture = renderer._texture
        updated = np.full_like(frame, (0, 255, 0))
        renderer.draw(updated)
        assert renderer._texture is texture
        assert_corners(window, updated)
        resized = np.full((15, 9, 3), (0, 0, 255), dtype=np.uint8)
        renderer.draw(resized, rotation=1)
        assert_corners(window, resized)
    finally:
        renderer.close()


def test_frame_fills_a_scaled_framebuffer(window, frame, monkeypatch):
    from stable_retro.rendering import ImageRenderer

    # Emulate Retina's logical-to-physical size difference on any CI display.
    width, height = window.get_framebuffer_size()
    monkeypatch.setattr(window, "get_size", lambda: (width // 2, height // 2))
    renderer = ImageRenderer(window)
    try:
        renderer.draw(frame)
        assert_corners(window, frame)
    finally:
        renderer.close()


def test_frame_uses_its_own_window_context(window, frame):
    import pyglet

    from stable_retro.rendering import ImageRenderer

    renderer = ImageRenderer(window)
    other_window = pyglet.window.Window(width=50, height=50, visible=False)
    try:
        # Creating another window changes the active context before the upload.
        renderer.draw(frame)
        assert_corners(window, frame)
    finally:
        renderer.close()
        other_window.close()


def test_viewer_close_reopen_and_event_close(monkeypatch, frame):
    import pyglet

    from stable_retro import rendering

    create_window = rendering.get_window

    def hidden_window(**kwargs):
        return create_window(**kwargs, visible=False)

    monkeypatch.setattr(rendering, "get_window", hidden_window)
    viewer = rendering.SimpleImageViewer()
    try:
        viewer.imshow(frame)
        first_window = viewer.window
        viewer.close()
        assert first_window.context is None
        viewer.imshow(frame, rotation=1)
        assert viewer.isopen
        assert viewer.window is not first_window
        current_window = viewer.window

        def dispatch_close():
            # Match native event dispatch without needing a physical mouse click.
            current_window._allow_dispatch_event = True
            current_window.dispatch_event("on_close")

        monkeypatch.setattr(current_window, "dispatch_events", dispatch_close)
        viewer.imshow(frame)
        assert not viewer.isopen
        assert viewer.window is None
        assert current_window.context is None
        assert first_window not in pyglet.app.windows
    finally:
        viewer.close()


@pytest.fixture
def interactive(monkeypatch, frame):
    import pyglet

    from stable_retro.examples.interactive import Interactive

    create_window = pyglet.window.Window

    def hidden_window(**kwargs):
        return create_window(width=160, height=100, visible=False)

    monkeypatch.setattr(pyglet.window, "Window", hidden_window)

    class Env:
        actions = None
        close_count = 0

        def __init__(self):
            self.actions = []

        def reset(self):
            return frame, {}

        def step(self, action):
            self.actions.append(action)
            return frame, 0.0, False, False, {}

        def close(self):
            self.close_count += 1

    class Viewer(Interactive):
        def get_image(self, obs, env):
            return obs[0] if isinstance(obs, tuple) else obs

        def keys_to_act(self, keys):
            return keys or None

    viewer = Viewer(Env(), sync=False)
    yield viewer
    viewer._on_close()


def test_interactive_frame_and_held_keys(interactive, frame):
    from pyglet.window import key

    interactive._draw()
    assert_corners(interactive._win, frame)
    interactive._key_handler.on_key_press(key.Z, 0)
    interactive._update(0.02)
    assert interactive._env.actions
    assert all("Z" in action for action in interactive._env.actions)
    interactive._key_handler.on_key_release(key.Z, 0)
    interactive._update(0.02)
    assert interactive._env.actions[-1] is None


def test_interactive_sync_keys_after_focus_loss(interactive):
    from pyglet.window import key

    interactive._sync = True
    interactive._key_handler.on_key_press(key.Z, 0)
    interactive._update(0.02)
    assert len(interactive._env.actions) == 1
    interactive._update(0.02)
    assert len(interactive._env.actions) == 1
    interactive._key_handler.on_deactivate()
    interactive._update(0.02)
    interactive._key_handler.on_key_press(key.Z, 0)
    interactive._update(0.02)
    assert len(interactive._env.actions) == 2


def test_interactive_escape_closes_without_step_or_process_exit(interactive):
    from pyglet.window import key

    interactive._key_handler.on_key_press(key.ESCAPE, 0)
    interactive._update(0.02)
    assert interactive._closed
    assert interactive._env.close_count == 1
    assert interactive._env.actions == []
    assert interactive._win.context is None
    interactive._on_close()
    assert interactive._env.close_count == 1


def test_interactive_run_returns_on_window_close(interactive, monkeypatch):
    monkeypatch.setattr(interactive._win, "dispatch_events", interactive._on_close)
    interactive.run()
    assert interactive._closed
    assert interactive._env.close_count == 1


@pytest.mark.parametrize("game", ["Airstriker-Genesis-v0", "Tetris-GameBoy-v0"])
def test_human_environment_frames(game, monkeypatch):
    import stable_retro as retro
    from stable_retro import rendering

    try:
        retro.data.get_romfile_path(game)
    except FileNotFoundError:
        pytest.skip(f"{game} ROM is not installed")

    create_window = rendering.get_window

    def hidden_window(**kwargs):
        return create_window(**kwargs, visible=False)

    monkeypatch.setattr(rendering, "get_window", hidden_window)
    with retro.make(game, render_mode="human") as env:
        obs, _ = env.reset()
        for _ in range(60):
            obs, _, terminated, truncated, _ = env.step(env.action_space.sample())
            if terminated or truncated:
                obs, _ = env.reset()
        assert env.viewer.isopen
        window = env.viewer.window
        env.viewer._renderer.draw(obs)
        assert_corners(window, obs)
    assert window.context is None
