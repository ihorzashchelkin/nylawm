#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <experimental/scope>
#include <iostream>

#include <fcntl.h>
#include <iterator>
#include <limits>
#include <print>
#include <string_view>
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

    for (const auto& bind : m_config.keybinds) {
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
    {
        xcb_query_tree_cookie_t cookie = xcb_query_tree(xconn(), root());
        xcb_query_tree_reply_t* reply = xcb_query_tree_reply(xconn(), cookie, nullptr);

        xcb_window_t* children = xcb_query_tree_children(reply);

        for (int i = 0; i < xcb_query_tree_children_length(reply); i++) {
            auto& child = children[i];

            xcb_reparent_window(xconn(),
                child,
                root(),
                0,
                0);
            xcb_flush(xconn());

            xcb_get_property_cookie_t cookie = xcb_get_property(xconn(),
                0,
                child,
                m_ewmh._NET_WM_NAME,
                m_ewmh.UTF8_STRING,
                0,
                std::numeric_limits<uint32_t>::max());

            xcb_get_property_reply_t* reply = xcb_get_property_reply(xconn(), cookie, nullptr);
            if (reply) {
                int len = xcb_get_property_value_length(reply);
                const std::string_view str { (char*)xcb_get_property_value(reply), size_t(len) };
                if (!str.empty())
                    std::cout << str << std::endl;
            }
        }

        free(reply);
    }

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
    std::cout << "ClientMessage from " << event->window << std::endl;

    if (event->type == m_ewmh._NET_WM_STATE) {
        auto& verb = event->data.data32[0];
        auto& arg1 = event->data.data32[1];
        auto& arg2 = event->data.data32[2];

        if (arg1 == m_ewmh._NET_WM_STATE_HIDDEN && verb == XCB_EWMH_WM_STATE_REMOVE) {
            // uint32_t stack_vals[] = { XCB_STACK_MODE_ABOVE };
            // xcb_configure_window(xconn(),
            //     event->window,
            //     XCB_CONFIG_WINDOW_STACK_MODE,
            //     stack_vals);
            //

            xcb_map_window(xconn(), event->window);

            // xcb_set_input_focus(xconn(),
            //     XCB_INPUT_FOCUS_POINTER_ROOT,
            //     event->window,
            //     XCB_CURRENT_TIME);

            // xcb_change_property(xconn(),
            //     XCB_PROP_MODE_REPLACE,
            //     root(),
            //     m_ewmh._NET_ACTIVE_WINDOW,
            //     XCB_ATOM_WINDOW,
            //     32,
            //     1,
            //     &event->window);
        }

#if 0
        switch (event->data.data32[0]) {
            case XCB_EWMH_WM_STATE_REMOVE: {
                xcb_get_property_cookie_t cookie = xcb_get_property(xconn(), 0, event->window, m_ewmh._NET_WM_STATE, XCB_ATOM_ATOM, 0, std::numeric_limits<uint32_t>::max());
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
#endif
    }
}

void wm::WindowManager::handle_error(const xcb_generic_error_t* error)
{
    utils::format_xcb_error(std::cerr, m_xcb_error_context, error);
    std::cerr << std::endl;
}

void wm::WindowManager::handle_configure_request(const xcb_configure_request_event_t* event)
{
    std::cout << "ConfigureRequest from " << event->window << std::endl;

    uint32_t mask = 0;
    uint32_t values[7];
    int c = 0;

#define COPY_MASK_MEMBER(mask_member, event_member) \
    do {                                            \
        if (event->value_mask & mask_member) {      \
            mask |= mask_member;                    \
            values[c++] = event->event_member;      \
        }                                           \
    } while (0)

    static int i = -1;
    i++;

#if true
    COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_X, x);
    COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_Y, y);
    COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_WIDTH, width);
    COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_HEIGHT, height);
#else // DOESNT WORK
    mask |= XCB_CONFIG_WINDOW_X;
    values[c++] = m_screen->width_in_pixels;
    mask |= XCB_CONFIG_WINDOW_Y;
    values[c++] = m_screen->height_in_pixels;
    mask |= XCB_CONFIG_WINDOW_WIDTH;
    values[c++] = m_screen->width_in_pixels;
    mask |= XCB_CONFIG_WINDOW_HEIGHT;
    values[c++] = m_screen->height_in_pixels;
#endif

    // TODO: apparently have to do honor first ConfigureRequest as-is otherwise some clients won't show up

    COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_BORDER_WIDTH, border_width);
    COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_SIBLING, sibling);
    COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_STACK_MODE, stack_mode);

    xcb_configure_window(xconn(), event->window, mask, values);
    xcb_flush(xconn());

    xcb_configure_notify_event_t notify_event = {};
    notify_event.response_type = XCB_CONFIGURE_NOTIFY;
    notify_event.event = event->window;
    notify_event.window = event->window;
    notify_event.x = 0;
    notify_event.y = 0;
    notify_event.width = m_screen->width_in_pixels;
    notify_event.height = m_screen->height_in_pixels;
    notify_event.border_width = event->border_width;
    notify_event.above_sibling = XCB_NONE;
    notify_event.override_redirect = 0;

    xcb_send_event(xconn(),
        0,
        event->window,
        XCB_EVENT_MASK_STRUCTURE_NOTIFY,
        (char*)&notify_event);
    xcb_flush(xconn());
}

void wm::WindowManager::handle_destroy_notify(const xcb_destroy_notify_event_t* event) { }
void wm::WindowManager::handle_enter_notify(const xcb_enter_notify_event_t* event) { }
void wm::WindowManager::handle_expose(const xcb_expose_event_t* event)
{
    std::cout << "Expose from " << event->window << std::endl;
}

void wm::WindowManager::handle_focus_in(const xcb_focus_in_event_t* event)
{
    std::cout << "FocusIn" << std::endl;
}

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

void wm::WindowManager::handle_mapping_notify(const xcb_mapping_notify_event_t* event)
{
    std::cout << "MappingNotify" << std::endl;
}

void wm::WindowManager::handle_map_request(const xcb_map_request_event_t* event)
{
    std::cout << "MapRequest from " << event->window << std::endl;
}

void wm::WindowManager::handle_motion_notify(const xcb_motion_notify_event_t* event) { }

void wm::WindowManager::handle_property_notify(const xcb_property_notify_event_t* event)
{
    std::cout << "PropertyNotify from " << event->window << std::endl;
}

void wm::WindowManager::handle_resize_request(const xcb_resize_request_event_t* event)
{
    std::cout << "ResizeRequest from " << event->window << std::endl;
}

void wm::WindowManager::handle_unmap_notify(const xcb_unmap_notify_event_t* event) { }

void wm::WindowManager::handle_event(const xcb_generic_event_t* event)
{
    switch (event->response_type & ~0x80) {
        case 0:
            handle_error(reinterpret_cast<const xcb_generic_error_t*>(event));
            break;
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
