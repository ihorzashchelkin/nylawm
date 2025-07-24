#include <cstdlib>

#include <GL/gl.h>
#include <GL/glx.h>

#include "x.hpp"
#include <xcb/xcb.h>

int main()
{
    auto X11Session = x11_conn_manager {};
    if (!X11Session)
        return EXIT_FAILURE;
}
