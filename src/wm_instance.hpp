#pragma once

#include <memory>
#include <vector>

#include <unistd.h>

#include <sys/wait.h>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xproto.h>

#include "conf_types.hpp"
#include "xevent_handler.hpp"

namespace wm {

class WMInstance : public XEventHandler, public WMActions {
public:
    explicit WMInstance(const std::shared_ptr<Configuration> conf)
        : conf_(conf)
    {
    }
    virtual ~WMInstance() = default;

    bool try_init();

    void run();
    void quit() override { running_ = false; }
    void spawn(const char* const command[]) override;

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

    bool running_ = false;
    const std::shared_ptr<Configuration> conf_;
    xcb_errors_context_t* error_context = nullptr;
    xcb_connection_t* conn_;
    xcb_screen_t* screen_;
    std::vector<ResolvedKeyBind> resolved_keybinds_;
};

} // namespace wm
