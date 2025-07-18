#pragma once

#include <cstdint>

namespace wm {
namespace conf {

    struct KeyBind {
        uint16_t modifiers;
        int keysym;
        void (*handler)();
    };

}
}
