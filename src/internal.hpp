#pragma once

#include <cstdint>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

namespace cirnowm {

typedef struct _XDisplay Display;
typedef struct __GLXcontextRec* GLXContext;

xcb_connection_t*
XGetXCBConnection(Display* dpy);

enum XEventQueueOwner
{
  XlibOwnsEventQueue = 0,
  XCBOwnsEventQueue
};
void
XSetEventQueueOwner(Display* dpy, enum XEventQueueOwner owner);

extern Display*
XOpenDisplay(const char* displayName);

extern int
XCloseDisplay(Display* display);

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

class AutoDisplay
{
  Display* mDisplay;

public:
  explicit AutoDisplay(const char* aDisplayName)
    : mDisplay{ XOpenDisplay(aDisplayName) }
  {
  }
  ~AutoDisplay() noexcept
  {
    if (mDisplay)
      XCloseDisplay(mDisplay);
  }

  AutoDisplay(const AutoDisplay&) = delete;
  AutoDisplay& operator=(const AutoDisplay&) = delete;

  AutoDisplay(const AutoDisplay&&) = delete;
  AutoDisplay& operator=(const AutoDisplay&&) = delete;

  bool good() noexcept { return mDisplay != nullptr; }

  Display* get() noexcept { return mDisplay; }
  const Display* get() const noexcept { return mDisplay; }
};

class WindowManager
{
  AutoDisplay& mDisplay;
  xcb_connection_t* mConn;

  uint8_t mFlags;
  static inline decltype(mFlags) FlagRunning = 0x1;
  static inline decltype(mFlags) FlagDebug = 0x2;

public:
  WindowManager(AutoDisplay& aDisplay)
    : mDisplay{ aDisplay }
    , mConn{ XGetXCBConnection(aDisplay.get()) }
  {
    XSetEventQueueOwner(mDisplay.get(), XCBOwnsEventQueue);
  }

private:
  bool IsRunning() { return (mFlags & FlagRunning) != 0; }
  void SetRunning(bool aIsRunning)
  {
    if (aIsRunning)
      mFlags |= FlagRunning;
    else
      mFlags &= ~FlagRunning;
  }

  bool IsDebug() { return (mFlags & FlagDebug) != 0; }
  void SetDebug(bool aIsDebug)
  {
    if (aIsDebug)
      mFlags |= FlagDebug;
    else
      mFlags &= ~FlagDebug;
  }

  void HandleEvent(const xcb_generic_event_t* aEvent);
  void HandleError(const xcb_generic_error_t* aError);

  void HandleClientMessage(const xcb_client_message_event_t* aEvent);
  void HandlePropertyNotify(const xcb_property_notify_event_t* aEvent);

  void HandleCreateNotify(const xcb_create_notify_event_t* aEvent);
  void HandleDestroyNotify(const xcb_destroy_notify_event_t* aEvent);
  void HandleEnterNotify(const xcb_enter_notify_event_t* aEvent);
  void HandleExpose(const xcb_expose_event_t* aEvent);
  void HandleFocusIn(const xcb_focus_in_event_t* aEvent);

  void HandleConfigureRequest(const xcb_configure_request_event_t* aEvent);
  void HandleConfigureNotify(const xcb_configure_notify_event_t* aEvent);
  void HandleResizeRequest(const xcb_resize_request_event_t* aEvent);

  void HandleButtonPress(const xcb_button_press_event_t* aEvent);
  void HandleKeyPress(const xcb_key_press_event_t* aEvent);
  void HandleMotionNotify(const xcb_motion_notify_event_t* aEvent);
  void HandleMappingNotify(const xcb_mapping_notify_event_t* aEvent);

  void HandleMapRequest(const xcb_map_request_event_t* aEvent);
  void HandleMapNotify(const xcb_map_notify_event_t* aEvent);
  void HandleUnmapNotify(const xcb_unmap_notify_event_t* aEvent);
};

}
