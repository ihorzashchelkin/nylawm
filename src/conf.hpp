#pragma once

#include <array>

#include <X11/keysym.h>
#include <xcb/xproto.h>

#include "conf_types.hpp"

class MyConfiguration : public wm::Configuration {
public:
    virtual const char* display_fallback() const { return ":1"; }
    virtual bool debug_xevents() const { return true; }

    virtual std::span<const wm::KeyBind> keybinds() const
    {
        static constexpr xcb_mod_mask_t Mod = XCB_MOD_MASK_4;
        static constexpr xcb_mod_mask_t Shift = XCB_MOD_MASK_SHIFT;

        static const wm::KeyBind bindings[] = {
            { Mod, XK_Return, [](auto actions) { actions->spawn((const char* const[]) { "ghostty", nullptr }); } },
            { Mod | Shift, XK_Q, [](auto actions) { actions->quit(); } },
        };

        return std::span<const wm::KeyBind> { bindings, std::size(bindings) };
    }
};
