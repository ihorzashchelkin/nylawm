#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glcorearb.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>

#include <xcb/xcb.h>

#include <iostream>
#include <print>

#include <cassert>
#include <unistd.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

#define Assert(x) assert(x)

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

void APIENTRY
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

int
main()
{
  int nScreen;
  xcb_connection_t* connection = xcb_connect(nullptr, &nScreen);
  if (xcb_connection_has_error(connection)) {
    std::println(std::cerr, "could not open display");
    return 1;
  }

  xcb_screen_t* screen = xcb_aux_get_screen(connection, nScreen);

  EGLDisplay display;
  {
    display = eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, connection, nullptr);
    if (display == EGL_NO_DISPLAY) {
      std::println(std::cerr, "could not create egl display");
      return 1;
    }

    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
      std::println(std::cerr, "could not initialize egl display");
      return 1;
    }

    if (major < 1 || (major == 1 && minor < 5)) {
      std::println(std::cerr, "EGL version 1.5 or higher is required");
      return 1;
    }
  }

  EGLBoolean ok = eglBindAPI(EGL_OPENGL_API);
  Assert(ok && "Failed to select OpenGL API for EGL");

  const char* exts = eglQueryString(display, EGL_EXTENSIONS);
  Assert(strstr(exts, "EGL_KHR_no_config_context"));
  Assert(strstr(exts, "EGL_KHR_surfaceless_context"));
  std::println("{}", exts);

  // clang-format off
  EGLint attr[] = {
    EGL_CONTEXT_MAJOR_VERSION,          4,
    EGL_CONTEXT_MINOR_VERSION,          5,
    EGL_CONTEXT_OPENGL_PROFILE_MASK,    EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
    EGL_CONTEXT_OPENGL_DEBUG,           EGL_TRUE,
    EGL_NONE
  };
  // clang-format on

  EGLContext context =
    eglCreateContext(display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, attr);
  if (context == EGL_NO_CONTEXT) {
    std::println(std::cerr,
                 "cannot create egl context, opengl 4.5 not supported?");
    return 1;
  }

  ok = eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
  Assert(ok && "Failed to make context current");

  // load OpenGL functions
#define X(type, name)                                                          \
  name = (type)eglGetProcAddress(#name);                                       \
  Assert(name);
  GL_FUNCTIONS(X)
#undef X

  glDebugMessageCallback(&GLDebugMessageCallback, nullptr);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  int width = 1280;
  int height = 720;

  xcb_window_t window = xcb_generate_id(connection);

  if (xcb_request_check(connection,
                        xcb_create_window_checked(
                          connection,
                          XCB_COPY_FROM_PARENT,
                          window,
                          screen->root,
                          0,
                          0,
                          width,
                          height,
                          0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          XCB_COPY_FROM_PARENT,
                          XCB_CW_EVENT_MASK,
                          (uint32_t[]){ XCB_EVENT_MASK_STRUCTURE_NOTIFY }))) {
    Assert(window && "Failed to create window");
  }

  EGLConfig configs[64];
  EGLint config_count = sizeof(configs) / sizeof(configs[0]);
  { // clang-format off
    EGLint attr[] = {
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
    };
    // clang-format on

    if (!eglChooseConfig(display, attr, configs, config_count, &config_count) ||
        config_count == 0) {
      std::println(std::cerr, "could not choose egl configs");
      return 1;
    }
  }

  EGLSurface surface;
  for (EGLint i = 0; i < config_count; ++i) {
    EGLAttrib attr[] = {
      EGL_RENDER_BUFFER,
      EGL_BACK_BUFFER,

      // uncomment for sRGB framebuffer
      // don't forget to call glEnable(GL_FRAMEBUFFER_SRGB) if shader outputs
      // linear
      // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,

      EGL_NONE
    };

    surface =
      eglCreatePlatformWindowSurface(display, configs[i], &window, attr);
    if (surface != EGL_NO_SURFACE) {
      break;
    }
  }

  if (surface == EGL_NO_SURFACE) {
    std::println(std::cerr, "could not create egl surface");
    return 1;
  }

  struct Vertex
  {
    float position[2];
    float uv[2];
    float color[3];
  };

  GLuint vbo;
  {
    struct Vertex data[] = {
      { { -0.00f, +0.75f }, { 25.0f, 50.0f }, { 1, 0, 0 } },
      { { +0.75f, -0.50f }, { 0.0f, 0.0f }, { 0, 1, 0 } },
      { { -0.75f, -0.50f }, { 50.0f, 0.0f }, { 0, 0, 1 } },
    };
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, sizeof(data), data, 0);
  }

  GLuint vao;
  {
    glCreateVertexArrays(1, &vao);

    GLint vbuf_index = 0;
    glVertexArrayVertexBuffer(vao, vbuf_index, vbo, 0, sizeof(Vertex));

    GLint a_pos = 0;
    glVertexArrayAttribFormat(
      vao, a_pos, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(vao, a_pos, vbuf_index);
    glEnableVertexArrayAttrib(vao, a_pos);

    GLint a_uv = 1;
    glVertexArrayAttribFormat(
      vao, a_uv, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
    glVertexArrayAttribBinding(vao, a_uv, vbuf_index);
    glEnableVertexArrayAttrib(vao, a_uv);

    GLint a_color = 2;
    glVertexArrayAttribFormat(
      vao, a_color, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, color));
    glVertexArrayAttribBinding(vao, a_color, vbuf_index);
    glEnableVertexArrayAttrib(vao, a_color);
  }

  GLuint texture;
  {
    unsigned int pixels[] = {
      0x80000000,
      0xffffffff,
      0xffffffff,
      0x80000000,
    };

    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLsizei width = 2;
    GLsizei height = 2;
    glTextureStorage2D(texture, 1, GL_RGBA8, width, height);
    glTextureSubImage2D(
      texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  }

  GLuint pipeline, vshader, fshader;
  {
    // clang-format off
    const char* glsl_vshader =
      "#version 450 core                             \n"
      "                                              \n"
      "layout (location=0) in vec2 a_pos;            \n" // position attribute index 0
      "layout (location=1) in vec2 a_uv;             \n" // uv attribute index 1
      "layout (location=2) in vec3 a_color;          \n" // color attribute index 2
      "                                              \n"
      "layout (location=0)                           \n" // (from ARB_explicit_uniform_location)
      "uniform mat2 u_matrix;                        \n" // matrix uniform location 0
      "                                              \n"
      "out gl_PerVertex { vec4 gl_Position; };       \n" // required because of ARB_separate_shader_objects
      "out vec2 uv;                                  \n"
      "out vec4 color;                               \n"
      "                                              \n"
      "void main()                                   \n"
      "{                                             \n"
      "    vec2 pos = u_matrix * a_pos;              \n"
      "    gl_Position = vec4(pos, 0, 1);            \n"
      "    uv = a_uv;                                \n"
      "    color = vec4(a_color, 1);                 \n"
      "}                                             \n"
      ;

    const char* glsl_fshader =
      "#version 450 core                             \n"
      "                                              \n"
      "in vec2 uv;                                   \n"
      "in vec4 color;                                \n"
      "                                              \n"
      "layout (binding=0)                            \n" // (from ARB_shading_language_420pack)
      "uniform sampler2D s_texture;                  \n" // texture unit binding 0
      "                                              \n"
      "layout (location=0)                           \n"
      "out vec4 o_color;                             \n" // output fragment data location 0
      "                                              \n"
      "void main()                                   \n"
      "{                                             \n"
      "    o_color = color * texture(s_texture, uv); \n"
      "}                                             \n"
      ;
    // clang-format on
    //

    vshader = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vshader);
    fshader = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fshader);

    GLint linked;
    glGetProgramiv(vshader, GL_LINK_STATUS, &linked);
    if (!linked) {
      char message[1024];
      glGetProgramInfoLog(vshader, sizeof(message), NULL, message);
      fprintf(stderr, "%s\n", message);
      Assert(!"Failed to create vertex shader!");
    }

    glGetProgramiv(fshader, GL_LINK_STATUS, &linked);
    if (!linked) {
      char message[1024];
      glGetProgramInfoLog(fshader, sizeof(message), NULL, message);
      fprintf(stderr, "%s\n", message);
      Assert(!"Failed to create fragment shader!");
    }

    glGenProgramPipelines(1, &pipeline);
    glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, vshader);
    glUseProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, fshader);
  }

  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    glDisable(GL_CULL_FACE);
  }

  xcb_map_window(connection, window);
  xcb_flush(connection);

  ok = eglMakeCurrent(display, surface, surface, context);
  Assert(ok && "Failed to make context current");

  int vsync = 1;
  ok = eglSwapInterval(display, vsync);
  Assert(ok && "Failed to set vsync");

  timespec c1;
  clock_gettime(CLOCK_MONOTONIC, &c1);

  float angle = 0.0f;

  for (;;) {
    auto attr = xcb_get_geometry_reply(
      connection, xcb_get_geometry(connection, window), nullptr);

    Assert(attr && "Failed to get window attributes");

    width = attr->width;
    height = attr->height;

    timespec c2;
    clock_gettime(CLOCK_MONOTONIC, &c2);
    float delta =
      (float)(c2.tv_sec - c1.tv_sec) + 1e-9f * (c2.tv_nsec - c1.tv_nsec);
    c1 = c2;

    if (width != 0 && height != 0) {
      glDrawBuffer(GL_BACK); // make sure backbuffer is selected for output
                             //

      glViewport(0, 0, width, height);

      glClearColor(0.392f, 0.584f, 0.929f, 1.f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
              GL_STENCIL_BUFFER_BIT);

      // setup rotation matrix in uniform
      {
        angle +=
          delta * 2.0f * (float)M_PI / 20.0f; // full rotation in 20 seconds
        angle = fmodf(angle, 2.0f * (float)M_PI);

        float aspect = (float)height / width;
        float matrix[] = {
          cosf(angle) * aspect,
          -sinf(angle),
          sinf(angle) * aspect,
          cosf(angle),
        };

        GLint u_matrix = 0;
        glProgramUniformMatrix2fv(vshader, u_matrix, 1, GL_FALSE, matrix);
      }

      // activate shaders for next draw call
      glBindProgramPipeline(pipeline);

      // provide vertex input
      glBindVertexArray(vao);

      GLint s_texture = 0;
      glBindTextureUnit(s_texture, texture);

      glDrawArrays(GL_TRIANGLES, 0, 3);

      if (!eglSwapBuffers(display, surface)) {
        std::println(std::cerr, "could not swap opengl buffers");
        return 1;
      }
    } else {
      if (vsync)
        usleep(10 * 1000);
    }
  }
}
