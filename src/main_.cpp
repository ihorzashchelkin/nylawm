#include <GL/glx.h>
#include <cstdlib>

#include "x.hpp"

int main()
{
    auto X11Session = x11_manager {};
    if (!X11Session)
        return EXIT_FAILURE;

    auto GLXWindow = glx_window_manager { X11Session };
}
