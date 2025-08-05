#include "src/nyla.hpp"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <X11/Xlib.h>
#include <algorithm>
#include <print>

namespace nyla {

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

#ifndef NDEBUG
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
#endif

const char*
initEgl(State& state)
{
  EGLint major, minor;

  state.dpy.egl =
    eglGetPlatformDisplay(EGL_PLATFORM_X11_KHR, state.dpy.x11, nullptr);
  if (!state.dpy.egl)
    return "could not create egl display";

  if (!eglInitialize(state.dpy.egl, &major, &minor))
    return "could not initialize egl display";

  if (major < 1 || (major == 1 && minor < 5))
    return "egl version 1.5 or higher is required";

  if (!eglBindAPI(EGL_OPENGL_API))
    return "could not select opengl for egl";

  std::string availableExts = eglQueryString(state.dpy.egl, EGL_EXTENSIONS);
#if 0
  std::string availableExts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
  std::replace(availableExts.begin(), availableExts.end(), ' ', '\n');
  std::println("{}", availableExts);
#endif

  for (auto ext : { "EGL_KHR_no_config_context",
                    "EGL_KHR_surfaceless_context",
                    "EGL_KHR_image_pixmap" }) {
    if (!availableExts.contains(ext))
      return ext;
  }

  {
    EGLint attr[] = {
      // clang-format off
      EGL_CONTEXT_MAJOR_VERSION,       4,
      EGL_CONTEXT_MINOR_VERSION,       5,
      EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
      EGL_CONTEXT_OPENGL_DEBUG,        EGL_TRUE,
      EGL_NONE
      // clang-format on
    };
    state.egl.context =
      eglCreateContext(state.dpy.egl, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, attr);
  }

  if (!state.egl.context)
    return "could not create egl context, is opengl 4.5 supported?";

  if (!eglMakeCurrent(
        state.dpy.egl, EGL_NO_SURFACE, EGL_NO_SURFACE, state.egl.context))
    return "eglMakeCurrent failed";

  // load OpenGL functions
#define X(type, name)                                                          \
  name = (type)eglGetProcAddress(#name);                                       \
  assert(name);
  GL_FUNCTIONS(X)
#undef X

#ifndef NDEBUG
  glDebugMessageCallback(&GLDebugMessageCallback, nullptr);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

  {
    EGLint attr[] = {
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
    };

    EGLConfig configs[64];
    int numConfigs;
    if (!eglChooseConfig(
          state.dpy.egl, attr, configs, std::size(configs), &numConfigs)) {
      return "could not choose egl config";
    }

    for (auto& config : std::span{ configs, (size_t)numConfigs }) {
      EGLAttrib attr[] = {
        EGL_RENDER_BUFFER,
        EGL_BACK_BUFFER,

        // uncomment for sRGB framebuffer
        // don't forget to call glEnable(GL_FRAMEBUFFER_SRGB) if shader outputs
        // linear
        // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,

        EGL_NONE
      };

      state.egl.surface = eglCreatePlatformWindowSurface(
        state.dpy.egl, config, &state.window, attr);
      if (state.egl.surface != EGL_NO_SURFACE)
        break;
    }

    if (state.egl.surface == EGL_NO_SURFACE)
      return "could not create egl surface";
  }

  return nullptr;
}

#undef GL_FUNCTIONS
}
