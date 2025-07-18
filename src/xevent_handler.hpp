#pragma once

#include "conf_types.hpp"
#include <memory>
#include <print>
#include <xcb/xcb.h>

namespace wm {

class XEventHandler {
public:
    void handle_generic(const std::shared_ptr<Configuration> conf, const xcb_generic_event_t* event)
    {
        switch (event->response_type & ~0x80) {
            case XCB_BUTTON_PRESS: {
                if (conf->debug_xevents()) {
                    std::println("XCB button press!");
                }
                handle_button_press(reinterpret_cast<const xcb_button_press_event_t*>(event));
                return;
            }

            case XCB_CLIENT_MESSAGE: {
                if (conf->debug_xevents()) {
                    std::println("XCB client message!");
                }
                handle_client_message(reinterpret_cast<const xcb_client_message_event_t*>(event));
                return;
            }

            case XCB_CONFIGURE_REQUEST: {
                if (conf->debug_xevents()) {
                    std::println("XCB configure request!");
                }
                handle_configure_request(reinterpret_cast<const xcb_configure_request_event_t*>(event));
                return;
            }

            case XCB_DESTROY_NOTIFY: {
                if (conf->debug_xevents()) {
                    std::println("XCB destroy notify!");
                }
                handle_destroy_notify(reinterpret_cast<const xcb_destroy_notify_event_t*>(event));
                return;
            }

            case XCB_ENTER_NOTIFY: {
                if (conf->debug_xevents()) {
                    std::println("XCB enter notify!");
                }
                handle_enter_notify(reinterpret_cast<const xcb_enter_notify_event_t*>(event));
                return;
            }

            case XCB_EXPOSE: {
                if (conf->debug_xevents()) {
                    std::println("XCB expose!");
                }
                handle_expose(reinterpret_cast<const xcb_expose_event_t*>(event));
                return;
            }

            case XCB_FOCUS_IN: {
                if (conf->debug_xevents()) {
                    std::println("XCB focus in!");
                }
                handle_focus_in(reinterpret_cast<const xcb_focus_in_event_t*>(event));
                return;
            }

            case XCB_KEY_PRESS: {
                if (conf->debug_xevents()) {
                    std::println("XCB key press!");
                }
                handle_key_press(reinterpret_cast<const xcb_key_press_event_t*>(event));
                return;
            }

            case XCB_MAPPING_NOTIFY: {
                if (conf->debug_xevents()) {
                    std::println("XCB mapping notify!");
                }
                handle_mapping_notify(reinterpret_cast<const xcb_mapping_notify_event_t*>(event));
                return;
            }

            case XCB_PROPERTY_NOTIFY: {
                if (conf->debug_xevents()) {
                    std::println("XCB property notify!");
                }
                handle_property_notify(reinterpret_cast<const xcb_property_notify_event_t*>(event));
                return;
            }

            case XCB_RESIZE_REQUEST: {
                if (conf->debug_xevents()) {
                    std::println("XCB resize request!");
                }
                handle_resize_request(reinterpret_cast<const xcb_resize_request_event_t*>(event));
                return;
            }

            case XCB_UNMAP_NOTIFY: {
                if (conf->debug_xevents()) {
                    std::println("XCB unmap notify!");
                }
                handle_unmap_notify(reinterpret_cast<const xcb_unmap_notify_event_t*>(event));
                return;
            }
        }
    }

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

}
