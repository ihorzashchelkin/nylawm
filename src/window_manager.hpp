#pragma once

#include <cstdint>
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

class window_manager;
using action = void (*)(window_manager*);

struct key_bind
{
    uint16_t Mods;
    int KeySym;
    action Handler;
};

struct resolved_key_bind
{
    uint16_t Mods;
    xcb_keycode_t KeyCode;
    action Handler;
};

struct workspace
{
};

struct managed_client
{
    static inline constexpr uint8_t FlagWasMapped = 1;
    static inline constexpr uint8_t FlagMapped = 1 << 1;
    uint8_t Flags;

    uint8_t WorkspaceId;

    uint16_t X, CurrentX;
    uint16_t Y, CurrentY;
    uint16_t Width, CurrentWidth;
    uint16_t Height, CurrentHeight;
};

struct config
{
    bool DebugLog;
    std::span<const key_bind> Bindings;
};

class window_manager
{
public:
    explicit window_manager(const config& conf)
        : Config(conf)
        , ErrorContext(nullptr)
    {
    }
    ~window_manager() { Cleanup(); }

    bool TryInit();
    void Run();
    void Quit() { Flags &= ~(FlagRunning); }
    void Spawn(const char* const command[]);

private:
    void HandleEvent(const xcb_generic_event_t* Event);
    void HandleError(const xcb_generic_error_t* Error);
    void HandleButtonPress(const xcb_button_press_event_t* Event);
    void HandleClientMessage(const xcb_client_message_event_t* Event);
    void HandleConfigureRequest(const xcb_configure_request_event_t* Event);
    void HandleDestroyNotify(const xcb_destroy_notify_event_t* Event);
    void HandleEntryNotify(const xcb_enter_notify_event_t* Event);
    void HandleExpose(const xcb_expose_event_t* Event);
    void HandleFocusIn(const xcb_focus_in_event_t* Event);
    void HandleKeyPress(const xcb_key_press_event_t* Event);
    void HandleMappingNotify(const xcb_mapping_notify_event_t* Event);
    void HandleMapRequest(const xcb_map_request_event_t* Event);
    void HandleMotionNotify(const xcb_motion_notify_event_t* Event);
    void HandlePropertyNotify(const xcb_property_notify_event_t* Event);
    void HandleResizeRequest(const xcb_resize_request_event_t* Event);
    void HandleUnmapNotify(const xcb_unmap_notify_event_t* Event);

    void Cleanup();
    void Prepare();
    void PrepareSpawn();

    managed_client* WindowToClient(xcb_window_t Window);
    managed_client* Manage(xcb_window_t Window);

    void TryResolveKeyBinds();
    void GrabKeys();

    static inline constexpr uint8_t FlagRunning = 1;
    uint8_t Flags = 0;

    uint8_t ActiveWorkspaceId;
    std::unordered_map<uint8_t, workspace> Workspaces = {
        { 0, workspace {} }
    };
    std::unordered_map<xcb_window_t, managed_client> ManagedClients;

    xcb_ewmh_connection_t Conn;
    xcb_connection_t*& XConn() { return Conn.connection; }

    xcb_screen_t* Screen;
    const xcb_window_t& RootWindow() { return Screen->root; }

    xcb_errors_context_t* ErrorContext;

    const config& Config;
    std::vector<resolved_key_bind> ResolvedKeybinds;
};

}
