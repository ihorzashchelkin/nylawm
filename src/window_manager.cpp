#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <experimental/scope>
#include <iostream>

#include <fcntl.h>
#include <iterator>
#include <print>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "utils.hpp"
#include "window_manager.hpp"

void wm::WindowManager::cleanup()
{
    if (m_xcb_error_context)
        xcb_errors_context_free(m_xcb_error_context);

    if (xconn()) {
        xcb_disconnect(xconn());
        xcb_ewmh_connection_wipe(&m_ewmh);
        memset(&m_ewmh, 0, sizeof(m_ewmh));
    }
}

bool wm::WindowManager::try_init()
{
    int screen_n;
    xconn() = xcb_connect(nullptr, &screen_n);
    if (xcb_connection_has_error(xconn())) {
        std::cerr << "could not connect to X server" << std::endl;
        return false;
    }

    if (xcb_errors_context_new(xconn(), &m_xcb_error_context)) {
        std::cerr << "could not create error context" << std::endl;
        return false;
    }

    const xcb_setup_t* setup = xcb_get_setup(xconn());
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

    m_screen = nullptr;
    for (int i = 0; screen_iterator.rem && i <= screen_n;
        xcb_screen_next(&screen_iterator), i++) {
        if (i == screen_n)
            m_screen = screen_iterator.data;
    }

    if (!m_screen) {
        std::cerr << "could not find screen: " << screen_n << std::endl;
        return false;
    }

    static constexpr uint32_t root_mask = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT // catch MapRequest, ConfigureRequest, CirculateRequest
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY // see MapNotify, UnmapNotify, ReparentNotify
        | XCB_EVENT_MASK_PROPERTY_CHANGE // track WM_PROTOCOLS, _NET_WM_STATE, _NET_CLIENT_LIST…
        | XCB_EVENT_MASK_BUTTON_PRESS // click-to-focus / root menu
        | XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_POINTER_MOTION // for drag-to-move/resize
        | XCB_EVENT_MASK_KEY_PRESS // global keybindings
        | XCB_EVENT_MASK_KEY_RELEASE
        | XCB_EVENT_MASK_EXPOSURE; // redraw desktop background if you draw one

    xcb_void_cookie_t window_attribute_change_cookie = xcb_change_window_attributes_checked(xconn(), root(), XCB_CW_EVENT_MASK, &root_mask);

    xcb_generic_error_t* error = xcb_request_check(xconn(), window_attribute_change_cookie);
    if (error) {
        wm::utils::format_xcb_error(std::cerr, m_xcb_error_context, error);
        std::cerr << std::endl;

        if (error->error_code == 10) // Bad Access
            std::cerr << "is another wm already running?" << std::endl;

        return false;
    }

    xcb_intern_atom_cookie_t* intern_atoms_cookies = xcb_ewmh_init_atoms(m_ewmh.connection, &m_ewmh);
    if (!xcb_ewmh_init_atoms_replies(&m_ewmh, intern_atoms_cookies, (xcb_generic_error_t**)0)) {
        std::cerr << "could not intern ewmh atoms" << std::endl;
        return false;
    }

    static xcb_atom_t supported[] = {
        m_ewmh._NET_WM_STATE_MODAL,
        m_ewmh._NET_WM_STATE_STICKY,
        m_ewmh._NET_WM_STATE_MAXIMIZED_VERT,
        m_ewmh._NET_WM_STATE_MAXIMIZED_HORZ,
        m_ewmh._NET_WM_STATE_SHADED,
        m_ewmh._NET_WM_STATE_SKIP_TASKBAR,
        m_ewmh._NET_WM_STATE_SKIP_PAGER,
        m_ewmh._NET_WM_STATE_HIDDEN,
        m_ewmh._NET_WM_STATE_FULLSCREEN,
        m_ewmh._NET_WM_STATE_ABOVE,
        m_ewmh._NET_WM_STATE_BELOW,
        m_ewmh._NET_WM_STATE_DEMANDS_ATTENTION,
    };
    xcb_ewmh_set_supported(&m_ewmh, screen_n, std::size(supported), supported);

    return true;
}

bool wm::WindowManager::resolve_keybinds()
{
    xcb_key_symbols_t* syms = xcb_key_symbols_alloc(xconn());
    std::experimental::scope_exit free_syms([&] { free(syms); });

    for (const auto& bind : m_conf.keybinds) {
        xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(syms, bind.keysym);
        if (!keycodes) {
            return false;
        }
        std::experimental::scope_exit free_codes([=] { free(keycodes); });
        while (*keycodes != XCB_NO_SYMBOL) {
            m_resolved_keybinds.emplace_back(ResolvedKeyBind { bind.modifiers, *keycodes, bind.handler });
            keycodes++;
        }
    }

    return true;
}

void wm::WindowManager::grab_keys()
{
    for (const auto& resolved_bind : m_resolved_keybinds) {
        xcb_grab_key(xconn(), 0, root(), resolved_bind.modifiers, resolved_bind.keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }
}

void wm::WindowManager::run()
{
    prepare_wm_process();

    if (!resolve_keybinds()) {
        std::cout << "could not resolve all keybinds" << std::endl;
    }

    grab_keys();

    running_ = true;
    while (running_) {
        xcb_flush(xconn());
        xcb_generic_event_t* event = xcb_wait_for_event(xconn());

        if (event) {
            handle_event(event);
            free(event);
        } else {
            std::cerr << "lost connection to X server" << std::endl;
            running_ = false;
        }
    }
}

void wm::WindowManager::spawn(const char* const command[])
{
    if (fork() == 0) {
        prepare_spawn_process();
        execvp(command[0], const_cast<char**>(command));
        std::cerr << "execvp failed" << std::endl;
        std::exit(1);
    }
}

void wm::WindowManager::prepare_wm_process()
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, nullptr);

    while (waitpid(-1, nullptr, WNOHANG) > 0)
        ;
}

void wm::WindowManager::prepare_spawn_process()
{
    close(xcb_get_file_descriptor(xconn()));

    int dev_null_fd = open("/dev/null", O_RDONLY);
    dup2(dev_null_fd, STDIN_FILENO);
    close(dev_null_fd);

    dev_null_fd = open("/dev/null", O_WRONLY);
    dup2(dev_null_fd, STDOUT_FILENO);
    dup2(dev_null_fd, STDERR_FILENO);
    close(dev_null_fd);

    setsid();

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &sa, nullptr);
}

void wm::WindowManager::handle_button_press(const xcb_button_press_event_t* event) { }

void wm::WindowManager::handle_client_message(const xcb_client_message_event_t* event)
{
    if (event->type == m_ewmh._NET_WM_STATE) {
        switch (event->data.data32[0]) {
            case XCB_EWMH_WM_STATE_REMOVE: {
                {
                    static xcb_atom_t atom = m_ewmh._NET_WM_STATE_HIDDEN;
                    xcb_void_cookie_t cookie = xcb_change_property_checked(xconn(), XCB_PROP_MODE_REPLACE, event->window, m_ewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1, &atom);
                    xcb_request_check(xconn(), cookie);
                }

                xcb_get_property_cookie_t cookie = xcb_get_property(xconn(), 0, event->window, m_ewmh._NET_WM_STATE, XCB_ATOM_ATOM, 0, 64);
                xcb_get_property_reply_t* reply = xcb_get_property_reply(xconn(), cookie, nullptr);

                if (reply) {
                    int len = xcb_get_property_value_length(reply);
                    if (!len) {
                    }

                    void* value = xcb_get_property_value(reply);

                    xcb_change_property(xconn(), XCB_PROP_MODE_REPLACE, event->window, m_ewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 0, nullptr);
                }

                break;
            }

            case XCB_EWMH_WM_STATE_ADD: {
                break;
            }

            case XCB_EWMH_WM_STATE_TOGGLE: {
                break;
            }
        }
    }
}

void wm::WindowManager::handle_configure_request(const xcb_configure_request_event_t* event)
{
    constexpr const uint16_t mask = (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
    constexpr const uint32_t values[] = { 0, 0, 1024, 768 };
    xcb_configure_window(xconn(), event->window, mask, values);
}

void wm::WindowManager::handle_destroy_notify(const xcb_destroy_notify_event_t* event) { }
void wm::WindowManager::handle_enter_notify(const xcb_enter_notify_event_t* event) { }
void wm::WindowManager::handle_expose(const xcb_expose_event_t* event) { }
void wm::WindowManager::handle_focus_in(const xcb_focus_in_event_t* event) { }

void wm::WindowManager::handle_key_press(const xcb_key_press_event_t* event)
{
    auto& keycode = event->detail;
    auto& modifiers = event->state;

    for (const auto& resolved_bind : m_resolved_keybinds) {
        if (resolved_bind.keycode == keycode && resolved_bind.modifiers == modifiers) {
            resolved_bind.handler(this);
            break;
        }
    }
}

void wm::WindowManager::handle_mapping_notify(const xcb_mapping_notify_event_t* event) { }
void wm::WindowManager::handle_map_request(const xcb_map_request_event_t* event) { }
void wm::WindowManager::handle_motion_notify(const xcb_motion_notify_event_t* event) { }
void wm::WindowManager::handle_property_notify(const xcb_property_notify_event_t* event) { }
void wm::WindowManager::handle_resize_request(const xcb_resize_request_event_t* event) { }
void wm::WindowManager::handle_unmap_notify(const xcb_unmap_notify_event_t* event) { }

void wm::WindowManager::handle_event(const xcb_generic_event_t* event)
{
    switch (event->response_type & ~0x80) {
        case XCB_BUTTON_PRESS:
            handle_button_press(reinterpret_cast<const xcb_button_press_event_t*>(event));
            break;
        case XCB_CLIENT_MESSAGE:
            handle_client_message(reinterpret_cast<const xcb_client_message_event_t*>(event));
            break;
        case XCB_CONFIGURE_REQUEST:
            handle_configure_request(reinterpret_cast<const xcb_configure_request_event_t*>(event));
            break;
        case XCB_DESTROY_NOTIFY:
            handle_destroy_notify(reinterpret_cast<const xcb_destroy_notify_event_t*>(event));
            break;
        case XCB_ENTER_NOTIFY:
            handle_enter_notify(reinterpret_cast<const xcb_enter_notify_event_t*>(event));
            break;
        case XCB_EXPOSE:
            handle_expose(reinterpret_cast<const xcb_expose_event_t*>(event));
            break;
        case XCB_FOCUS_IN:
            handle_focus_in(reinterpret_cast<const xcb_focus_in_event_t*>(event));
            break;
        case XCB_KEY_PRESS:
            handle_key_press(reinterpret_cast<const xcb_key_press_event_t*>(event));
            break;
        case XCB_MAPPING_NOTIFY:
            handle_mapping_notify(reinterpret_cast<const xcb_mapping_notify_event_t*>(event));
            break;
        case XCB_PROPERTY_NOTIFY:
            handle_property_notify(reinterpret_cast<const xcb_property_notify_event_t*>(event));
            break;
        case XCB_RESIZE_REQUEST:
            handle_resize_request(reinterpret_cast<const xcb_resize_request_event_t*>(event));
            break;
        case XCB_UNMAP_NOTIFY:
            handle_unmap_notify(reinterpret_cast<const xcb_unmap_notify_event_t*>(event));
            break;
    }
}
