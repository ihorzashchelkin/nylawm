#include <iostream>
#include <ostream>
#include <string_view>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <liburing.h>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xproto.h>

#include "util.hpp"

#define STRING(s) #s

#ifndef VERSION
#define VERSION "unset version"
#endif

constexpr const char *fallback_display = ":1";

struct WMState {
  xcb_connection_t *connection;
  xcb_window_t root;
};

WMState *try_startup(const char *displayname);

int main(int argc, char *argv[]) {
  if (argc == 2 && std::string_view{argv[1]} == "-v") {
    std::cout << argv[0] << '-' << STRING(VERSION) << std::endl;
    return EXIT_SUCCESS;
  }

  if (argc > 1) {
    std::cout << "usage: " << argv[0] << " [-v]" << std::endl;
    return EXIT_SUCCESS;
  }

  WMState *state = try_startup(nullptr);
  if (!state) {
    if (fallback_display) {
      std::cout << "trying " << fallback_display << " fallback..." << std::endl;
      state = try_startup(fallback_display);
    }

    if (!state)
      return EXIT_FAILURE;
  }

  xcb_connection_t *&connection = state->connection;
  uint32_t xcb_fd = xcb_get_file_descriptor(connection);

  io_uring ring;
  if (io_uring_queue_init(8, &ring, 0) < 0) {
    std::cerr << "io_uring_queue_init: " << strerror(errno) << std::endl;
    return EXIT_FAILURE;
  }
}

WMState *try_startup(const char *displayname) {
  int default_screen_number;
  xcb_connection_t *connection =
      xcb_connect(displayname, &default_screen_number);
  if (xcb_connection_has_error(connection)) {
    std::cerr << "could connect to X server" << std::endl;
    return nullptr;
  }

  const xcb_setup_t *setup = xcb_get_setup(connection);
  xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

  xcb_screen_t *screen = nullptr;
  for (int i = 0; screen_iterator.rem && i <= default_screen_number;
       xcb_screen_next(&screen_iterator), i++) {
    if (i == default_screen_number)
      screen = screen_iterator.data;
  }

  if (!screen) {
    std::cerr << "could not find screen: " << default_screen_number
              << std::endl;
    return nullptr;
  }

  xcb_window_t &root = screen->root;

  xcb_generic_error_t *error = xcb_request_check(
      connection, xcb_change_window_attributes_checked(
                      connection, root, XCB_CW_EVENT_MASK,
                      (uint32_t[]){XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT}));
  if (error) {
    log_xcb_error(connection, error);
    if (error->error_code == 10)
      std::cerr << "another wm is already running?" << std::endl;

    return nullptr;
  }

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, nullptr);

  while (waitpid(-1, nullptr, WNOHANG) > 0)
    ;

  return new WMState{connection, root};
}
