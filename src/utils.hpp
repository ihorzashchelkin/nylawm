#pragma once

#include <ostream>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>

namespace wm::utils {

// TODO: implement template and use std::format instead
void FormatAtomName(std::ostream& out, xcb_ewmh_connection_t& ewmh_connection, const xcb_atom_t atom);
void FormatXCBError(std::ostream& out, xcb_errors_context_t* error_context, const xcb_generic_error_t* error);

}
