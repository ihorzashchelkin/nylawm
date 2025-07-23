#include <cassert>
#include <cstdlib>
#include <ostream>
#include <print>

#include <xcb/xcb_errors.h>

#include "utils.hpp"

namespace wm::utils {

void PrintAtomName(std::ostream& out, xcb_ewmh_connection_t& ewmh, const xcb_atom_t atom)
{
    if (!atom)
    {
        std::println(out, "None");
        return;
    }

    auto  cookie = xcb_get_atom_name(ewmh.connection, atom);
    auto* reply  = xcb_get_atom_name_reply(ewmh.connection, cookie, nullptr);
    out << std::string_view { xcb_get_atom_name_name(reply), size_t(xcb_get_atom_name_name_length(reply)) };
    free(reply);
}

void PrintXCBError(std::ostream& out, xcb_errors_context_t* error_context, const xcb_generic_error_t* error)
{
    std::println(out, "error(code={}): Bad{}", int(error->error_code), xcb_errors_get_name_for_error(error_context, error->error_code, nullptr));
}

void PropertyRemoveAtom(xcb_connection_t* Conn, xcb_window_t Win, xcb_atom_t Property, xcb_atom_t Val)
{
    auto  Cookie = xcb_get_property(Conn, 0, Win, Property, XCB_ATOM_ATOM, 0, std::numeric_limits<uint32_t>::max());
    auto* Reply  = xcb_get_property_reply(Conn, Cookie, nullptr);

    int  n       = xcb_get_property_value_length(Reply);
    auto OldVals = static_cast<xcb_atom_t*>(xcb_get_property_value(Reply));

    assert(n <= 64);
    xcb_atom_t NewVals[64];

    int c = 0;
    for (int i = 0; i < n; ++i)
    {
        if (OldVals[i] != Val)
            NewVals[c++] = OldVals[i];
    }

    xcb_change_property(Conn, XCB_PROP_MODE_REPLACE, Win, Property, XCB_ATOM_ATOM, 32, c, NewVals);
    xcb_flush(Conn);
    free(Reply);
}

void PropertyAddAtom(xcb_connection_t* Conn, xcb_window_t Win, xcb_atom_t Property, xcb_atom_t Val)
{
    xcb_change_property(Conn, XCB_PROP_MODE_APPEND, Win, Property, XCB_ATOM_ATOM, 32, 1, &Val);
    xcb_flush(Conn);
}

bool PropertyHasAtom(xcb_connection_t* Conn, xcb_window_t Win, xcb_atom_t Property, xcb_atom_t Val)
{
    auto  Cookie = xcb_get_property(Conn, 0, Win, Property, XCB_ATOM_ATOM, 0, std::numeric_limits<uint32_t>::max());
    auto* Reply  = xcb_get_property_reply(Conn, Cookie, nullptr);

    int  n       = xcb_get_property_value_length(Reply);
    auto OldVals = static_cast<xcb_atom_t*>(xcb_get_property_value(Reply));

    for (int i = 0; i < n; ++i)
    {
        if (OldVals[i] == Val)
        {
            free(Reply);
            return true;
        }
    }

    free(Reply);
    return false;
}

void PropertyToggleAtom(xcb_connection_t* Conn, xcb_window_t Win, xcb_atom_t Property, xcb_atom_t Val)
{
    if (PropertyHasAtom(Conn, Win, Property, Val))
        PropertyRemoveAtom(Conn, Win, Property, Val);
    else
        PropertyAddAtom(Conn, Win, Property, Val);
}

}
