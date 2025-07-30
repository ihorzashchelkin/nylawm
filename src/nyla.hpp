#pragma once

#include <bitset>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include <EGL/egl.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>

namespace nyla {

struct State;

struct Keybind
{
  uint16_t mod;
  uint32_t key;
  void (*action)(State&);
};

struct Client
{
  int16_t x;
  int16_t y;
  uint16_t width;
  uint16_t height;
  uint16_t borderWidth;
};

struct State
{
  enum
  {
    Flag_Running,
    Flag_Count
  };
  std::bitset<Flag_Count> flags;

  struct
  {
    xcb_connection_t* conn;
    xcb_screen_t* screen;
    xcb_window_t window;
    uint8_t pendingRequests;
  } xcb;

  struct
  {
    EGLDisplay dpy;
    EGLContext context;
    EGLSurface surface;
  } egl;

  std::vector<Keybind> keybinds;
  std::unordered_map<xcb_window_t, Client> clients;
};

const char*
initXcb(State& state, std::span<const Keybind> keybinds);

const char*
initEgl(State& state);

void
preRun(State& state);

void
run(State& state);

void
grabKeys(State& state);

void
spawn(State& state, const char* const command[]);

void
quit(State& state);

namespace handlers {

void
configureRequest(State& state, xcb_configure_request_event_t& event);

void
configureNotify(State& state, xcb_configure_notify_event_t& event);

void
createNotify(State& state, xcb_create_notify_event_t& event);

void
destroyNotify(State& state, xcb_destroy_notify_event_t& event);

void
enterNotify(State& state, xcb_enter_notify_event_t& event);

void
expose(State& state, xcb_expose_event_t& event);

void
focusIn(State& state, xcb_focus_in_event_t& event);

void
keyPress(State& state, xcb_key_press_event_t& event);

void
keyRelease(State& state, xcb_key_release_event_t& event);

void
keyRelease(State& state, xcb_key_release_event_t& event);

void
buttonPress(State& state, xcb_button_press_event_t& event);

void
buttonRelease(State& state, xcb_button_release_event_t& event);

void
resizeRequest(State& state, xcb_resize_request_event_t& event);

void
unmapNotify(State& state, xcb_unmap_notify_event_t& event);

void
clientMessage(State& state, xcb_client_message_event_t& event);

void
propertyNotify(State& state, xcb_property_notify_event_t& event);

void
motionNotify(State& state, xcb_motion_notify_event_t& event);

void
mapRequest(State& state, xcb_map_request_event_t& event);

void
mapNotify(State& state, xcb_map_notify_event_t& event);

void
mappingNotify(State& state, xcb_mapping_notify_event_t& event);

void
error(State& state, xcb_generic_error_t& aError);

void
genericEvent(State& state, xcb_generic_event_t* event);

}

}
