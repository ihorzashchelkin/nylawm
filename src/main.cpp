#include "nyla.hpp"

#include <X11/keysym.h>
#include <xcb/xproto.h>

#include <iostream>
#include <print>

inline const char* const term_command[] = { "ghostty", nullptr };

#define MOD XCB_MOD_MASK_4

static nyla::Keybind keybinds[]{
  {
    MOD,
    XK_Return,
    [](auto state) { nyla::spawn(state, term_command); },
  },
  {
    MOD | XCB_MOD_MASK_SHIFT,
    XK_Q,
    [](auto state) { nyla::quit(state); },
  },
  // { kMod,          XK_C,      [](auto x) { x->Kill(); } },
  // { kMod,          XK_U,      [](auto x) { x->SwitchToPrevWorkspace(); } },
  // { kMod,          XK_I,      [](auto x) { x->SwitchToNextWorkspace(); } },
};

int
main(int argc, char* argv[])
{
  putenv(const_cast<char*>("DISPLAY=:1"));

  nyla::State state{};

  if (auto err = nyla::initXcb(state, keybinds); err) {
    std::println(std::cerr, "initXcb: {}", err);
    return 1;
  }

  if (auto err = nyla::initEgl(state); err) {
    std::println(std::cerr, "initEgl: {}", err);
    return 1;
  }

  nyla::run(state);
}
