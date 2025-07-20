#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <iterator>
#include <string_view>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "utils.hpp"
#include "window_manager.hpp"

void wm::window_manager::Cleanup()
{
    if (ErrorContext)
        xcb_errors_context_free(ErrorContext);

    if (XConn()) {
        xcb_disconnect(XConn());
        // TODO: is this needed?
        // xcb_ewmh_connection_wipe(&m_ewmh);
        memset(&Conn, 0, sizeof(Conn));
    }
}

bool wm::window_manager::TryInit()
{
    int ScreenNumber;
    XConn() = xcb_connect(nullptr, &ScreenNumber);
    if (xcb_connection_has_error(XConn())) {
        std::cerr << "could not connect to X server" << std::endl;
        return false;
    }

    if (xcb_errors_context_new(XConn(), &ErrorContext)) {
        std::cerr << "could not create error context" << std::endl;
        return false;
    }

    const xcb_setup_t* ConnSetup = xcb_get_setup(XConn());
    xcb_screen_iterator_t ScreenIter = xcb_setup_roots_iterator(ConnSetup);

    Screen = nullptr;
    for (int i = 0; ScreenIter.rem && i <= ScreenNumber;
        xcb_screen_next(&ScreenIter), i++) {
        if (i == ScreenNumber)
            Screen = ScreenIter.data;
    }

    if (!Screen) {
        std::cerr << "could not find screen: " << ScreenNumber << std::endl;
        return false;
    }

    static constexpr uint32_t RootEventMask = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
        | XCB_EVENT_MASK_PROPERTY_CHANGE
        | XCB_EVENT_MASK_EXPOSURE
        | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_POINTER_MOTION
        | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
    xcb_void_cookie_t Cookie = xcb_change_window_attributes_checked(XConn(),
        RootWindow(),
        XCB_CW_EVENT_MASK,
        &RootEventMask);

    xcb_generic_error_t* Error = xcb_request_check(XConn(), Cookie);
    if (Error) {
        wm::utils::FormatXCBError(std::cerr, ErrorContext, Error);
        std::cerr << std::endl;

        if (Error->error_code == 10) // Bad Access
            std::cerr << "is another wm already running?" << std::endl;

        return false;
    }

    xcb_intern_atom_cookie_t* InternAtomsCookie = xcb_ewmh_init_atoms(Conn.connection, &Conn);
    if (!xcb_ewmh_init_atoms_replies(&Conn, InternAtomsCookie, (xcb_generic_error_t**)0)) {
        std::cerr << "could not intern ewmh atoms" << std::endl;
        return false;
    }

    static xcb_atom_t SupportedStates[] = {
        Conn._NET_WM_STATE_MODAL,
        Conn._NET_WM_STATE_STICKY,
        Conn._NET_WM_STATE_MAXIMIZED_VERT,
        Conn._NET_WM_STATE_MAXIMIZED_HORZ,
        Conn._NET_WM_STATE_SHADED,
        Conn._NET_WM_STATE_SKIP_TASKBAR,
        Conn._NET_WM_STATE_SKIP_PAGER,
        Conn._NET_WM_STATE_HIDDEN,
        Conn._NET_WM_STATE_FULLSCREEN,
        Conn._NET_WM_STATE_ABOVE,
        Conn._NET_WM_STATE_BELOW,
        Conn._NET_WM_STATE_DEMANDS_ATTENTION,
    };
    xcb_ewmh_set_supported(&Conn, ScreenNumber, std::size(SupportedStates), SupportedStates);

    return true;
}

void wm::window_manager::TryResolveKeyBinds()
{
    xcb_key_symbols_t* Syms = xcb_key_symbols_alloc(XConn());

    for (const auto& Binding : Config.Bindings) {
        xcb_keycode_t* KeyCodes = xcb_key_symbols_get_keycode(Syms, Binding.KeySym);
        if (KeyCodes) {
            for (int i = 0; *(KeyCodes + i) != XCB_NO_SYMBOL; i++)
                ResolvedKeybinds.emplace_back(resolved_key_bind { Binding.Mods, *(KeyCodes + i), Binding.Handler });

            free(KeyCodes);
        }
    }

    free(Syms);
}

void wm::window_manager::GrabKeys()
{
    for (const auto& ResolvedBind : ResolvedKeybinds) {
        xcb_grab_key(XConn(), 0, RootWindow(), ResolvedBind.Mods, ResolvedBind.KeyCode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }
}

void wm::window_manager::Run()
{
    {
        xcb_query_tree_cookie_t Cookie = xcb_query_tree(XConn(), RootWindow());
        xcb_query_tree_reply_t* Reply = xcb_query_tree_reply(XConn(), Cookie, nullptr);

        xcb_window_t* Children = xcb_query_tree_children(Reply);

        for (int i = 0; i < xcb_query_tree_children_length(Reply); i++) {
            auto& child = Children[i];

            xcb_reparent_window(XConn(),
                child,
                RootWindow(),
                0, 0);

#if 0
            xcb_get_property_cookie_t cookie = xcb_get_property(XConn(),
                0,
                child,
                Conn._NET_WM_NAME,
                Conn.UTF8_STRING,
                0,
                std::numeric_limits<uint32_t>::max());

            xcb_get_property_reply_t* reply = xcb_get_property_reply(XConn(), cookie, nullptr);
            if (reply) {
                int len = xcb_get_property_value_length(reply);
                const std::string_view str { (char*)xcb_get_property_value(reply), size_t(len) };
                if (!str.empty())
                    std::cout << str << std::endl;
            }
#endif
        }

        xcb_flush(XConn());
        free(Reply);
    }

    Prepare();
    TryResolveKeyBinds();
    GrabKeys();

    Flags |= FlagRunning;
    do {
        xcb_generic_event_t* Event = xcb_wait_for_event(XConn());

        if (Event) {
            HandleEvent(Event);
            free(Event);

            constexpr bool ParanoidFlush = false;
            if (ParanoidFlush)
                xcb_flush(XConn());
        } else {
            std::cerr << "lost connection to X server" << std::endl;
            Flags &= ~FlagRunning;
            break;
        }

        if (Flags & FlagDirty) {
            for (auto& [Window, ManagedClient] : ManagedClients) {
                if ((ManagedClient.Flags & ManagedClient.FlagValid) == 0) {
                    if (Config.DebugLog)
                        std::cout << "Mapping " << Window << std::endl;

                    xcb_map_window(XConn(), Window);

#if 1
                    uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
                    uint32_t values[] { 0, 0, Screen->width_in_pixels, Screen->height_in_pixels };
                    xcb_configure_window(XConn(), Window, mask, values);
                    xcb_flush(XConn());
#endif

                    ManagedClient.Flags |= ManagedClient.FlagValid;
                }
            }
            xcb_flush(XConn());
        }

    } while (Flags & FlagRunning);
}

void wm::window_manager::Spawn(const char* const command[])
{
    if (fork() == 0) {
        PrepareSpawn();
        execvp(command[0], const_cast<char**>(command));
        std::cerr << "execvp failed" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void wm::window_manager::Prepare()
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, nullptr);

    while (waitpid(-1, nullptr, WNOHANG) > 0)
        ;
}

void wm::window_manager::PrepareSpawn()
{
    close(xcb_get_file_descriptor(XConn()));

    constexpr bool RedirectSpawnSTDIO = true;
    if (RedirectSpawnSTDIO) {
        int dev_null_fd = open("/dev/null", O_RDONLY);
        dup2(dev_null_fd, STDIN_FILENO);
        close(dev_null_fd);

        dev_null_fd = open("/dev/null", O_WRONLY);
        dup2(dev_null_fd, STDOUT_FILENO);
        dup2(dev_null_fd, STDERR_FILENO);
        close(dev_null_fd);
    }

    setsid();

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &sa, nullptr);
}

void wm::window_manager::HandleButtonPress(const xcb_button_press_event_t* event)
{
}

void wm::window_manager::HandleClientMessage(const xcb_client_message_event_t* event)
{
    std::cout << "ClientMessage from " << event->window << std::endl;

    if (event->type == Conn._NET_WM_STATE) {
        auto& verb = event->data.data32[0];
        auto& arg1 = event->data.data32[1];
        auto& arg2 = event->data.data32[2];

        if (arg1 == Conn._NET_WM_STATE_HIDDEN && verb == XCB_EWMH_WM_STATE_REMOVE) {
            // uint32_t stack_vals[] = { XCB_STACK_MODE_ABOVE };
            // xcb_configure_window(xconn(),
            //     event->window,
            //     XCB_CONFIG_WINDOW_STACK_MODE,
            //     stack_vals);
            //

            // xcb_map_window(XConn(), event->window);

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

void wm::window_manager::HandleError(const xcb_generic_error_t* error)
{
    utils::FormatXCBError(std::cerr, ErrorContext, error);
    std::cerr << std::endl;
}

void wm::window_manager::HandleConfigureRequest(const xcb_configure_request_event_t* event)
{
    const bool managed = ManagedClients.contains(event->window);
    if (!managed) {
        // should honor ConfigureRequest as-is here - otherwise some clients won't map

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

        COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_X, x);
        COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_Y, y);
        COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_WIDTH, width);
        COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_HEIGHT, height);
        COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_BORDER_WIDTH, border_width);
        COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_SIBLING, sibling);
        COPY_MASK_MEMBER(XCB_CONFIG_WINDOW_STACK_MODE, stack_mode);

#undef COPY_MASK_MEMBER

        xcb_configure_window(XConn(), event->window, mask, values);
        xcb_flush(XConn());

        ManagedClients.emplace(std::pair { event->window, managed_client { .Window = event->window } });
        Flags |= FlagDirty;
    }
}

void wm::window_manager::HandleDestroyNotify(const xcb_destroy_notify_event_t* event)
{
    ManagedClients.erase(event->window);
    Flags |= FlagDirty;
}

void wm::window_manager::HandleEntryNotify(const xcb_enter_notify_event_t* event)
{
}

void wm::window_manager::HandleExpose(const xcb_expose_event_t* event)
{
    std::cout << "Expose from " << event->window << std::endl;
}

void wm::window_manager::HandleFocusIn(const xcb_focus_in_event_t* event)
{
    std::cout << "FocusIn" << std::endl;
}

void wm::window_manager::HandleKeyPress(const xcb_key_press_event_t* event)
{
    auto& keycode = event->detail;
    auto& modifiers = event->state;

    for (const auto& resolved_bind : ResolvedKeybinds) {
        if (resolved_bind.KeyCode == keycode && resolved_bind.Mods == modifiers) {
            resolved_bind.Handler(this);
            break;
        }
    }
}

void wm::window_manager::HandleMappingNotify(const xcb_mapping_notify_event_t* event)
{
    std::cout << "MappingNotify" << std::endl;
}

void wm::window_manager::HandleMapRequest(const xcb_map_request_event_t* event)
{
    std::cout << "MapRequest from " << event->window << std::endl;
}

void wm::window_manager::HandleMotionNotify(const xcb_motion_notify_event_t* event)
{
}

void wm::window_manager::HandlePropertyNotify(const xcb_property_notify_event_t* event)
{
    std::cout << "PropertyNotify from " << event->window << std::endl;
}

void wm::window_manager::HandleResizeRequest(const xcb_resize_request_event_t* event)
{
    std::cout << "ResizeRequest from " << event->window << std::endl;
}

void wm::window_manager::HandleUnmapRequest(const xcb_unmap_notify_event_t* event)
{
}

void wm::window_manager::HandleEvent(const xcb_generic_event_t* event)
{
    switch (event->response_type & ~0x80) {
        case 0:
            HandleError(
                reinterpret_cast<const xcb_generic_error_t*>(event));
            break;

        case XCB_BUTTON_PRESS:
            HandleButtonPress(
                reinterpret_cast<const xcb_button_press_event_t*>(event));
            break;
        case XCB_CLIENT_MESSAGE:
            HandleClientMessage(
                reinterpret_cast<const xcb_client_message_event_t*>(event));
            break;
        case XCB_CONFIGURE_REQUEST:
            HandleConfigureRequest(
                reinterpret_cast<const xcb_configure_request_event_t*>(event));
            break;
        case XCB_DESTROY_NOTIFY:
            HandleDestroyNotify(
                reinterpret_cast<const xcb_destroy_notify_event_t*>(event));
            break;
        case XCB_ENTER_NOTIFY:
            HandleEntryNotify(
                reinterpret_cast<const xcb_enter_notify_event_t*>(event));
            break;
        case XCB_EXPOSE:
            HandleExpose(
                reinterpret_cast<const xcb_expose_event_t*>(event));
            break;
        case XCB_FOCUS_IN:
            HandleFocusIn(
                reinterpret_cast<const xcb_focus_in_event_t*>(event));
            break;
        case XCB_KEY_PRESS:
            HandleKeyPress(
                reinterpret_cast<const xcb_key_press_event_t*>(event));
            break;
        case XCB_MAPPING_NOTIFY:
            HandleMappingNotify(
                reinterpret_cast<const xcb_mapping_notify_event_t*>(event));
            break;
        case XCB_PROPERTY_NOTIFY:
            HandlePropertyNotify(
                reinterpret_cast<const xcb_property_notify_event_t*>(event));
            break;
        case XCB_RESIZE_REQUEST:
            HandleResizeRequest(
                reinterpret_cast<const xcb_resize_request_event_t*>(event));
            break;
        case XCB_UNMAP_NOTIFY:
            HandleUnmapRequest(
                reinterpret_cast<const xcb_unmap_notify_event_t*>(event));
            break;
    }
}
