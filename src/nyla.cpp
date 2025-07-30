#include <EGL/egl.h>
#include <X11/Xutil.h>
#include <csignal>
#include <iostream>
#include <print>
#include <span>

#include <fcntl.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/dri3.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "nyla.hpp"

namespace nyla {

const char*
initXcb(State& state, std::span<const Keybind> keybinds)
{
  int iscreen;
  state.xcb.conn = xcb_connect(nullptr, &iscreen);
  if (xcb_connection_has_error(state.xcb.conn))
    return "could not open display";

  state.xcb.screen = xcb_aux_get_screen(state.xcb.conn, iscreen);
  if (!state.xcb.screen)
    return "could not get screen";

  if (xcb_errors_context_new(state.xcb.conn, &state.xcb.errorContext))
    return "could not create xcb error context";

  state.xcb.window = xcb_generate_id(state.xcb.conn);
  if (xcb_request_check(
        state.xcb.conn,
        xcb_create_window_checked(state.xcb.conn,
                                  XCB_COPY_FROM_PARENT,
                                  state.xcb.window,
                                  state.xcb.screen->root,
                                  0,
                                  0,
                                  state.xcb.screen->width_in_pixels,
                                  state.xcb.screen->height_in_pixels,
                                  0,
                                  XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                  XCB_COPY_FROM_PARENT,
                                  0,
                                  nullptr))) {
    return "could not create window";
  }

  uint32_t rootEventMask =
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
    XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_EXPOSURE |
    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
    XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_KEY_PRESS |
    XCB_EVENT_MASK_KEY_RELEASE;
  if (xcb_request_check(
        state.xcb.conn,
        xcb_change_window_attributes_checked(state.xcb.conn,
                                             state.xcb.screen->root,
                                             XCB_CW_EVENT_MASK,
                                             &rootEventMask))) {
    return "could not change root window attributes. is another wm already "
           "running?";
  }

  if (xcb_request_check(
        state.xcb.conn,
        xcb_map_window_checked(state.xcb.conn, state.xcb.window))) {
    return "could not map window";
  }

  xcb_key_symbols_t* syms = xcb_key_symbols_alloc(state.xcb.conn);
  for (auto& keybind : keybinds) {
    xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(syms, keybind.key);
    if (!keycodes)
      return "could not get keycodes for a keysym";

    for (xcb_keycode_t* p = keycodes; *p != XCB_NO_SYMBOL; ++p) {
      state.keybinds.emplace_back(
        Keybind{ .mod = keybind.mod, .key = *p, .action = keybind.action });
    }
    free(keycodes);
  }
  free(syms);

  grabKeys(state);

  return nullptr;
}

void
grabKeys(State& state)
{
  for (auto& keybind : state.keybinds) {
    xcb_grab_key(state.xcb.conn,
                 0,
                 state.xcb.screen->root,
                 keybind.mod,
                 keybind.key,
                 XCB_GRAB_MODE_ASYNC,
                 XCB_GRAB_MODE_ASYNC);
  }
  xcb_flush(state.xcb.conn);
}

static void
render(State& state)
{
  for (auto [window, client] : state.clients) {
    if (!client.pixmap)
      continue;

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
    xcb_generic_event_t* event = xcb_poll_for_event(state.xcb.conn);
    if (event) {
      handleEvent(state, event);

      if (state.xcb.pendingRequests >= 32) {
        xcb_flush(state.xcb.conn);
        state.xcb.pendingRequests = 0;
      }

      free(event);
      continue;
    }

    if (auto err = xcb_connection_has_error(state.xcb.conn); err) {
      std::println(
        std::cerr,
        "xcb connection error: Bad{}",
        xcb_errors_get_name_for_error(state.xcb.errorContext, err, nullptr));
      break;
    }

    if (state.xcb.pendingRequests > 0) {
      xcb_flush(state.xcb.conn);
      state.xcb.pendingRequests = 0;
    }

    render(state);
  }
}

void
spawn(State& state, const char* const command[])
{
  if (fork() != 0)
    return;

  close(xcb_get_file_descriptor(state.xcb.conn));

  {
    int dev_null_fd = open("/dev/null", O_RDONLY);
    dup2(dev_null_fd, STDIN_FILENO);
    close(dev_null_fd);
  }

  {
    int dev_null_fd = open("/dev/null", O_WRONLY);
    dup2(dev_null_fd, STDOUT_FILENO);
    dup2(dev_null_fd, STDERR_FILENO);
    close(dev_null_fd);
  }

  setsid();

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_DFL;
  sigaction(SIGCHLD, &sa, nullptr);

  execvp(command[0], const_cast<char**>(command));
  std::println(std::cerr, "execvp failed");
  std::exit(EXIT_FAILURE);
}

void
quit(State& state)
{
  state.flags.reset(State::Flag_Running);
}

}
