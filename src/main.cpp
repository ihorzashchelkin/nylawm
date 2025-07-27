#include <array>
#include <cstdlib>

#include "internal.hpp"

#include <X11/keysym.h>
#include <xcb/xproto.h>

#include "handlers.cpp"
#include "window_manager.cpp"

inline constexpr const char* const term_command[] = { "ghostty", nullptr };

inline constexpr xcb_mod_mask_t kMod = XCB_MOD_MASK_4;
inline constexpr xcb_mod_mask_t kShift = XCB_MOD_MASK_SHIFT;
inline constexpr auto bindings = std::to_array<const cirnowm::Keybind>({
  // clang-format off
  { kMod,          XK_Return, [](auto x) { x->Spawn(term_command); } },
  { kMod | kShift, XK_Q,      [](auto x) { x->Quit(); } },
  // { kMod,          XK_C,      [](auto x) { x->Kill(); } },
  // { kMod,          XK_U,      [](auto x) { x->SwitchToPrevWorkspace(); } },
  // { kMod,          XK_I,      [](auto x) { x->SwitchToNextWorkspace(); } },
  // clang-format on
});

int
main(int argc, char* argv[])
{
  putenv(const_cast<char*>("DISPLAY=:1"));

  cirnowm::WindowManager wm{ bindings };
  if (!wm)
    return EXIT_FAILURE;

  wm.SetDebug(true);

  wm.Run();
}
