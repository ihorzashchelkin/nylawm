#pragma once

#include <ostream>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

namespace wm::utils {

void format_atom_name(std::ostream& out, xcb_ewmh_connection_t& ewmh, const xcb_atom_t atom);

}
