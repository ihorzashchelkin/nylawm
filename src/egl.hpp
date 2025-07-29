#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glcorearb.h>

#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <print>
#include <xcb/xproto.h>

namespace nyla::egl {

// make sure you use functions that are valid for selected GL version (specified
// when context is created)
#define GL_FUNCTIONS(X)                                                        \
  X(PFNGLENABLEPROC, glEnable)                                                 \
  X(PFNGLDISABLEPROC, glDisable)                                               \
  X(PFNGLBLENDFUNCPROC, glBlendFunc)                                           \
  X(PFNGLVIEWPORTPROC, glViewport)                                             \
  X(PFNGLCLEARCOLORPROC, glClearColor)                                         \
  X(PFNGLCLEARPROC, glClear)                                                   \
  X(PFNGLDRAWBUFFERPROC, glDrawBuffer)                                         \
  X(PFNGLDRAWARRAYSPROC, glDrawArrays)                                         \
  X(PFNGLCREATEBUFFERSPROC, glCreateBuffers)                                   \
  X(PFNGLNAMEDBUFFERSTORAGEPROC, glNamedBufferStorage)                         \
  X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                               \
  X(PFNGLCREATEVERTEXARRAYSPROC, glCreateVertexArrays)                         \
  X(PFNGLVERTEXARRAYATTRIBBINDINGPROC, glVertexArrayAttribBinding)             \
  X(PFNGLVERTEXARRAYVERTEXBUFFERPROC, glVertexArrayVertexBuffer)               \
  X(PFNGLVERTEXARRAYATTRIBFORMATPROC, glVertexArrayAttribFormat)               \
  X(PFNGLENABLEVERTEXARRAYATTRIBPROC, glEnableVertexArrayAttrib)               \
  X(PFNGLCREATESHADERPROGRAMVPROC, glCreateShaderProgramv)                     \
  X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                                     \
  X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)                           \
  X(PFNGLGENPROGRAMPIPELINESPROC, glGenProgramPipelines)                       \
  X(PFNGLUSEPROGRAMSTAGESPROC, glUseProgramStages)                             \
  X(PFNGLBINDPROGRAMPIPELINEPROC, glBindProgramPipeline)                       \
  X(PFNGLPROGRAMUNIFORMMATRIX2FVPROC, glProgramUniformMatrix2fv)               \
  X(PFNGLBINDTEXTUREUNITPROC, glBindTextureUnit)                               \
  X(PFNGLCREATETEXTURESPROC, glCreateTextures)                                 \
  X(PFNGLTEXTUREPARAMETERIPROC, glTextureParameteri)                           \
  X(PFNGLTEXTURESTORAGE2DPROC, glTextureStorage2D)                             \
  X(PFNGLTEXTURESUBIMAGE2DPROC, glTextureSubImage2D)                           \
  X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback)

#define X(type, name) static type name;
GL_FUNCTIONS(X)
#undef X

void
GLDebugMessageCallback(GLenum aSource,
                       GLenum aType,
                       GLuint aId,
                       GLenum aSeverity,
                       GLsizei aLength,
                       const GLchar* aMessage,
                       const void* aUserParam)
{
  std::println(std::cerr,
               "GL CALLBACK: {} type = {:#x}, severity = {:#x}, message = {}",
               (aType == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
               aType,
               aSeverity,
               aMessage);
}

class EGLContainer
{
  static constexpr int EGL_MINIMUM_MAJOR_VERSION = 1;
  static constexpr int EGL_MINIMUM_MINOR_VERSION = 5;

  EGLDisplay mDisplay{};
  EGLContext mContext{};
  xcb_screen_t* mScreen{};
  xcb_window_t mCompostitorWindow{};

public:
  EGLContainer()
  {
    int nScreen;
    xcb_connection_t* connection = xcb_connect(nullptr, &nScreen);
    if (xcb_connection_has_error(connection)) {
      std::println(std::cerr, "could not open display");
      return;
    }

    mScreen = xcb_aux_get_screen(connection, nScreen);
    if (!mScreen) {
      std::println(std::cerr, "could not get screen");
      return;
    }

    mDisplay = eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, connection, nullptr);
    if (mDisplay == EGL_NO_DISPLAY) {
      std::println(std::cerr, "could not create egl display");
      return;
    }

    EGLint major, minor;
    if (!eglInitialize(mDisplay, &major, &minor)) {
      std::println(std::cerr, "could not initialize egl display");
      return;
    }

    if ((major < EGL_MINIMUM_MAJOR_VERSION) ||
        (major == EGL_MINIMUM_MAJOR_VERSION &&
         minor < EGL_MINIMUM_MINOR_VERSION)) {
      std::println(std::cerr,
                   "egl version {}.{} or higher is required",
                   EGL_MINIMUM_MAJOR_VERSION,
                   EGL_MINIMUM_MINOR_VERSION);
      return;
    }

    EGLBoolean ok = eglBindAPI(EGL_OPENGL_API);
    assert(ok && "could not select opengl for egl");

    {
      const char* exts = eglQueryString(mDisplay, EGL_EXTENSIONS);
      auto checkExt = [&](const char* aExt) -> bool {
        if (strstr(exts, aExt)) {
          return true;
        } else {
          std::println("{} extension is required", aExt);
          return false;
        }
      };

      assert(checkExt("EGL_KHR_no_config_context"));
      assert(checkExt("EGL_KHR_surfaceless_context"));
    }

    mContext = eglCreateContext(mDisplay,
                                EGL_NO_CONFIG_KHR,
                                EGL_NO_CONTEXT,
                                (EGLint[]){
                                  // clang-format off
                                  EGL_CONTEXT_MAJOR_VERSION,        4,
                                  EGL_CONTEXT_MINOR_VERSION,        5,
                                  EGL_CONTEXT_OPENGL_PROFILE_MASK,  EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                  EGL_CONTEXT_OPENGL_DEBUG,         EGL_TRUE,
                                  EGL_NONE
                                  // clang-format on
                                });

    if (mContext == EGL_NO_CONTEXT) {
      std::println(std::cerr,
                   "cannot create egl context, opengl 4.5 not supported?");
    }

    ok = eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, mContext);
    assert(ok && "eglMakeCurrent failed");

    // load OpenGL functions
#define X(type, name)                                                          \
  name = (type)eglGetProcAddress(#name);                                       \
  assert(name);
    GL_FUNCTIONS(X)
#undef X

    glDebugMessageCallback(&GLDebugMessageCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    mCompostitorWindow = xcb_generate_id(connection);
    if (xcb_request_check(
          connection,
          xcb_create_window_checked(connection,
                                    XCB_COPY_FROM_PARENT,
                                    mCompostitorWindow,
                                    mScreen->root,
                                    0,
                                    0,
                                    mScreen->width_in_pixels,
                                    mScreen->height_in_pixels,
                                    0,
                                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                    XCB_COPY_FROM_PARENT,
                                    0,
                                    nullptr))) {
      assert(mCompostitorWindow && "could not create window");
    }
  }

private:
}

void
a()
{
}
}

#undef GL_FUNCTIONS
}
