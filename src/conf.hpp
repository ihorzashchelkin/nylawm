#pragma once

#include <array>

#include <X11/keysym.h>
#include <xcb/xproto.h>

#include "conf_types.hpp"
#include "wm.hpp"

namespace wm {
namespace conf {

    constexpr const char* version = "v0.0.1";
    constexpr const bool debug_xevents = true;

    constexpr const char* display_fallback = ":1";

    constexpr const char* term_command[] = { "ghostty", nullptr };

    constexpr const xcb_mod_mask_t Mod = XCB_MOD_MASK_4;
    constexpr const xcb_mod_mask_t Shift = XCB_MOD_MASK_SHIFT;
    constexpr const auto keybinds = std::to_array<KeyBind>({
        { Mod, XK_Return, [] { wm::spawn(term_command); } },
        { Mod | Shift, XK_Q, [] { wm::quit(); } },
    });

}
}
