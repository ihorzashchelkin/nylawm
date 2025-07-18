#include <cstdint>
#include <cstdlib>
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

namespace wm {

bool WindowManager::try_init()
{
    int screen_n;
    ewmh.connection = xcb_connect(nullptr, &screen_n);
    if (xcb_connection_has_error(ewmh.connection)) {
        std::cerr << "could not connect to X server" << std::endl;
        return false;
    }

    {
        xcb_intern_atom_cookie_t* cookie = xcb_ewmh_init_atoms(ewmh.connection, &ewmh);
        xcb_ewmh_init_atoms_replies(&ewmh, cookie, (xcb_generic_error_t**)0);
    }

    const xcb_setup_t* setup = xcb_get_setup(ewmh.connection);
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

    screen_ = nullptr;
    for (int i = 0; screen_iterator.rem && i <= screen_n;
        xcb_screen_next(&screen_iterator), i++) {
        if (i == screen_n)
            screen_ = screen_iterator.data;
    }

    if (!screen_) {
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

    xcb_generic_error_t* error = xcb_request_check(ewmh.connection, xcb_change_window_attributes_checked(ewmh.connection, screen_->root, XCB_CW_EVENT_MASK, &root_mask));
    if (error) {
        log_xcb_error(error);
        if (error->error_code == 10) // Bad Access
            std::cerr << "is another wm already running?" << std::endl;

        return false;
    }

    static xcb_atom_t supported[] = {
        ewmh._NET_WM_STATE_MODAL,
        ewmh._NET_WM_STATE_STICKY,
        ewmh._NET_WM_STATE_MAXIMIZED_VERT,
        ewmh._NET_WM_STATE_MAXIMIZED_HORZ,
        ewmh._NET_WM_STATE_SHADED,
        ewmh._NET_WM_STATE_SKIP_TASKBAR,
        ewmh._NET_WM_STATE_SKIP_PAGER,
        ewmh._NET_WM_STATE_HIDDEN,
        ewmh._NET_WM_STATE_FULLSCREEN,
        ewmh._NET_WM_STATE_ABOVE,
        ewmh._NET_WM_STATE_BELOW,
        ewmh._NET_WM_STATE_DEMANDS_ATTENTION,
    };
    xcb_ewmh_set_supported(&ewmh, screen_n, std::size(supported), supported);

    return true;
}

bool WindowManager::resolve_keybinds()
{
    xcb_key_symbols_t* syms = xcb_key_symbols_alloc(ewmh.connection);
    std::experimental::scope_exit free_syms([&] { free(syms); });

    for (const auto& bind : conf_.keybinds()) {
        xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(syms, bind.keysym);
        if (!keycodes) {
            return false;
        }
        std::experimental::scope_exit free_codes([=] { free(keycodes); });
        while (*keycodes != XCB_NO_SYMBOL) {
            resolved_keybinds_.emplace_back(ResolvedKeyBind { bind.modifiers, *keycodes, bind.handler });
            keycodes++;
        }
    }

    return true;
}

void WindowManager::grab_keys()
{
    for (const auto& resolved_bind : resolved_keybinds_) {
        xcb_grab_key(ewmh.connection, 0, screen_->root, resolved_bind.modifiers, resolved_bind.keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }
}

void WindowManager::run()
{
    prepare_wm_process();

    if (!resolve_keybinds()) {
        std::cout << "could not resolve all keybinds" << std::endl;
    }

    grab_keys();

    running_ = true;
    while (running_) {
        xcb_flush(ewmh.connection);
        xcb_generic_event_t* event = xcb_wait_for_event(ewmh.connection);

        if (event) {
            std::experimental::scope_exit free_event([&] { free(event); });
            handle_event(ewmh, conf_, event);
        } else {
            std::cerr << "lost connection to X server" << std::endl;
            running_ = false;
        }
    }
}

void WindowManager::spawn(const char* const command[])
{
    switch (fork()) {
        case -1: {
            std::cerr << "fork() failed" << std::endl;
            return;
        }

        case 0: {
            prepare_spawn_process();
            execvp(command[0], const_cast<char**>(command));
            std::cerr << "execvp failed" << std::endl;
            std::exit(1);
        }
    }
}

void WindowManager::prepare_wm_process()
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, nullptr);

    while (waitpid(-1, nullptr, WNOHANG) > 0)
        ;
}

void WindowManager::prepare_spawn_process()
{
    close(xcb_get_file_descriptor(ewmh.connection));

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

void WindowManager::handle_key_press(const xcb_key_press_event_t* event)
{
    auto& keycode = event->detail;
    auto& modifiers = event->state;

    for (const auto& resolved_bind : resolved_keybinds_) {
        if (resolved_bind.keycode == keycode && resolved_bind.modifiers == modifiers) {
            resolved_bind.handler(this);
            break;
        }
    }
}

void WindowManager::handle_client_message(const xcb_client_message_event_t* event)
{
    if (event->type == ewmh._NET_WM_STATE) {
        if (conf_.debug_events())
            std::print("_NET_WM_STATE  ");

        switch (event->data.data32[0]) {
            case XCB_EWMH_WM_STATE_REMOVE: {
                if (conf_.debug_events())
                    std::print("REMOVE");

                xcb_change_property(ewmh.connection, XCB_PROP_MODE_REPLACE, event->window, ewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 0, nullptr);
                break;
            }

            case XCB_EWMH_WM_STATE_ADD: {
                if (conf_.debug_events())
                    std::print("ADD");

                break;
            }

            case XCB_EWMH_WM_STATE_TOGGLE: {
                if (conf_.debug_events())
                    std::print("TOGGLE");

                break;
            }
        }

        std::print("  [");
        utils::format_atom_name(std::cout, ewmh, event->data.data32[1]);
        std::print(", ");
        utils::format_atom_name(std::cout, ewmh, event->data.data32[2]);
        std::cout << "]  source: " << event->data.data32[3] << std::endl;
    }
}

void WindowManager::handle_configure_request(const xcb_configure_request_event_t* event)
{
    constexpr const uint16_t mask = (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
    constexpr const uint32_t values[] = { 0, 0, 1024, 768 };
    xcb_configure_window(ewmh.connection, event->window, mask, values);
}

void WindowManager::log_xcb_error(const xcb_generic_error_t* error)
{
    if (!error_context)
        xcb_errors_context_new(ewmh.connection, &error_context);

    auto& errorcode = error->error_code;
    std::cerr << "error: " << int(errorcode) << " Bad" << xcb_errors_get_name_for_error(error_context, errorcode, nullptr) << std::endl;
}

}
