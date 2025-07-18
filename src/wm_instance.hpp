#pragma once

#include <vector>

#include <unistd.h>

#include <sys/wait.h>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xproto.h>

#include "xevent_handler.hpp"

namespace wm {

struct ResolvedKeyBind {
    uint16_t modifiers;
    xcb_keycode_t keycode;
    void (*handler)();
};

class Instance : public XEventHandler {
public:
    bool try_init();

    void run();
    void quit() { running_ = false; }
    void spawn(const char* const command[]);

    // void handle_button_press(const xcb_button_press_event_t* event) override;
    // void handle_client_message(const xcb_client_message_event_t* event) override;
    void handle_configure_request(const xcb_configure_request_event_t* event) override;
    // void handle_destroy_notify(const xcb_destroy_notify_event_t* event) override;
    // void handle_enter_notify(const xcb_enter_notify_event_t* event) override;
    // void handle_expose(const xcb_expose_event_t* event) override;
    // void handle_focus_in(const xcb_focus_in_event_t* event) override;
    void handle_key_press(const xcb_key_press_event_t* event) override;
    // void handle_mapping_notify(const xcb_mapping_notify_event_t* event) override;
    // void handle_map_request(const xcb_map_request_event_t* event) override;
    // void handle_motion_notify(const xcb_motion_notify_event_t* event) override;
    // void handle_property_notify(const xcb_property_notify_event_t* event) override;
    // void handle_resize_request(const xcb_resize_request_event_t* event) override;
    // void handle_unmap_notify(const xcb_unmap_notify_event_t* event) override;

private:
    void log_xcb_error(const xcb_generic_error_t* error);

    void prepare_wm_process();
    void prepare_spawn_process();

    void resolve_keybinds();
    void grab_keys();

    xcb_errors_context_t* error_context = nullptr;
    bool running_ = false;
    xcb_connection_t* conn_;
    xcb_screen_t* screen_;
    std::vector<ResolvedKeyBind> resolved_keybinds_;

    virtual ~Instance() = default;
};

} // namespace wm
