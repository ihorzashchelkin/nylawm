#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <unordered_map>
#include <vector>

#include <unistd.h>

#include <sys/wait.h>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>

namespace wm {

class controller;
using action = void (*)(controller*);

struct keybind
{
    uint16_t Mods;
    int      KeySym;
    action   Handler;

    struct resolved
    {
        uint16_t      Mods;
        xcb_keycode_t KeyCode;
        action        Handler;
    };
};

struct desktop
{
    uint32_t     Pixmap;
    xcb_window_t ActiveWin;
};

struct client
{
    uint8_t                       Flags;
    static inline decltype(Flags) FlagMapped = 1 << 0;

    uint32_t Pixmap;
    uint8_t  DesktopIndex;
    uint16_t X;
    uint16_t CurrentX;
    uint16_t Y;
    uint16_t CurrentY;
    uint16_t Width;
    uint16_t CurrentWidth;
    uint16_t Height;
    uint16_t CurrentHeight;
};

struct config
{
    bool                     DebugLog;
    std::span<const keybind> Bindings;
};

class controller
{
    uint8_t                       Flags;
    static inline decltype(Flags) FlagRunning = 1 << 0;

    uint8_t                                  CurDesktopIndex;
    uint16_t                                 MouseX;
    uint16_t                                 MouseY;
    xcb_window_t                             CompositorWindow;
    xcb_window_t                             OverlayWin;
    uint32_t                                 GraphicsContext;
    std::array<desktop, 16>                  Desktops;
    std::unordered_map<xcb_window_t, client> Clients;
    xcb_ewmh_connection_t                    Conn;
    xcb_screen_t*                            Screen;
    xcb_errors_context_t*                    ErrorContext;
    const config&                            Config;
    std::vector<keybind::resolved>           ResolvedKeybinds;

public:
    explicit controller(const config& Config)
        : Flags {}
        , CurDesktopIndex {}
        , MouseX { std::numeric_limits<decltype(MouseX)>::max() }
        , MouseY { std::numeric_limits<decltype(MouseY)>::max() }
        , GraphicsContext {}
        , Desktops {}
        , Clients {}
        , Conn {}
        , Screen {}
        , ErrorContext {}
        , Config { Config }
        , ResolvedKeybinds {}
    {
    }
    ~controller();

    bool TryInit();
    void Run();

    void Quit() { Flags &= ~(FlagRunning); }
    void Spawn(const char* const command[]);
    void Kill();
    void SwitchToNextWorkspace();
    void SwitchToPrevWorkspace();

private:
    xcb_connection_t* GetConn() { return Conn.connection; }
    void              SetConn(xcb_connection_t* Conn) { this->Conn.connection = Conn; }
    xcb_window_t      GetRoot() { return Screen->root; }
    desktop&          GetCurDesktop() { return Desktops[CurDesktopIndex]; }

    client* GetClient(xcb_window_t Window);
    client& Manage(xcb_window_t Window);
    void    Prepare();
    void    PrepareSpawn();
    void    TryResolveKeyBinds();
    void    GrabKeys();

    void HandleEvent(const xcb_generic_event_t* Event);
    void HandleError(const xcb_generic_error_t* Error);
    void HandleButtonPress(const xcb_button_press_event_t* Event);
    void HandleClientMessage(const xcb_client_message_event_t* Event);
    void HandleConfigureRequest(const xcb_configure_request_event_t* Event);
    void HandleConfigureNotify(const xcb_configure_notify_event_t* Event);
    void HandleDestroyNotify(const xcb_destroy_notify_event_t* Event);
    void HandleEntryNotify(const xcb_enter_notify_event_t* Event);
    void HandleExpose(const xcb_expose_event_t* Event);
    void HandleFocusIn(const xcb_focus_in_event_t* Event);
    void HandleKeyPress(const xcb_key_press_event_t* Event);
    void HandleMappingNotify(const xcb_mapping_notify_event_t* Event);
    void HandleMapRequest(const xcb_map_request_event_t* Event);
    void HandleMapNotify(const xcb_map_notify_event_t* Event);
    void HandleMotionNotify(const xcb_motion_notify_event_t* Event);
    void HandlePropertyNotify(const xcb_property_notify_event_t* Event);
    void HandleResizeRequest(const xcb_resize_request_event_t* Event);
    void HandleUnmapNotify(const xcb_unmap_notify_event_t* Event);
};

}
