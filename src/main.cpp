#include <iostream>
#include <iterator>

#include <X11/keysym.h>
#include <xcb/xproto.h>

#include "window_manager.hpp"

inline constexpr const char* const term_command[] = { "ghostty", nullptr };

inline constexpr xcb_mod_mask_t Mod = XCB_MOD_MASK_4;
inline constexpr xcb_mod_mask_t Shift = XCB_MOD_MASK_SHIFT;
inline constexpr wm::key_bind bindings[] = {
    // clang-format off
    { Mod,         XK_Return, [](auto x) { x->Spawn(term_command); } },
    { Mod | Shift, XK_Q,      [](auto x) { x->Quit(); } },
    // clang-format on
};

int main(int argc, char* argv[])
{
    if (argc == 2 && std::string_view { argv[1] } == "-v")
    {
        std::cout << "v0.0.1" << std::endl;
        return EXIT_SUCCESS;
    }

    if (argc > 1)
    {
        std::cout << "usage: " << argv[0] << " [-v]" << std::endl;
        return EXIT_SUCCESS;
    }

    constexpr wm::config Config = {
        .DebugLog = true,
        .Bindings = { bindings, std::size(bindings) },
    };

    wm::window_manager Instance(Config);

    if (!Instance.TryInit())
    {
        putenv(const_cast<char*>("DISPLAY=:1"));
        if (!Instance.TryInit())
            return EXIT_FAILURE;
    }

    Instance.Run();
}
