#include "hwrender.h"

#ifdef ENABLE_HW_RENDER

#include <cstdlib>
#include <cstring>
#include <iostream>

// EGL headers
#include <EGL/egl.h>
#include <EGL/eglext.h>

// GLX headers for fallback on virgl VMs
#include <X11/Xlib.h>
#include <GL/glx.h>

// EGL extension defines (if not in headers)
#ifndef EGL_PLATFORM_DEVICE_EXT
#define EGL_PLATFORM_DEVICE_EXT 0x313F
#endif
#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif

// OpenGL headers
#include <GL/gl.h>
#include <GL/glext.h>

// GL function pointers (loaded dynamically)
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
static PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
static PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
static PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
static PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;

namespace Retro {

HWRenderContext::HWRenderContext() {
    memset(&m_callback, 0, sizeof(m_callback));
}

HWRenderContext::~HWRenderContext() {
    destroy();
}

bool HWRenderContext::init(const retro_hw_render_callback& cb) {
    std::cerr << "HWRender: init() called, context_type=" << cb.context_type << std::endl;
    // Only support OpenGL contexts for now
    if (cb.context_type != RETRO_HW_CONTEXT_OPENGL &&
        cb.context_type != RETRO_HW_CONTEXT_OPENGL_CORE &&
        cb.context_type != RETRO_HW_CONTEXT_OPENGLES2 &&
        cb.context_type != RETRO_HW_CONTEXT_OPENGLES3 &&
        cb.context_type != RETRO_HW_CONTEXT_OPENGLES_VERSION) {
        std::cerr << "HWRender: Unsupported context type " << cb.context_type << std::endl;
        return false;
    }

    m_callback = cb;

    std::cerr << "HWRender: calling initEGL()..." << std::endl;
    if (!initEGL()) {
        std::cerr << "HWRender: EGL failed, trying GLX fallback..." << std::endl;
        if (!initGLX()) {
            std::cerr << "HWRender: Both EGL and GLX failed" << std::endl;
            return false;
        }
        std::cerr << "HWRender: initGLX() succeeded" << std::endl;
    } else {
        std::cerr << "HWRender: initEGL() succeeded" << std::endl;
    }

    // Load GL extension functions based on backend
    if (m_backend == GLBackend::EGL) {
        glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)eglGetProcAddress("glGenFramebuffers");
        glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)eglGetProcAddress("glBindFramebuffer");
        glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)eglGetProcAddress("glDeleteFramebuffers");
        glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)eglGetProcAddress("glFramebufferTexture2D");
        glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)eglGetProcAddress("glFramebufferRenderbuffer");
        glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)eglGetProcAddress("glCheckFramebufferStatus");
        glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)eglGetProcAddress("glGenRenderbuffers");
        glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)eglGetProcAddress("glBindRenderbuffer");
        glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)eglGetProcAddress("glRenderbufferStorage");
        glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)eglGetProcAddress("glDeleteRenderbuffers");
    } else {
        glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)glXGetProcAddressARB((const GLubyte*)"glGenFramebuffers");
        glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)glXGetProcAddressARB((const GLubyte*)"glBindFramebuffer");
        glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)glXGetProcAddressARB((const GLubyte*)"glDeleteFramebuffers");
        glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)glXGetProcAddressARB((const GLubyte*)"glFramebufferTexture2D");
        glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)glXGetProcAddressARB((const GLubyte*)"glFramebufferRenderbuffer");
        glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)glXGetProcAddressARB((const GLubyte*)"glCheckFramebufferStatus");
        glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)glXGetProcAddressARB((const GLubyte*)"glGenRenderbuffers");
        glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)glXGetProcAddressARB((const GLubyte*)"glBindRenderbuffer");
        glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)glXGetProcAddressARB((const GLubyte*)"glRenderbufferStorage");
        glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)glXGetProcAddressARB((const GLubyte*)"glDeleteRenderbuffers");
    }

    if (!glGenFramebuffers || !glBindFramebuffer) {
        std::cerr << "HWRender: Failed to load GL functions" << std::endl;
        if (m_backend == GLBackend::EGL) destroyEGL();
        else destroyGLX();
        return false;
    }

    // Create initial framebuffer (640x480 default, will be resized)
    if (!initFramebuffer(640, 480)) {
        std::cerr << "HWRender: Failed to create framebuffer" << std::endl;
        if (m_backend == GLBackend::EGL) destroyEGL();
        else destroyGLX();
        return false;
    }

    m_enabled = true;

    std::cerr << "HWRender: init() completed successfully" << std::endl;
    // NOTE: We do NOT call context_reset here. According to libretro spec,
    // context_reset should be called by the frontend AFTER RETRO_ENVIRONMENT_SET_HW_RENDER
    // returns true. The core will call get_current_framebuffer() from context_reset,
    // so the callbacks must be set up first.

    return true;
}

void HWRenderContext::contextReset() {
    std::cerr << "HWRender: contextReset() called, enabled=" << m_enabled << std::endl;
    if (!m_enabled) {
        return;
    }
    if (m_callback.context_reset) {
        std::cerr << "HWRender: calling core's context_reset callback..." << std::endl;
        m_callback.context_reset();
        std::cerr << "HWRender: context_reset callback returned" << std::endl;
    }
}

void HWRenderContext::destroy() {
    if (!m_enabled) {
        return;
    }

    // Call the core's context destroy callback
    if (m_callback.context_destroy) {
        m_callback.context_destroy();
    }

    destroyFramebuffer();
    if (m_backend == GLBackend::EGL) {
        destroyEGL();
    } else if (m_backend == GLBackend::GLX) {
        destroyGLX();
    }

    m_enabled = false;
    m_backend = GLBackend::NoBackend;
    memset(&m_callback, 0, sizeof(m_callback));
}

// EGL extension function types
typedef EGLDisplay (EGLAPIENTRY *PFNEGLGETPLATFORMDISPLAYEXTPROC)(EGLenum platform, void *native_display, const EGLint *attrib_list);
typedef EGLBoolean (EGLAPIENTRY *PFNEGLQUERYDEVICESEXTPROC)(EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices);

bool HWRenderContext::initEGL() {
    EGLint major, minor;

    const char* extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

    // Check if we have a display (GUI mode)
    const char* displayEnv = getenv("DISPLAY");
    bool hasDisplay = (displayEnv != nullptr && displayEnv[0] != '\0');

    // For GUI apps with a display, try X11 display FIRST
    // (EGL device platform has issues with virgl and other virtual GPUs)
    if (hasDisplay) {
        m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (m_eglDisplay != EGL_NO_DISPLAY) {
            if (eglInitialize((EGLDisplay)m_eglDisplay, &major, &minor)) {
                std::cerr << "HWRender: Using native X11 display" << std::endl;
                goto egl_initialized;
            }
            m_eglDisplay = nullptr;
        }
    }

    // For headless mode, try EGL device platform
    if (extensions && strstr(extensions, "EGL_EXT_device_base") &&
        strstr(extensions, "EGL_EXT_platform_device")) {

        auto eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC)
            eglGetProcAddress("eglQueryDevicesEXT");
        auto eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
            eglGetProcAddress("eglGetPlatformDisplayEXT");

        if (eglQueryDevicesEXT && eglGetPlatformDisplayEXT) {
            EGLDeviceEXT devices[8];
            EGLint numDevices = 0;

            if (eglQueryDevicesEXT(8, devices, &numDevices) && numDevices > 0) {
                std::cerr << "HWRender: Found " << numDevices << " EGL device(s)" << std::endl;

                // Try to get device query extension
                auto eglQueryDeviceStringEXT = (const char* (*)(EGLDeviceEXT, EGLint))
                    eglGetProcAddress("eglQueryDeviceStringEXT");

                // Try each device, preferring non-software devices
                for (EGLint i = 0; i < numDevices; ++i) {
                    // Query device info if available
                    if (eglQueryDeviceStringEXT) {
                        const char* devExts = eglQueryDeviceStringEXT(devices[i], EGL_EXTENSIONS);
                        std::cerr << "HWRender: Device " << i << " extensions: "
                                  << (devExts ? devExts : "none") << std::endl;
                    }

                    m_eglDisplay = eglGetPlatformDisplayEXT(
                        EGL_PLATFORM_DEVICE_EXT, devices[i], nullptr);

                    if (m_eglDisplay != EGL_NO_DISPLAY) {
                        if (eglInitialize((EGLDisplay)m_eglDisplay, &major, &minor)) {
                            // Check vendor to prefer hardware over software
                            const char* vendor = eglQueryString((EGLDisplay)m_eglDisplay, EGL_VENDOR);
                            std::cerr << "HWRender: Device " << i << " vendor: "
                                      << (vendor ? vendor : "unknown") << std::endl;

                            std::cerr << "HWRender: Trying EGL device platform (device " << i << ")" << std::endl;
                            m_useSurfaceless = true;  // Device platform uses surfaceless
                            goto egl_initialized;
                        }
                        m_eglDisplay = nullptr;
                    }
                }
            }
        }
    }

    // Fallback: try native X11 display
    if (hasDisplay) {
        m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (m_eglDisplay != EGL_NO_DISPLAY) {
            if (eglInitialize((EGLDisplay)m_eglDisplay, &major, &minor)) {
                std::cerr << "HWRender: Using native X11 display" << std::endl;
                goto egl_initialized;
            }
            m_eglDisplay = nullptr;
        }
    }

    // Fallback: try surfaceless platform (Mesa)
    if (extensions && strstr(extensions, "EGL_MESA_platform_surfaceless")) {
        auto eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
            eglGetProcAddress("eglGetPlatformDisplayEXT");

        if (eglGetPlatformDisplayEXT) {
            m_eglDisplay = eglGetPlatformDisplayEXT(
                EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);

            if (m_eglDisplay != EGL_NO_DISPLAY) {
                if (eglInitialize((EGLDisplay)m_eglDisplay, &major, &minor)) {
                    std::cerr << "HWRender: Using Mesa surfaceless platform" << std::endl;
                    m_useSurfaceless = true;
                    goto egl_initialized;
                }
                m_eglDisplay = nullptr;
            }
        }
    }

    // Final fallback: default display without checking DISPLAY
    if (!hasDisplay) {
        m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (m_eglDisplay != EGL_NO_DISPLAY) {
            if (eglInitialize((EGLDisplay)m_eglDisplay, &major, &minor)) {
                goto egl_initialized;
            }
        }
    }

    std::cerr << "HWRender: No EGL display available" << std::endl;
    return false;

egl_initialized:
    std::cerr << "HWRender: EGL " << major << "." << minor << " initialized, useSurfaceless=" << m_useSurfaceless << std::endl;

    // Print EGL vendor info for debugging
    const char* eglVendor = eglQueryString((EGLDisplay)m_eglDisplay, EGL_VENDOR);
    const char* eglVersion = eglQueryString((EGLDisplay)m_eglDisplay, EGL_VERSION);
    std::cerr << "HWRender: EGL Vendor: " << (eglVendor ? eglVendor : "unknown") << std::endl;
    std::cerr << "HWRender: EGL Version: " << (eglVersion ? eglVersion : "unknown") << std::endl;

    // Check for surfaceless context support
    const char* eglExtensions = eglQueryString((EGLDisplay)m_eglDisplay, EGL_EXTENSIONS);
    bool hasSurfacelessContext = eglExtensions && strstr(eglExtensions, "EGL_KHR_surfaceless_context");
    std::cerr << "HWRender: EGL_KHR_surfaceless_context supported: " << (hasSurfacelessContext ? "yes" : "no") << std::endl;

    // Try OpenGL ES FIRST - it has better EGL support on virtual GPUs like virgl
    // Then fall back to desktop OpenGL
    bool useOpenGLES = false;

    // Check what the core wants
    bool coreWantsGLES = (m_callback.context_type == RETRO_HW_CONTEXT_OPENGLES2 ||
                          m_callback.context_type == RETRO_HW_CONTEXT_OPENGLES3 ||
                          m_callback.context_type == RETRO_HW_CONTEXT_OPENGLES_VERSION);

    // Try OpenGL ES first (better compatibility with virgl and other virtual GPUs)
    if (eglBindAPI(EGL_OPENGL_ES_API)) {
        useOpenGLES = true;
        std::cerr << "HWRender: Using OpenGL ES API (better virgl compatibility)" << std::endl;
    } else if (!coreWantsGLES && eglBindAPI(EGL_OPENGL_API)) {
        useOpenGLES = false;
        std::cerr << "HWRender: Using desktop OpenGL API" << std::endl;
    } else {
        std::cerr << "HWRender: eglBindAPI failed for both OpenGL ES and OpenGL" << std::endl;
        eglTerminate((EGLDisplay)m_eglDisplay);
        return false;
    }
    std::cerr << "HWRender: Bound " << (useOpenGLES ? "OpenGL ES" : "OpenGL") << " API" << std::endl;

    // Choose config - try pbuffer first since surfaceless has driver issues
    EGLConfig config;
    EGLint numConfigs;
    EGLint renderableType = useOpenGLES ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_BIT;

    // Try pbuffer config first (more compatible)
    {
        EGLint configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, renderableType,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_STENCIL_SIZE, 8,
            EGL_NONE
        };
        if (eglChooseConfig((EGLDisplay)m_eglDisplay, configAttribs, &config, 1, &numConfigs) && numConfigs > 0) {
            m_useSurfaceless = false;  // We got a pbuffer config, so use pbuffer not surfaceless
            std::cerr << "HWRender: Using pbuffer mode" << std::endl;
        } else {
            // Fallback: try without pbuffer requirement (surfaceless mode)
            if (hasSurfacelessContext) {
                EGLint configAttribs2[] = {
                    EGL_RENDERABLE_TYPE, renderableType,
                    EGL_RED_SIZE, 8,
                    EGL_GREEN_SIZE, 8,
                    EGL_BLUE_SIZE, 8,
                    EGL_ALPHA_SIZE, 8,
                    EGL_DEPTH_SIZE, 24,
                    EGL_STENCIL_SIZE, 8,
                    EGL_NONE
                };
                if (eglChooseConfig((EGLDisplay)m_eglDisplay, configAttribs2, &config, 1, &numConfigs) && numConfigs > 0) {
                    m_useSurfaceless = true;
                    std::cerr << "HWRender: Using surfaceless context mode (fallback)" << std::endl;
                }
            }
        }
    }

    if (numConfigs == 0) {
        std::cerr << "HWRender: eglChooseConfig failed - no configs found" << std::endl;
        eglTerminate((EGLDisplay)m_eglDisplay);
        return false;
    }
    std::cerr << "HWRender: Got " << numConfigs << " config(s)" << std::endl;

    // Query config details for debugging
    EGLint configRenderableType = 0;
    EGLint configSurfaceType = 0;
    eglGetConfigAttrib((EGLDisplay)m_eglDisplay, config, EGL_RENDERABLE_TYPE, &configRenderableType);
    eglGetConfigAttrib((EGLDisplay)m_eglDisplay, config, EGL_SURFACE_TYPE, &configSurfaceType);
    std::cerr << "HWRender: Config RENDERABLE_TYPE=0x" << std::hex << configRenderableType
              << " (OpenGL=" << ((configRenderableType & EGL_OPENGL_BIT) ? "yes" : "no")
              << ", ES3=" << ((configRenderableType & EGL_OPENGL_ES3_BIT) ? "yes" : "no") << ")"
              << ", SURFACE_TYPE=0x" << configSurfaceType << std::dec << std::endl;

    // Create pbuffer surface (off-screen) - skip if using surfaceless
    if (!m_useSurfaceless) {
        EGLint pbufferAttribs[] = {
            EGL_WIDTH, 640,
            EGL_HEIGHT, 480,
            EGL_NONE
        };
        m_eglSurface = eglCreatePbufferSurface((EGLDisplay)m_eglDisplay, config, pbufferAttribs);
        if (m_eglSurface == EGL_NO_SURFACE) {
            std::cerr << "HWRender: eglCreatePbufferSurface failed (error: 0x" << std::hex << eglGetError() << std::dec << "), will try surfaceless" << std::endl;
            m_useSurfaceless = true;
        } else {
            std::cerr << "HWRender: Created pbuffer surface" << std::endl;
        }
    }

    // Store config for later use
    m_eglConfig = config;

    // Create OpenGL context
    EGLint contextAttribs[16];
    int attrIdx = 0;

    if (useOpenGLES) {
        // OpenGL ES context - use EGL_CONTEXT_CLIENT_VERSION for EGL 1.4 compatibility
        contextAttribs[attrIdx++] = EGL_CONTEXT_CLIENT_VERSION;
        contextAttribs[attrIdx++] = 3;  // OpenGL ES 3.x
    } else {
        // Desktop OpenGL context
        contextAttribs[attrIdx++] = EGL_CONTEXT_MAJOR_VERSION;
        contextAttribs[attrIdx++] = (EGLint)(m_callback.version_major ? m_callback.version_major : 3);
        contextAttribs[attrIdx++] = EGL_CONTEXT_MINOR_VERSION;
        contextAttribs[attrIdx++] = (EGLint)(m_callback.version_minor ? m_callback.version_minor : 3);
        contextAttribs[attrIdx++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
        contextAttribs[attrIdx++] = (m_callback.context_type == RETRO_HW_CONTEXT_OPENGL_CORE)
            ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT
            : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT;
    }
    contextAttribs[attrIdx] = EGL_NONE;

    std::cerr << "HWRender: Creating context (" << (useOpenGLES ? "OpenGL ES 3" :
              (std::string("OpenGL ") + std::to_string(contextAttribs[1]) + "." + std::to_string(contextAttribs[3])).c_str()) << ")" << std::endl;

    m_eglContext = eglCreateContext((EGLDisplay)m_eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (m_eglContext == EGL_NO_CONTEXT) {
        std::cerr << "HWRender: eglCreateContext failed (error: 0x" << std::hex << eglGetError() << std::dec << "), trying simpler context" << std::endl;

        // Try a simpler context without version/profile constraints
        EGLint simpleContextAttribs[] = { EGL_NONE };
        m_eglContext = eglCreateContext((EGLDisplay)m_eglDisplay, config, EGL_NO_CONTEXT, simpleContextAttribs);
        if (m_eglContext == EGL_NO_CONTEXT) {
            std::cerr << "HWRender: Simple eglCreateContext also failed (error: 0x" << std::hex << eglGetError() << std::dec << ")" << std::endl;
            if (m_eglSurface) {
                eglDestroySurface((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurface);
            }
            eglTerminate((EGLDisplay)m_eglDisplay);
            return false;
        }
    }
    std::cerr << "HWRender: Context created successfully" << std::endl;

    // Re-bind the API right before making current (in case Qt changed it)
    if (!eglBindAPI(useOpenGLES ? EGL_OPENGL_ES_API : EGL_OPENGL_API)) {
        std::cerr << "HWRender: Re-binding API failed" << std::endl;
    }
    // Release any existing context on this thread first
    eglMakeCurrent((EGLDisplay)m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    // Make context current
    EGLSurface surface = m_useSurfaceless ? EGL_NO_SURFACE : (EGLSurface)m_eglSurface;
    std::cerr << "HWRender: Making current with surface=" << (m_useSurfaceless ? "EGL_NO_SURFACE" : "pbuffer") << std::endl;
    if (!eglMakeCurrent((EGLDisplay)m_eglDisplay, surface, surface, (EGLContext)m_eglContext)) {
        EGLint error = eglGetError();
        std::cerr << "HWRender: eglMakeCurrent failed (error: 0x" << std::hex << error << std::dec << ")" << std::endl;

        // If surfaceless failed, try with a pbuffer as last resort
        if (m_useSurfaceless && !m_eglSurface) {
            std::cerr << "HWRender: Trying pbuffer fallback..." << std::endl;
            EGLint pbufferAttribs[] = {
                EGL_WIDTH, 640,
                EGL_HEIGHT, 480,
                EGL_NONE
            };
            m_eglSurface = eglCreatePbufferSurface((EGLDisplay)m_eglDisplay, (EGLConfig)m_eglConfig, pbufferAttribs);
            if (m_eglSurface != EGL_NO_SURFACE) {
                if (eglMakeCurrent((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurface, (EGLSurface)m_eglSurface, (EGLContext)m_eglContext)) {
                    std::cerr << "HWRender: Pbuffer fallback succeeded!" << std::endl;
                    m_useSurfaceless = false;
                    goto context_current;
                }
                std::cerr << "HWRender: Pbuffer fallback also failed (error: 0x" << std::hex << eglGetError() << std::dec << ")" << std::endl;
                eglDestroySurface((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurface);
                m_eglSurface = nullptr;
            }
        }

        eglDestroyContext((EGLDisplay)m_eglDisplay, (EGLContext)m_eglContext);
        if (m_eglSurface) {
            eglDestroySurface((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurface);
        }
        eglTerminate((EGLDisplay)m_eglDisplay);
        return false;
    }

context_current:
    std::cerr << "HWRender: Context made current successfully" << std::endl;
    m_backend = GLBackend::EGL;

    return true;
}

void HWRenderContext::destroyEGL() {
    if (m_eglDisplay) {
        eglMakeCurrent((EGLDisplay)m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_eglContext) {
            eglDestroyContext((EGLDisplay)m_eglDisplay, (EGLContext)m_eglContext);
            m_eglContext = nullptr;
        }
        if (m_eglSurface) {
            eglDestroySurface((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurface);
            m_eglSurface = nullptr;
        }
        eglTerminate((EGLDisplay)m_eglDisplay);
        m_eglDisplay = nullptr;
    }
}

bool HWRenderContext::initGLX() {
    // GLX fallback for VMs with broken EGL (like virgl)
    const char* displayEnv = getenv("DISPLAY");
    if (!displayEnv || displayEnv[0] == '\0') {
        std::cerr << "HWRender: GLX requires DISPLAY environment variable" << std::endl;
        return false;
    }

    Display* display = XOpenDisplay(displayEnv);
    if (!display) {
        std::cerr << "HWRender: Failed to open X11 display for GLX" << std::endl;
        return false;
    }
    m_glxDisplay = display;

    // Check GLX version
    int glxMajor, glxMinor;
    if (!glXQueryVersion(display, &glxMajor, &glxMinor)) {
        std::cerr << "HWRender: glXQueryVersion failed" << std::endl;
        XCloseDisplay(display);
        m_glxDisplay = nullptr;
        return false;
    }
    std::cerr << "HWRender: GLX version " << glxMajor << "." << glxMinor << std::endl;

    // Choose FB config for pbuffer
    int fbConfigAttribs[] = {
        GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        None
    };

    int numConfigs = 0;
    GLXFBConfig* fbConfigs = glXChooseFBConfig(display, DefaultScreen(display), fbConfigAttribs, &numConfigs);
    if (!fbConfigs || numConfigs == 0) {
        std::cerr << "HWRender: glXChooseFBConfig failed" << std::endl;
        XCloseDisplay(display);
        m_glxDisplay = nullptr;
        return false;
    }
    std::cerr << "HWRender: Got " << numConfigs << " GLX FB config(s)" << std::endl;

    GLXFBConfig fbConfig = fbConfigs[0];
    m_glxFBConfig = fbConfig;
    XFree(fbConfigs);

    // Create pbuffer
    int pbufferAttribs[] = {
        GLX_PBUFFER_WIDTH, 640,
        GLX_PBUFFER_HEIGHT, 480,
        GLX_PRESERVED_CONTENTS, True,
        None
    };
    GLXPbuffer pbuffer = glXCreatePbuffer(display, fbConfig, pbufferAttribs);
    if (!pbuffer) {
        std::cerr << "HWRender: glXCreatePbuffer failed" << std::endl;
        XCloseDisplay(display);
        m_glxDisplay = nullptr;
        return false;
    }
    m_glxPbuffer = pbuffer;
    std::cerr << "HWRender: Created GLX pbuffer" << std::endl;

    // Create OpenGL context
    // Try GLX_ARB_create_context for modern OpenGL
    typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
    auto glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
        glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");

    GLXContext context = nullptr;
    if (glXCreateContextAttribsARB) {
        int contextAttribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, (int)(m_callback.version_major ? m_callback.version_major : 3),
            GLX_CONTEXT_MINOR_VERSION_ARB, (int)(m_callback.version_minor ? m_callback.version_minor : 3),
            GLX_CONTEXT_PROFILE_MASK_ARB,
                (m_callback.context_type == RETRO_HW_CONTEXT_OPENGL_CORE)
                    ? GLX_CONTEXT_CORE_PROFILE_BIT_ARB
                    : GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
            None
        };
        context = glXCreateContextAttribsARB(display, fbConfig, nullptr, True, contextAttribs);
    }

    if (!context) {
        // Fallback to legacy context creation
        std::cerr << "HWRender: glXCreateContextAttribsARB failed, trying legacy" << std::endl;
        XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbConfig);
        if (vi) {
            context = glXCreateContext(display, vi, nullptr, True);
            XFree(vi);
        }
    }

    if (!context) {
        std::cerr << "HWRender: Failed to create GLX context" << std::endl;
        glXDestroyPbuffer(display, pbuffer);
        XCloseDisplay(display);
        m_glxDisplay = nullptr;
        m_glxPbuffer = 0;
        return false;
    }
    m_glxContext = context;
    std::cerr << "HWRender: Created GLX context" << std::endl;

    // Make context current
    if (!glXMakeContextCurrent(display, pbuffer, pbuffer, context)) {
        std::cerr << "HWRender: glXMakeContextCurrent failed" << std::endl;
        glXDestroyContext(display, context);
        glXDestroyPbuffer(display, pbuffer);
        XCloseDisplay(display);
        m_glxDisplay = nullptr;
        m_glxContext = nullptr;
        m_glxPbuffer = 0;
        return false;
    }
    std::cerr << "HWRender: GLX context made current successfully" << std::endl;

    // Print GL info
    const char* glVendor = (const char*)glGetString(GL_VENDOR);
    const char* glRenderer = (const char*)glGetString(GL_RENDERER);
    const char* glVersion = (const char*)glGetString(GL_VERSION);
    std::cerr << "HWRender: GL Vendor: " << (glVendor ? glVendor : "unknown") << std::endl;
    std::cerr << "HWRender: GL Renderer: " << (glRenderer ? glRenderer : "unknown") << std::endl;
    std::cerr << "HWRender: GL Version: " << (glVersion ? glVersion : "unknown") << std::endl;

    m_backend = GLBackend::GLX;
    return true;
}

void HWRenderContext::destroyGLX() {
    if (m_glxDisplay) {
        Display* display = (Display*)m_glxDisplay;
        glXMakeContextCurrent(display, None, None, nullptr);
        if (m_glxContext) {
            glXDestroyContext(display, (GLXContext)m_glxContext);
            m_glxContext = nullptr;
        }
        if (m_glxPbuffer) {
            glXDestroyPbuffer(display, m_glxPbuffer);
            m_glxPbuffer = 0;
        }
        XCloseDisplay(display);
        m_glxDisplay = nullptr;
    }
}

bool HWRenderContext::initFramebuffer(unsigned width, unsigned height) {
    m_width = width;
    m_height = height;

    // Create color texture
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create depth/stencil renderbuffer if requested
    if (m_callback.depth || m_callback.stencil) {
        glGenRenderbuffers(1, &m_depthRb);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRb);
        if (m_callback.stencil) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        }
    }

    // Create framebuffer
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

    if (m_depthRb) {
        if (m_callback.stencil) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRb);
        } else {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRb);
        }
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "HWRender: Framebuffer incomplete (status: 0x" << std::hex << status << std::dec << ")" << std::endl;
        destroyFramebuffer();
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_readbackBuffer.resize(width * height * 4);

    return true;
}

void HWRenderContext::destroyFramebuffer() {
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_colorTexture) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_depthRb) {
        glDeleteRenderbuffers(1, &m_depthRb);
        m_depthRb = 0;
    }
    m_readbackBuffer.clear();
}

void HWRenderContext::resize(unsigned width, unsigned height) {
    if (width == m_width && height == m_height) {
        return;
    }

    destroyFramebuffer();
    initFramebuffer(width, height);
}

retro_proc_address_t HWRenderContext::getProcAddress(const char* sym) const {
    if (m_backend == GLBackend::GLX) {
        return (retro_proc_address_t)glXGetProcAddressARB((const GLubyte*)sym);
    }
    return (retro_proc_address_t)eglGetProcAddress(sym);
}

const void* HWRenderContext::readbackFramebuffer(unsigned width, unsigned height) {
    if (!m_enabled) {
        return nullptr;
    }

    // Resize readback buffer if needed
    if (width != m_width || height != m_height) {
        m_width = width;
        m_height = height;
        m_readbackBuffer.resize(width * height * 4);
    }

    // Read from FBO 0 (the default framebuffer / pbuffer)
    // The core renders to FBO 0, so we read directly from there
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, m_readbackBuffer.data());

    // Flip vertically if needed (OpenGL has bottom-left origin)
    if (m_callback.bottom_left_origin) {
        flipVertical(width, height);
    }

    return m_readbackBuffer.data();
}

void HWRenderContext::flipVertical(unsigned width, unsigned height) {
    size_t rowSize = width * 4;
    std::vector<uint8_t> tempRow(rowSize);

    for (unsigned y = 0; y < height / 2; ++y) {
        uint8_t* topRow = m_readbackBuffer.data() + y * rowSize;
        uint8_t* bottomRow = m_readbackBuffer.data() + (height - 1 - y) * rowSize;

        memcpy(tempRow.data(), topRow, rowSize);
        memcpy(topRow, bottomRow, rowSize);
        memcpy(bottomRow, tempRow.data(), rowSize);
    }
}

} // namespace Retro

#endif // ENABLE_HW_RENDER
