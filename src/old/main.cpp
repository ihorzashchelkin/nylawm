#include "nylaunity.hpp"

static const char* const term_command[] = { "ghostty", nullptr };

#define ModMask Mod4Mask

static nyla::Keybind keybinds[]{
  {
    ModMask,
    XK_Return,
    [](auto state) { nyla::spawn(state, term_command); },
  },
  {
    ModMask | ShiftMask,
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
#if false
  putenv(const_cast<char*>("DISPLAY=:1"));
#endif

  nyla::State state{};

  if (auto err = nyla::initX11(state, keybinds); err) {
    std::println(std::cerr, "initX11: {}", err);
    return EXIT_FAILURE;
  }

  if (auto err = nyla::initEgl(state); err) {
    std::println(std::cerr, "initEgl: {}", err);
    return EXIT_FAILURE;
  }

  nyla::run(state);
}
