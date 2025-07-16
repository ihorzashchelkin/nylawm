#pragma once

#include <csignal>
#include <span>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <xcb/xcb.h>
#include <xcb/xproto.h>

namespace wm {

struct KeyBind {
  uint16_t modifiers;
  int keysym;
  void (*handler)();
};

struct ResolvedKeyBind {
  uint16_t modifiers;
  xcb_keycode_t keycode;
  void (*handler)();
};

class Instance {
public:
  bool try_init();

  void run();
  void quit() { running_ = false; }
  void spawn(const char *const command[]);

private:
  void flush() { xcb_flush(conn_); }

  void prepare_wm_process();
  void prepare_spawn_process();

  void resolve_keybinds();
  void grab_keys();

  void handle_key_press(xcb_key_press_event_t *event);

  bool running_ = false;
  xcb_connection_t *conn_;
  xcb_screen_t *screen_;
  std::vector<ResolvedKeyBind> resolved_keybinds_;
};

Instance *instance();
inline void run() { instance()->run(); }
inline void quit() { instance()->quit(); }
inline void spawn(const char *const command[]) { instance()->spawn(command); }

} // namespace wm
