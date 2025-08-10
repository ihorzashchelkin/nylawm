#include "nyla.hpp"
#include <iterator>

static EGLDisplay eglDpy;
static EGLContext eglContext;
static EGLSurface eglSurface;

#define GL_FUNCTIONS(X)                                                                                                \
    X(PFNGLENABLEPROC, glEnable)                                                                                       \
    X(PFNGLDISABLEPROC, glDisable)                                                                                     \
    X(PFNGLBLENDFUNCPROC, glBlendFunc)                                                                                 \
    X(PFNGLVIEWPORTPROC, glViewport)                                                                                   \
    X(PFNGLCLEARCOLORPROC, glClearColor)                                                                               \
    X(PFNGLCLEARPROC, glClear)                                                                                         \
    X(PFNGLDRAWBUFFERPROC, glDrawBuffer)                                                                               \
    X(PFNGLDRAWARRAYSPROC, glDrawArrays)                                                                               \
    X(PFNGLCREATEBUFFERSPROC, glCreateBuffers)                                                                         \
    X(PFNGLNAMEDBUFFERSTORAGEPROC, glNamedBufferStorage)                                                               \
    X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                                                                     \
    X(PFNGLCREATEVERTEXARRAYSPROC, glCreateVertexArrays)                                                               \
    X(PFNGLVERTEXARRAYATTRIBBINDINGPROC, glVertexArrayAttribBinding)                                                   \
    X(PFNGLVERTEXARRAYVERTEXBUFFERPROC, glVertexArrayVertexBuffer)                                                     \
    X(PFNGLVERTEXARRAYATTRIBFORMATPROC, glVertexArrayAttribFormat)                                                     \
    X(PFNGLENABLEVERTEXARRAYATTRIBPROC, glEnableVertexArrayAttrib)                                                     \
    X(PFNGLCREATESHADERPROGRAMVPROC, glCreateShaderProgramv)                                                           \
    X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                                                                           \
    X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)                                                                 \
    X(PFNGLGENPROGRAMPIPELINESPROC, glGenProgramPipelines)                                                             \
    X(PFNGLUSEPROGRAMSTAGESPROC, glUseProgramStages)                                                                   \
    X(PFNGLBINDPROGRAMPIPELINEPROC, glBindProgramPipeline)                                                             \
    X(PFNGLPROGRAMUNIFORMMATRIX2FVPROC, glProgramUniformMatrix2fv)                                                     \
    X(PFNGLBINDTEXTUREUNITPROC, glBindTextureUnit)                                                                     \
    X(PFNGLCREATETEXTURESPROC, glCreateTextures)                                                                       \
    X(PFNGLTEXTUREPARAMETERIPROC, glTextureParameteri)                                                                 \
    X(PFNGLTEXTURESTORAGE2DPROC, glTextureStorage2D)                                                                   \
    X(PFNGLTEXTURESUBIMAGE2DPROC, glTextureSubImage2D)                                                                 \
    X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback)

#define X(type, name) static type name;
GL_FUNCTIONS(X)
#undef X

#ifndef NDEBUG
void GLDebugMessageCallback(GLenum source,
                            GLenum type,
                            GLuint id,
                            GLenum severity,
                            GLsizei length,
                            const GLchar* message,
                            const void* userParam)
{
    debug_fmt("GL CALLBACK: %s type = %#x, severity = %#x, message = %s",
              (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
              type,
              severity,
              message);
}
#endif

void initOverlay()
{
    eglDpy = eglGetPlatformDisplay(EGL_PLATFORM_X11_KHR, dpy, NULL);
    assert(eglDpy);

    EGLint major, minor;
    assert(eglInitialize(eglDpy, &major, &minor));
    assert(major > 1 || (major == 1 && minor >= 5));

    assert(eglBindAPI(EGL_OPENGL_API));

    const char* availableExts = eglQueryString(eglDpy, EGL_EXTENSIONS);
    assert(strstr(availableExts, "EGL_KHR_no_config_context"));
    assert(strstr(availableExts, "EGL_KHR_surfaceless_context"));
    assert(strstr(availableExts, "EGL_KHR_image_pixmap"));

    eglContext = eglCreateContext(eglDpy,
                                  EGL_NO_CONFIG_KHR,
                                  EGL_NO_CONTEXT,
                                  (EGLint[]){
                                      // clang-format off
                                      EGL_CONTEXT_MAJOR_VERSION,       4,
                                      EGL_CONTEXT_MINOR_VERSION,       5,
                                      EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                      EGL_CONTEXT_OPENGL_DEBUG,        EGL_TRUE,
                                      EGL_NONE
                                      // clang-format on
                                  });
    assert(eglContext);
    assert(eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext));

    // load OpenGL functions
#define X(type, name)                                                                                                  \
    name = (type)eglGetProcAddress(#name);                                                                             \
    assert(name);
    GL_FUNCTIONS(X)
#undef X

#ifndef NDEBUG
    glDebugMessageCallback(&GLDebugMessageCallback, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    bool ok = !xcb_checked(xcb_create_window,
                           XCB_COPY_FROM_PARENT,
                           overlayWindow,
                           screen->root,
                           0,
                           0,
                           screen->width_in_pixels,
                           screen->height_in_pixels,
                           0,
                           XCB_WINDOW_CLASS_INPUT_OUTPUT,
                           XCB_COPY_FROM_PARENT,
                           0,
                           (u32[]){});
    assert(ok && "could not create overlay window");

    EGLConfig configs[64];
    int numConfigs;
    assert(eglChooseConfig(eglDpy,
                           (EGLint[]){
                               // clang-format off
                                   EGL_SURFACE_TYPE,         EGL_WINDOW_BIT,
                                   EGL_CONFORMANT,           EGL_OPENGL_BIT,
                                   EGL_RENDERABLE_TYPE,      EGL_OPENGL_BIT,
                                   EGL_COLOR_BUFFER_TYPE,    EGL_RGB_BUFFER,

                                   EGL_RED_SIZE,             8,
                                   EGL_GREEN_SIZE,           8,
                                   EGL_BLUE_SIZE,            8,
                                   EGL_DEPTH_SIZE,           24,
                                   EGL_STENCIL_SIZE,         8,

                                   // uncomment for multisampled framebuffer
                                   // EGL_SAMPLE_BUFFERS,    1,
                                   // EGL_SAMPLES,           4, // 4x MSAA

                                   EGL_NONE,
                               // clang-format on
                           },
                           configs,
                           std::size(configs),
                           &numConfigs));

    for (EGLConfig* config = configs; config != configs + numConfigs; ++config)
    {
        EGLAttrib attr[] = {EGL_RENDER_BUFFER,
                            EGL_BACK_BUFFER,

                            // uncomment for sRGB framebuffer
                            // don't forget to call glEnable(GL_FRAMEBUFFER_SRGB) if shader outputs
                            // linear
                            // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,

                            EGL_NONE};

        eglSurface = eglCreatePlatformWindowSurface(eglDpy, config, &overlayWindow, attr);
        if (eglSurface)
            break;
    }

    assert(eglSurface != EGL_NO_SURFACE);
}
