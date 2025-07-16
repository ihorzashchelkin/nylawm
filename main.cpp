#include <experimental/scope>
#include <iostream>
#include <ostream>
#include <string_view>

#include <csignal>
#include <cstdint>
#include <cstdlib>

#include <sys/wait.h>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xproto.h>

#include "config.hpp"
#include "util.hpp"

bool try_startup(const char *displayname, xcb_connection_t *&connection,
                 xcb_window_t &root);

int main(int argc, char *argv[]) {
  if (argc == 2 && std::string_view{argv[1]} == "-v") {
    std::cout << argv[0] << '-' << version << std::endl;
    return EXIT_SUCCESS;
  }

  if (argc > 1) {
    std::cout << "usage: " << argv[0] << " [-v]" << std::endl;
    return EXIT_SUCCESS;
  }

  xcb_connection_t *connection;
  xcb_window_t root;

  if (!try_startup(nullptr, connection, root)) {
    if (!fallback_display)
      return EXIT_FAILURE;

    std::cerr << "trying " << fallback_display << " fallback..." << std::endl;
    if (!try_startup(fallback_display, connection, root))
      return EXIT_FAILURE;
  }

  while (true) {
    xcb_generic_event_t *event = xcb_wait_for_event(connection);
    if (!event) {
      std::cerr << "lost connection to X server" << std::endl;
      break;
    }

    std::experimental::scope_exit free_event([&] { free(event); });
  }
}

bool try_startup(const char *displayname, xcb_connection_t *&connection,
                 xcb_window_t &root) {
  int default_screen_number;
  connection = xcb_connect(displayname, &default_screen_number);
  if (xcb_connection_has_error(connection)) {
    std::cerr << "could connect to X server" << std::endl;
    return false;
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
    return false;
  }

  root = screen->root;

  xcb_generic_error_t *error = xcb_request_check(
      connection, xcb_change_window_attributes_checked(
                      connection, root, XCB_CW_EVENT_MASK,
                      (uint32_t[]){XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT}));
  if (error) {
    log_xcb_error(connection, error);
    if (error->error_code == 10)
      std::cerr << "another wm is already runting?" << std::endl;

    return false;
  }

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, nullptr);

  while (waitpid(-1, nullptr, WNOHANG) > 0)
    ;

  return true;
}
