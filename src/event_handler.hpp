#pragma once

#include "conf_types.hpp"
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

namespace wm {

class EventHandler {
public:
    void handle_event(xcb_ewmh_connection_t& ewmh, const Configuration& conf, const xcb_generic_event_t* event);

protected:
    virtual void handle_button_press(const xcb_button_press_event_t* event) { };
    virtual void handle_client_message(const xcb_client_message_event_t* event) { };
    virtual void handle_configure_request(const xcb_configure_request_event_t* event) { };
    virtual void handle_destroy_notify(const xcb_destroy_notify_event_t* event) { };
    virtual void handle_enter_notify(const xcb_enter_notify_event_t* event) { };
    virtual void handle_expose(const xcb_expose_event_t* event) { };
    virtual void handle_focus_in(const xcb_focus_in_event_t* event) { };
    virtual void handle_key_press(const xcb_key_press_event_t* event) { };
    virtual void handle_mapping_notify(const xcb_mapping_notify_event_t* event) { };
    virtual void handle_map_request(const xcb_map_request_event_t* event) { };
    virtual void handle_motion_notify(const xcb_motion_notify_event_t* event) { };
    virtual void handle_property_notify(const xcb_property_notify_event_t* event) { };
    virtual void handle_resize_request(const xcb_resize_request_event_t* event) { };
    virtual void handle_unmap_notify(const xcb_unmap_notify_event_t* event) { };
};

namespace ewmh_utils {
    void format_client_message(std::ostream& out, xcb_ewmh_connection_t& ewmh, const xcb_client_message_event_t* event);
}

}
