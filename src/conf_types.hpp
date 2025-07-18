#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include <X11/keysym.h>
#include <xcb/xcb.h>

namespace wm {

class WMActions {
public:
    virtual void quit() { }
    virtual void spawn(const char* const command[]) { }
};

using ActionsConsumer = void (*)(WMActions*);

struct KeyBind {
    uint16_t modifiers;
    int keysym;
    ActionsConsumer handler;
};

struct ResolvedKeyBind {
    uint16_t modifiers;
    xcb_keycode_t keycode;
    ActionsConsumer handler;
};

class Configuration {
public:
    virtual bool debug_xevents() const = 0;
    virtual const char* display_fallback() const = 0;
    virtual std::span<const KeyBind> keybinds() const = 0;
};

}
