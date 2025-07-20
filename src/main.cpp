#include <iostream>
#include <iterator>

#include <X11/keysym.h>
#include <xcb/xproto.h>

#include "window_manager.hpp"

inline constexpr const char* const term_command[] = { "ghostty", nullptr };

inline constexpr const xcb_mod_mask_t Mod = XCB_MOD_MASK_4;
inline constexpr const xcb_mod_mask_t Shift = XCB_MOD_MASK_SHIFT;
inline constexpr const wm::KeyBind bindings[] = {
    // clang-format off
    { Mod,         XK_Return, [](auto wm) { wm->spawn(term_command); } },
    { Mod | Shift, XK_Q,      [](auto wm) { wm->quit(); } },
    // clang-format on
};

int main(int argc, char* argv[])
{
    if (argc == 2 && std::string_view { argv[1] } == "-v") {
        std::cout << "v0.0.1" << std::endl;
        return EXIT_SUCCESS;
    }

    if (argc > 1) {
        std::cout << "usage: " << argv[0] << " [-v]" << std::endl;
        return EXIT_SUCCESS;
    }

    wm::Config config = {};
    config.debug_events = true;
    config.keybinds = { bindings, std::size(bindings) };

    wm::WindowManager instance(config);

    if (!instance.try_init()) {
        putenv(const_cast<char*>("DISPLAY=:1"));
        if (!instance.try_init())
            return EXIT_FAILURE;
    }

    instance.run();
}
