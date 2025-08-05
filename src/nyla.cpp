#include "nyla.hpp"
#include <X11/Xlib.h>
#include <iostream>
#include <print>
#include <xcb/composite.h>
#include <xcb/dri3.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

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

    std::println(std::cerr,
                 "map state = {} {}",
                 xcb_get_window_attributes_reply(
                   state.dpy.xcb,
                   xcb_get_window_attributes(state.dpy.xcb, window),
                   nullptr)
                   ->map_state,
                 (int)XCB_MAP_STATE_VIEWABLE);

    client.pixmap = xcb_generate_id(state.dpy.xcb);
    if (xcb_request_check(state.dpy.xcb,
                          xcb_composite_name_window_pixmap_checked(
                            state.dpy.xcb, window, client.pixmap))) {
      std::println(std::cerr,
                   "xcb_composite_name_window_pixmap_checked FAILED");
      exit(1);
    }

    // TODO: check why is this failing
    // https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_image_pixmap.txt

    xcb_generic_error_t* err = nullptr;
    xcb_dri3_buffer_from_pixmap_reply_t* reply =
      xcb_dri3_buffer_from_pixmap_reply(
        state.dpy.xcb,
        xcb_dri3_buffer_from_pixmap(state.dpy.xcb, client.pixmap),
        &err);

    if (!reply) {
      std::println(std::cerr,
                   "buffer_from_pixmap failed: X error code {}",
                   err ? err->error_code : -1);
      std::exit(1);
    }

    if (reply) {
      int fd = xcb_dri3_buffer_from_pixmap_reply_fds(state.dpy.xcb, reply)[0];

      EGLImage image = eglCreateImage(
        state.dpy.egl, state.egl.context, EGL_GL_TEXTURE_2D, &fd, nullptr);
      std::println("{}", long(image));

      // free(reply);
    }
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
    if (XPending(state.dpy.x11)) {
      XEvent event;
      XNextEvent(state.dpy.x11, &event);
      handleEvent(state, &event);
      XFlush(state.dpy.x11);
      continue;
    }

    render(state);
  }
}

}
