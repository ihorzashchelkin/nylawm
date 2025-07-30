#include <array>

#include <X11/keysym.h>
#include <xcb/xproto.h>

#include "nyla.hpp"

inline constexpr const char* const term_command[] = { "ghostty", nullptr };

inline constexpr xcb_mod_mask_t kMod = XCB_MOD_MASK_4;
inline constexpr xcb_mod_mask_t kShift = XCB_MOD_MASK_SHIFT;
inline constexpr auto bindings = std::to_array<const nyla::Keybind>({
  {
    kMod,
    XK_Return,
    [](auto state) { nyla::spawn(state, term_command); },
  },
  {
    kMod | kShift,
    XK_Q,
    [](auto state) { nyla::quit(state); },
  },
  // { kMod,          XK_C,      [](auto x) { x->Kill(); } },
  // { kMod,          XK_U,      [](auto x) { x->SwitchToPrevWorkspace(); } },
  // { kMod,          XK_I,      [](auto x) { x->SwitchToNextWorkspace(); } },
});

int
main(int argc, char* argv[])
{
  putenv(const_cast<char*>("DISPLAY=:1"));

  nyla::State state{};

  nyla::initXcb(state, bindings);
  nyla::initEgl(state);
  nyla::preRun(state);
  nyla::run(state);
}
