#pragma once

#include <span>
#include <vector>

#include <unistd.h>

#include <sys/wait.h>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>

namespace wm {

class WindowManager;

using Action = void (*)(WindowManager*);

struct KeyBind {
    uint16_t modifiers;
    int keysym;
    Action handler;
};

struct ResolvedKeyBind {
    uint16_t modifiers;
    xcb_keycode_t keycode;
    Action handler;
};

struct Config {
    bool debug_events;
    std::span<const KeyBind> keybinds;
};

class WindowManager {
public:
    explicit WindowManager(const Config& conf)
        : m_conf(conf)
        , m_xcb_error_context(nullptr)
    {
    }
    ~WindowManager() { cleanup(); }

    bool try_init();
    void run();
    void quit() { running_ = false; }
    void spawn(const char* const command[]);

private:
    void handle_event(const xcb_generic_event_t* event);
    void handle_button_press(const xcb_button_press_event_t* event);
    void handle_client_message(const xcb_client_message_event_t* event);
    void handle_configure_request(const xcb_configure_request_event_t* event);
    void handle_destroy_notify(const xcb_destroy_notify_event_t* event);
    void handle_enter_notify(const xcb_enter_notify_event_t* event);
    void handle_expose(const xcb_expose_event_t* event);
    void handle_focus_in(const xcb_focus_in_event_t* event);
    void handle_key_press(const xcb_key_press_event_t* event);
    void handle_mapping_notify(const xcb_mapping_notify_event_t* event);
    void handle_map_request(const xcb_map_request_event_t* event);
    void handle_motion_notify(const xcb_motion_notify_event_t* event);
    void handle_property_notify(const xcb_property_notify_event_t* event);
    void handle_resize_request(const xcb_resize_request_event_t* event);
    void handle_unmap_notify(const xcb_unmap_notify_event_t* event);

    void cleanup();
    void prepare_wm_process();
    void prepare_spawn_process();

    bool resolve_keybinds();
    void grab_keys();

    bool running_ = false;

    xcb_ewmh_connection_t m_ewmh;
    xcb_connection_t*& xconn() { return m_ewmh.connection; }

    xcb_screen_t* m_screen;
    const xcb_window_t& root() { return m_screen->root; }

    xcb_errors_context_t* m_xcb_error_context;

    const Config& m_conf;
    std::vector<ResolvedKeyBind> m_resolved_keybinds;
};

}
