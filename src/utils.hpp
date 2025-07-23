#pragma once

#include <ostream>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_ewmh.h>

namespace wm::utils {

void PrintAtomName(std::ostream& out, xcb_ewmh_connection_t& ewmh_connection, const xcb_atom_t atom);
void PrintXCBError(std::ostream& out, xcb_errors_context_t* error_context, const xcb_generic_error_t* error);

bool PropertyHasAtom(xcb_connection_t* Conn, xcb_window_t Win, xcb_atom_t Property, xcb_atom_t Val);
void PropertyRemoveAtom(xcb_connection_t* Conn, xcb_window_t Win, xcb_atom_t Property, xcb_atom_t Val);
void PropertyAddAtom(xcb_connection_t* Conn, xcb_window_t Win, xcb_atom_t Property, xcb_atom_t Val);
void PropertyToggleAtom(xcb_connection_t* Conn, xcb_window_t Win, xcb_atom_t Property, xcb_atom_t Val);

}
