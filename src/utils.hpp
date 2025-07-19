#pragma once

#include <ostream>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_ewmh.h>

namespace wm::utils {

void format_atom_name(std::ostream& out, xcb_ewmh_connection_t& ewmh_connection, const xcb_atom_t atom);
void format_xcb_error(std::ostream& out, xcb_errors_context_t* error_context, const xcb_generic_error_t* error);

}
