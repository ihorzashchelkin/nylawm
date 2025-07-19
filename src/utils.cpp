#include <ostream>
#include <xcb/xcb_errors.h>

#include "utils.hpp"

void wm::utils::format_atom_name(std::ostream& out, xcb_ewmh_connection_t& ewmh, const xcb_atom_t atom)
{
    if (atom) {
        xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(ewmh.connection, atom);
        xcb_get_atom_name_reply_t* reply = xcb_get_atom_name_reply(ewmh.connection, cookie, nullptr);
        out << std::string_view { xcb_get_atom_name_name(reply), size_t(xcb_get_atom_name_name_length(reply)) };
        free(reply);
    } else {
        out << "None";
    }
}

void wm::utils::format_xcb_error(std::ostream& out, xcb_errors_context_t* error_context, const xcb_generic_error_t* error)
{
    out << "error: " << int(error->error_code) << " Bad" << xcb_errors_get_name_for_error(error_context, error->error_code, nullptr);
}

