#include "wm.hpp"
#include "config.hpp"
#include "util.hpp"

#include <cstdlib>
#include <experimental/scope>

#include <format>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

namespace wm {

bool Instance::try_init() {
  int default_screen_number;
  conn_ = xcb_connect(nullptr, &default_screen_number);
  if (xcb_connection_has_error(conn_)) {
    std::cerr << "could not connect to X server" << std::endl;
    return false;
  }

  const xcb_setup_t *setup = xcb_get_setup(conn_);
  xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

  screen_ = nullptr;
  for (int i = 0; screen_iterator.rem && i <= default_screen_number;
       xcb_screen_next(&screen_iterator), i++) {
    if (i == default_screen_number)
      screen_ = screen_iterator.data;
  }

  if (!screen_) {
    std::cerr << "could not find screen: " << default_screen_number
              << std::endl;
    return false;
  }

  constexpr const xcb_cw_t cw_attr = XCB_CW_EVENT_MASK;
  constexpr const xcb_event_mask_t event_masks[] = {
      XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, XCB_EVENT_MASK_KEY_PRESS,
      XCB_EVENT_MASK_BUTTON_PRESS, XCB_EVENT_MASK_BUTTON_RELEASE};

  xcb_generic_error_t *error =
      xcb_request_check(conn_, xcb_change_window_attributes_checked(
                                   conn_, screen_->root, cw_attr, event_masks));
  if (error) {
    util::log_xcb_error(conn_, error);
    if (error->error_code == 10) // Bad Access
      std::cerr << "is another wm already running?" << std::endl;

    return false;
  }

  return true;
}

void Instance::resolve_keybinds() {
  xcb_key_symbols_t *syms = xcb_key_symbols_alloc(conn_);
  std::experimental::scope_exit free_syms([&] { free(syms); });

  for (const KeyBind &bind : config::keybinds) {
    xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(syms, bind.keysym);
    std::experimental::scope_exit free_keycodes([=] { free(keycodes); });

    while (*keycodes != XCB_NO_SYMBOL) {
      resolved_keybinds_.emplace_back(
          (ResolvedKeyBind){bind.modifiers, *keycodes, bind.handler});
      keycodes++;
    }
  }
}

void Instance::grab_keys() {
  for (const ResolvedKeyBind &resolved_bind : resolved_keybinds_) {
    xcb_grab_key(conn_, 0, screen_->root, resolved_bind.modifiers,
                 resolved_bind.keycode, XCB_GRAB_MODE_ASYNC,
                 XCB_GRAB_MODE_ASYNC);
  }

  flush();
}

void Instance::run() {
  prepare_wm_process();
  resolve_keybinds();
  grab_keys();

  running_ = true;
  while (running_) {
    xcb_generic_event_t *event = xcb_wait_for_event(conn_);
    if (!event) {
      std::cerr << "lost connection to X server" << std::endl;
      break;
    }

    std::experimental::scope_exit free_event([&] { free(event); });

    int event_type = (event->response_type & ~0x80);
    switch (event_type) {
    case XCB_MAPPING_NOTIFY: {
      if (((xcb_mapping_notify_event_t *)event)->request ==
          XCB_MAPPING_KEYBOARD)
        grab_keys();
      break;
    }

    case XCB_KEY_PRESS: {
      handle_key_press((xcb_key_press_event_t *)event);
      break;
    }
    }
  }
}

void Instance::spawn(const char *const command[]) {
  if (fork() != 0)
    return;

  prepare_spawn_process();

  execvp(command[0], const_cast<char **>(command));
  throw new std::runtime_error(std::format("execvp '%s' failed", command[0]));
}

void Instance::prepare_wm_process() {
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, nullptr);

  while (waitpid(-1, nullptr, WNOHANG) > 0)
    ;
}

void Instance::prepare_spawn_process() {
  xcb_disconnect(conn_);

  setsid();

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_DFL;
  sigaction(SIGCHLD, &sa, nullptr);
}

void Instance::handle_key_press(xcb_key_press_event_t *event) {
  xcb_keycode_t &keycode = event->detail;
  uint16_t &modifiers = event->state;

  for (const ResolvedKeyBind &resolved_bind : resolved_keybinds_) {
    if (resolved_bind.keycode == keycode &&
        resolved_bind.modifiers == modifiers) {
      resolved_bind.handler();
      break;
    }
  }
}

} // namespace wm
