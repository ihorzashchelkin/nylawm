#include <print>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include "conf_types.hpp"
#include "event_handler.hpp"

namespace wm {

void EventHandler::handle_event(xcb_ewmh_connection_t& ewmh, const Configuration& conf, const xcb_generic_event_t* event)
{
    switch (event->response_type & ~0x80) {
        case XCB_BUTTON_PRESS: {
            if (conf.debug_events()) {
                std::println("XCB button press!");
            }
            handle_button_press(reinterpret_cast<const xcb_button_press_event_t*>(event));
            return;
        }

        case XCB_CLIENT_MESSAGE: {
            handle_client_message(reinterpret_cast<const xcb_client_message_event_t*>(event));
            return;
        }

        case XCB_CONFIGURE_REQUEST: {
            if (conf.debug_events()) {
                std::println("XCB configure request!");
            }
            handle_configure_request(reinterpret_cast<const xcb_configure_request_event_t*>(event));
            return;
        }

        case XCB_DESTROY_NOTIFY: {
            if (conf.debug_events()) {
                std::println("XCB destroy notify!");
            }
            handle_destroy_notify(reinterpret_cast<const xcb_destroy_notify_event_t*>(event));
            return;
        }

        case XCB_ENTER_NOTIFY: {
            if (conf.debug_events()) {
                std::println("XCB enter notify!");
            }
            handle_enter_notify(reinterpret_cast<const xcb_enter_notify_event_t*>(event));
            return;
        }

        case XCB_EXPOSE: {
            if (conf.debug_events()) {
                std::println("XCB expose!");
            }
            handle_expose(reinterpret_cast<const xcb_expose_event_t*>(event));
            return;
        }

        case XCB_FOCUS_IN: {
            if (conf.debug_events()) {
                std::println("XCB focus in!");
            }
            handle_focus_in(reinterpret_cast<const xcb_focus_in_event_t*>(event));
            return;
        }

        case XCB_KEY_PRESS: {
            if (conf.debug_events()) {
                std::println("XCB key press!");
            }
            handle_key_press(reinterpret_cast<const xcb_key_press_event_t*>(event));
            return;
        }

        case XCB_MAPPING_NOTIFY: {
            if (conf.debug_events()) {
                std::println("XCB mapping notify!");
            }
            handle_mapping_notify(reinterpret_cast<const xcb_mapping_notify_event_t*>(event));
            return;
        }

        case XCB_PROPERTY_NOTIFY: {
            if (conf.debug_events()) {
                std::println("XCB property notify!");
            }
            handle_property_notify(reinterpret_cast<const xcb_property_notify_event_t*>(event));
            return;
        }

        case XCB_RESIZE_REQUEST: {
            if (conf.debug_events()) {
                std::println("XCB resize request!");
            }
            handle_resize_request(reinterpret_cast<const xcb_resize_request_event_t*>(event));
            return;
        }

        case XCB_UNMAP_NOTIFY: {
            if (conf.debug_events()) {
                std::println("XCB unmap notify!");
            }
            handle_unmap_notify(reinterpret_cast<const xcb_unmap_notify_event_t*>(event));
            return;
        }
    }
}

}
