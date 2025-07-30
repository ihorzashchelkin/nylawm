// example how to set up OpenGL core context on X11 with EGL
// and use basic functionality of OpenGL 4.5 version

// to compile on Ubuntu first install following packages: build-essential
// libx11-dev libgl-dev libegl-dev then run: gcc x11_opengl.c -o x11_opengl -lm
// -lX11 -lEGL

// important extension functionality used here:
// (4.3) KHR_debug:
// https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_debug.txt (4.5)
// ARB_direct_state_access:
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_direct_state_access.txt
// (4.1) ARB_separate_shader_objects:
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_separate_shader_objects.txt
// (4.2) ARB_shading_language_420pack:
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_shading_language_420pack.txt
// (4.3) ARB_explicit_uniform_location:
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_explicit_uniform_location.txt

// assumes following EGL extensions are present (should be always present on
// modern EGL 1.5 implementation): EGL_KHR_surfaceless_context:
// https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_surfaceless_context.txt
// EGL_KHR_no_config_context:
// https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_no_config_context.txt
// EGL_KHR_platform_x11:
// https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_platform_x11.txt

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glcorearb.h>
#include <X11/Xlib.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// replace this with your favorite Assert() implementation
#include <assert.h>
#define Assert(cond) assert(cond)

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

#define STR2(x) #x
#define STR(x) STR2(x)

static void
FatalError(const char* message)
{
  fprintf(stderr, "%s\n", message);

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "zenity --error --no-wrap --text=\"%s\"", message);
  system(cmd);

  exit(0);
}

#ifndef NDEBUG
static void APIENTRY
DebugCallback(GLenum source,
              GLenum type,
              GLuint id,
              GLenum severity,
              GLsizei length,
              const GLchar* message,
              const void* user)
{
  fprintf(stderr, "%s\n", message);
  if (severity == GL_DEBUG_SEVERITY_HIGH ||
      severity == GL_DEBUG_SEVERITY_MEDIUM) {
    assert(!"OpenGL API usage error! Use debugger to examine call stack!");
  }
}
#endif

int
main()
{
  Display* dpy = XOpenDisplay(NULL);
  if (!dpy) {
    FatalError("Cannot open X display");
  }

  // first create EGL context that can be done independently from any X11
  // windows created later

  // initialize EGL
  EGLDisplay* display;
  {
    display = eglGetPlatformDisplay(EGL_PLATFORM_X11_KHR, dpy, NULL);
    if (display == EGL_NO_DISPLAY) {
      FatalError("Cannot create EGL display");
    }

    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
      FatalError("Cannot initialize EGL display");
    }
    if (major < 1 || (major == 1 && minor < 5)) {
      FatalError("EGL version 1.5 or higher required");
    }
  }

  // choose OpenGL API for EGL, by default it uses OpenGL ES
  EGLBoolean ok = eglBindAPI(EGL_OPENGL_API);
  Assert(ok && "Failed to select OpenGL API for EGL");

  // first create EGL context, suface doesn't matter, this context can be shared
  // across many surfaces
  EGLContext context;
  {
    EGLint attr[] = {
      EGL_CONTEXT_MAJOR_VERSION,
      4,
      EGL_CONTEXT_MINOR_VERSION,
      5,
      EGL_CONTEXT_OPENGL_PROFILE_MASK,
      EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
#ifndef NDEBUG
      // ask for debug context for non "Release" builds
      // this is so we can enable debug callback
      EGL_CONTEXT_OPENGL_DEBUG,
      EGL_TRUE,
#endif
      EGL_NONE,
    };

    context =
      eglCreateContext(display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, attr);
    if (context == EGL_NO_CONTEXT) {
      FatalError("Cannot create EGL context, OpenGL 4.5 not supported?");
    }
  }

  // make GL context active on current thread, no output surface needed
  // you can skip this and activate GL context only when you have EGL surface
  // created if all you want is to have only one context & one surface
  ok = eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
  Assert(ok && "Failed to make context current");

  // load OpenGL functions
#define X(type, name)                                                          \
  name = (type)eglGetProcAddress(#name);                                       \
  Assert(name);
  GL_FUNCTIONS(X)
#undef X

#ifndef NDEBUG
  // enable debug callback
  glDebugMessageCallback(&DebugCallback, NULL);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

  // now can create arbitrary amount of X11 windows & their EGL surface
  // this example creates only one window

  XSetWindowAttributes attributes = {
    .event_mask = StructureNotifyMask,
  };

  // create window
  int width = 1280;
  int height = 720;
  Window window = XCreateWindow(dpy,
                                DefaultRootWindow(dpy),
                                0,
                                0,
                                width,
                                height,
                                0,
                                CopyFromParent,
                                InputOutput,
                                CopyFromParent,
                                CWEventMask,
                                &attributes);
  Assert(window && "Failed to create window");

  // uncomment in case you want fixed size window
  // XSizeHints* hints = XAllocSizeHints();
  // Assert(hints);
  // hints->flags |= PMinSize | PMaxSize;
  // hints->min_width  = hints->max_width  = width;
  // hints->min_height = hints->max_height = height;
  // XSetWMNormalHints(dpy, window, hints);
  // XFree(hints);

  // set window title
  XStoreName(dpy, window, "OpenGL Window");

  // subscribe to window close notification
  Atom WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
  Atom WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dpy, window, &WM_DELETE_WINDOW, 1);

  // create EGL surface for X11 window
  EGLSurface surface = EGL_NO_SURFACE;
  {
    // first query matching EGL configurations
    EGLConfig configs[64];
    EGLint config_count = sizeof(configs) / sizeof(configs[0]);
    {
      EGLint attr[] = {
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_CONFORMANT,
        EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_BIT,
        EGL_COLOR_BUFFER_TYPE,
        EGL_RGB_BUFFER,

        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_DEPTH_SIZE,
        24,
        EGL_STENCIL_SIZE,
        8,

        // uncomment for multisampled framebuffer
        // EGL_SAMPLE_BUFFERS, 1,
        // EGL_SAMPLES,        4, // 4x MSAA

        EGL_NONE,
      };
      if (!eglChooseConfig(
            display, attr, configs, config_count, &config_count) ||
          config_count == 0) {
        FatalError("Cannot choose EGL configs");
      }
    }

    // then try all configs, to find out which one works for selected
    // attributes, because it is not guaranteed that first config will work - as
    // there's no way to choose config with specific EGL_GL_COLORSPACE value
    for (EGLint i = 0; i < config_count; i++) {
      EGLAttrib attr[] = {
        EGL_RENDER_BUFFER,
        EGL_BACK_BUFFER, // double buffered framebuffer

        // uncomment for sRGB framebuffer
        // don't forget to call glEnable(GL_FRAMEBUFFER_SRGB) if shader outputs
        // linear
        // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,

        EGL_NONE,
      };
      surface =
        eglCreatePlatformWindowSurface(display, configs[i], &window, attr);
      if (surface != EGL_NO_SURFACE) {
        break;
      }
    }

    if (surface == EGL_NO_SURFACE) {
      FatalError("Cannot create EGL surface");
    }
  }

  // create GL objects

  struct Vertex
  {
    float position[2];
    float uv[2];
    float color[3];
  };

  // vertex buffer containing triangle vertices
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

  // vertex input
  GLuint vao;
  {
    glCreateVertexArrays(1, &vao);

    GLint vbuf_index = 0;
    glVertexArrayVertexBuffer(vao, vbuf_index, vbo, 0, sizeof(struct Vertex));

    GLint a_pos = 0;
    glVertexArrayAttribFormat(
      vao, a_pos, 2, GL_FLOAT, GL_FALSE, offsetof(struct Vertex, position));
    glVertexArrayAttribBinding(vao, a_pos, vbuf_index);
    glEnableVertexArrayAttrib(vao, a_pos);

    GLint a_uv = 1;
    glVertexArrayAttribFormat(
      vao, a_uv, 2, GL_FLOAT, GL_FALSE, offsetof(struct Vertex, uv));
    glVertexArrayAttribBinding(vao, a_uv, vbuf_index);
    glEnableVertexArrayAttrib(vao, a_uv);

    GLint a_color = 2;
    glVertexArrayAttribFormat(
      vao, a_color, 3, GL_FLOAT, GL_FALSE, offsetof(struct Vertex, color));
    glVertexArrayAttribBinding(vao, a_color, vbuf_index);
    glEnableVertexArrayAttrib(vao, a_color);
  }

  // checkerboard texture, with 50% transparency on black colors
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

  // fragment & vertex shaders for drawing triangle
  GLuint pipeline, vshader, fshader;
  {
    // clang-format off
    const char* glsl_vshader =
	"#version 450 core                             \n"
	"#line " STR(__LINE__) "                     \n\n" // actual line number in this file for nicer error messages
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
	"#line " STR(__LINE__) "                     \n\n" // actual line number in this file for nicer error messages
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

  // setup global GL state
  {
    // enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // disble depth testing
    glDisable(GL_DEPTH_TEST);

    // disable culling
    glDisable(GL_CULL_FACE);
  }

  // show the window
  XMapWindow(dpy, window);

  // activate EGL context for specific EGL surface on current thread
  // do this call before issuing draw calls to make the output go to specific
  // EGL surface
  ok = eglMakeCurrent(display, surface, surface, context);
  Assert(ok && "Failed to make context current");

  // use 0 to disable vsync
  int vsync = 1;
  ok = eglSwapInterval(display, vsync);
  Assert(ok && "Failed to set vsync");

  struct timespec c1;
  clock_gettime(CLOCK_MONOTONIC, &c1);

  float angle = 0;

  for (;;) {
    // process all incoming X11 events
    if (XPending(dpy)) {
      XEvent event;
      XNextEvent(dpy, &event);
      if (event.type == ClientMessage) {
        if (event.xclient.message_type == WM_PROTOCOLS) {
          Atom protocol = event.xclient.data.l[0];
          if (protocol == WM_DELETE_WINDOW) {
            // window closed, exit the for loop
            break;
          }
        }
      }
      continue;
    }

    // get current window size
    XWindowAttributes attr;
    Status status = XGetWindowAttributes(dpy, window, &attr);
    Assert(status && "Failed to get window attributes");

    width = attr.width;
    height = attr.height;

    struct timespec c2;
    clock_gettime(CLOCK_MONOTONIC, &c2);
    float delta =
      (float)(c2.tv_sec - c1.tv_sec) + 1e-9f * (c2.tv_nsec - c1.tv_nsec);
    c1 = c2;

    // render only if window size is non-zero
    if (width != 0 && height != 0) {
      // make sure backbuffer is selected for output
      glDrawBuffer(GL_BACK);

      // setup output size covering all client area of window
      glViewport(0, 0, width, height);

      // clear screen
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

      // bind texture to texture unit
      GLint s_texture = 0; // texture unit that sampler2D will use in GLSL code
      glBindTextureUnit(s_texture, texture);

      // draw 3 vertices as triangle
      glDrawArrays(GL_TRIANGLES, 0, 3);

      // swap the buffers to show output
      if (!eglSwapBuffers(display, surface)) {
        FatalError("Failed to swap OpenGL buffers!");
      }
    } else {
      // window is minimized, cannot vsync - instead sleep a bit
      if (vsync) {
        usleep(10 * 1000);
      }
    }
  }
}
