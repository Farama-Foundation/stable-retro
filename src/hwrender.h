#pragma once

#include "libretro.h"
#include <cstdint>
#include <vector>

#ifdef ENABLE_HW_RENDER

namespace Retro {

enum class GLBackend {
    NoBackend,
    EGL,
    GLX
};

/**
 * Hardware rendering context for GPU-accelerated cores.
 * Supports headless OpenGL rendering via EGL or GLX on Linux.
 */
class HWRenderContext {
public:
    HWRenderContext();
    ~HWRenderContext();

    // Non-copyable
    HWRenderContext(const HWRenderContext&) = delete;
    HWRenderContext& operator=(const HWRenderContext&) = delete;

    /**
     * Initialize the hardware rendering context.
     * @param cb The libretro hardware render callback from the core.
     * @return true if initialization succeeded.
     */
    bool init(const retro_hw_render_callback& cb);

    /**
     * Destroy the hardware rendering context and free resources.
     */
    void destroy();

    /**
     * Check if hardware rendering is currently enabled.
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * Update the framebuffer size.
     * @param width New width in pixels.
     * @param height New height in pixels.
     */
    void resize(unsigned width, unsigned height);

    /**
     * Get the current framebuffer object ID for the core to render to.
     * Returns 0 to use the default framebuffer (pbuffer surface).
     */
    uintptr_t getCurrentFramebuffer() const { return 0; }

    /**
     * Get the address of an OpenGL function.
     * @param sym The symbol name.
     * @return The function pointer, or nullptr if not found.
     */
    retro_proc_address_t getProcAddress(const char* sym) const;

    /**
     * Call the core's context_reset callback.
     * Must be called after init() returns and the HW render callbacks are wired up.
     */
    void contextReset();

    /**
     * Read pixels from the GPU framebuffer into CPU memory.
     * @param width Frame width.
     * @param height Frame height.
     * @return Pointer to the pixel data (RGBA8888 format), or nullptr on failure.
     */
    const void* readbackFramebuffer(unsigned width, unsigned height);

    /**
     * Get the pitch (bytes per row) of the readback buffer.
     */
    size_t getReadbackPitch() const { return m_width * 4; }

    /**
     * Get the libretro callback structure.
     */
    const retro_hw_render_callback& getCallback() const { return m_callback; }

    /**
     * Check if the frame needs vertical flipping.
     */
    bool needsFlip() const { return m_callback.bottom_left_origin; }

private:
    bool initEGL();
    bool initGLX();
    bool initFramebuffer(unsigned width, unsigned height);
    void destroyEGL();
    void destroyGLX();
    void destroyFramebuffer();
    void flipVertical(unsigned width, unsigned height);

    retro_hw_render_callback m_callback{};
    bool m_enabled = false;
    GLBackend m_backend = GLBackend::NoBackend;

    // EGL state
    void* m_eglDisplay = nullptr;
    void* m_eglContext = nullptr;
    void* m_eglSurface = nullptr;
    void* m_eglConfig = nullptr;
    bool m_useSurfaceless = false;

    // GLX state
    void* m_glxDisplay = nullptr;
    void* m_glxContext = nullptr;
    unsigned long m_glxPbuffer = 0;
    void* m_glxFBConfig = nullptr;

    // OpenGL state
    unsigned int m_fbo = 0;
    unsigned int m_colorTexture = 0;
    unsigned int m_depthRb = 0;
    unsigned m_width = 0;
    unsigned m_height = 0;

    // Readback buffer
    std::vector<uint8_t> m_readbackBuffer;
};

} // namespace Retro

#endif // ENABLE_HW_RENDER
