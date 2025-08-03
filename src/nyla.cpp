#include "nyla.hpp"
#include <X11/Xlib.h>

namespace nyla {

const char*
eglGetErrorString(EGLint error)
{
#define CASE_STR(value)                                                        \
  case value:                                                                  \
    return #value;
  switch (error) {
    CASE_STR(EGL_SUCCESS)
    CASE_STR(EGL_NOT_INITIALIZED)
    CASE_STR(EGL_BAD_ACCESS)
    CASE_STR(EGL_BAD_ALLOC)
    CASE_STR(EGL_BAD_ATTRIBUTE)
    CASE_STR(EGL_BAD_CONTEXT)
    CASE_STR(EGL_BAD_CONFIG)
    CASE_STR(EGL_BAD_CURRENT_SURFACE)
    CASE_STR(EGL_BAD_DISPLAY)
    CASE_STR(EGL_BAD_SURFACE)
    CASE_STR(EGL_BAD_MATCH)
    CASE_STR(EGL_BAD_PARAMETER)
    CASE_STR(EGL_BAD_NATIVE_PIXMAP)
    CASE_STR(EGL_BAD_NATIVE_WINDOW)
    CASE_STR(EGL_CONTEXT_LOST)
    default:
      return "Unknown";
  }
#undef CASE_STR
}

static void
render(State& state)
{
  for (auto [window, client] : state.clients) {
    if (!client.pixmap)
      continue;

    // TODO: check why is this failing
    // https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_image_pixmap.txt

    EGLImage image = eglCreateImage(state.dpy.egl,
                                    EGL_NO_CONTEXT,
                                    EGL_NATIVE_PIXMAP_KHR,
                                    (EGLClientBuffer)(uintptr_t)client.pixmap,
                                    nullptr);

    if (image == EGL_NO_IMAGE) {
      std::println("{}", eglGetErrorString(eglGetError()));
    } else {
      std::println("Success??");

      eglDestroyImage(state.dpy.egl, image);
    }

#if 0
    xcb_dri3_buffer_from_pixmap(state.xcb.conn, client.pixmap);

    if (auto reply =
          xcb_dri3_buffer_from_pixmap_reply(state.xcb.conn, , nullptr);
        reply) {

      int fd = xcb_dri3_buffer_from_pixmap_reply_fds(state.xcb.conn, reply)[0];

      EGLImage image = eglCreateImage(
        state.egl.dpy, state.egl.context, EGL_GL_TEXTURE_2D, &fd, nullptr);
      std::println("{}", long(image));

      // free(reply);
    }
#endif
  }

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(30ms);
}

void
run(State& state)
{
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, nullptr);

  while (waitpid(-1, nullptr, WNOHANG) > 0)
    ;

  state.flags.set(State::Flag_Running);

  while (state.flags.test(State::Flag_Running)) {
    XEvent event;
    if (XPending(state.dpy.x11)) {
      XNextEvent(state.dpy.x11, &event);
      handleEvent(state, event);
      XFlush(state.dpy.x11);
      continue;
    }

    render(state);
  }
}

}
