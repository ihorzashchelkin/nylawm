#pragma once

#include <bitset>
#include <cstdint>
#include <epoxy/glx_generated.h>
#include <span>
#include <unordered_map>
#include <vector>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>

#include <boost/circular_buffer.hpp>

typedef struct _XDisplay Display;
typedef struct __GLXcontextRec* GLXContext;

namespace nyla {

class WindowManager;
using Action = void (*)(WindowManager*);

struct Keybind
{
  uint16_t mMods;
  int mKeysyms;
  Action mActions;

  struct Resolved
  {
    uint16_t mMods;
    xcb_keycode_t mKeycode;
    Action mAction;
  };
};

struct Client
{
  uint32_t mXPixmap;
  uint32_t mGLXPixmap;
  uint32_t mGLVertexBuffer;
  uint32_t mGLVertexArray;
  int16_t mPosX;
  int16_t mPosY;
  uint16_t mWidth;
  uint16_t mHeight;
  uint16_t mBorderWidth;
};

struct IgnoredEvent
{
  uint16_t mSequence;
  uint8_t mType;
};

class WindowManager
{
  enum
  {
    Flag_Init = 0,
    Flag_Running,
    Flag_Debug,
    Flag_Count
  };
  std::bitset<Flag_Count> mFlags{};
  Display* mDisplay{};
  xcb_ewmh_connection_t mEwmh{};
  int mScreenNumber{};
  xcb_screen_t* mScreen{};
  xcb_errors_context_t* mErrorContext{};
  xcb_window_t mCompositorWindow{};
  std::vector<Keybind::Resolved> mKeybinds{};
  GLXContext mGlxContext{};
  uint64_t mGlxWindow{};
  GLXFBConfig mFbConfig{};
  int mVisualId{};
  std::unordered_map<xcb_window_t, Client> mClients{};
  boost::circular_buffer<IgnoredEvent> mIgnoredEvents{ 4 };
  uint8_t mPendingXRequests{};

public:
  WindowManager(std::span<const Keybind> aKeyBinds);

  explicit operator bool() { return mFlags.test(Flag_Init); }

  bool IsRunning() { return mFlags.test(Flag_Running); }
  void Quit() { mFlags.reset(Flag_Running); }

  bool IsDebug() { return mFlags.test(Flag_Debug); }
  void SetDebug(bool aIsDebug) { mFlags.set(Flag_Debug, aIsDebug); }

  void Spawn(const char* const aCommand[]);

  void Run();

private:
  void GrabKeys();

  void RenderAll();

  void HandleError(const xcb_generic_error_t* aError);
  void HandleEvent(const xcb_generic_event_t* aEvent);

  void HandleButtonPress(const xcb_button_press_event_t* aEvent);
  void HandleButtonRelease(const xcb_button_release_event_t* aEvent);
  void HandleClientMessage(const xcb_client_message_event_t* aEvent);
  void HandleConfigureNotify(const xcb_configure_notify_event_t* aEvent);
  void HandleConfigureRequest(const xcb_configure_request_event_t* aEvent);
  void HandleCreateNotify(const xcb_create_notify_event_t* aEvent);
  void HandleDestroyNotify(const xcb_destroy_notify_event_t* aEvent);
  void HandleEnterNotify(const xcb_enter_notify_event_t* aEvent);
  void HandleExpose(const xcb_expose_event_t* aEvent);
  void HandleFocusIn(const xcb_focus_in_event_t* aEvent);
  void HandleKeyPress(const xcb_key_press_event_t* aEvent);
  void HandleKeyRelease(const xcb_key_release_event_t* aEvent);
  void HandleMapNotify(const xcb_map_notify_event_t* aEvent);
  void HandleMapRequest(const xcb_map_request_event_t* aEvent);
  void HandleMappingNotify(const xcb_mapping_notify_event_t* aEvent);
  void HandleMotionNotify(const xcb_motion_notify_event_t* aEvent);
  void HandlePropertyNotify(const xcb_property_notify_event_t* aEvent);
  void HandleResizeRequest(const xcb_resize_request_event_t* aEvent);
  void HandleUnmapNotify(const xcb_unmap_notify_event_t* aEvent);
};

bool
PropertyAtomContains(xcb_connection_t* conn,
                     xcb_window_t win,
                     xcb_atom_t prop,
                     xcb_atom_t val);

void
PropertyAtomRemove(xcb_connection_t* conn,
                   xcb_window_t win,
                   xcb_atom_t prop,
                   xcb_atom_t val);

void
PropertyAtomAppend(xcb_connection_t* conn,
                   xcb_window_t win,
                   xcb_atom_t prop,
                   xcb_atom_t val);

void
PropertyAtomPrepend(xcb_connection_t* conn,
                    xcb_window_t win,
                    xcb_atom_t prop,
                    xcb_atom_t val);

void
PropertyAtomToggle(xcb_connection_t* conn,
                   xcb_window_t win,
                   xcb_atom_t prop,
                   xcb_atom_t val);

}
