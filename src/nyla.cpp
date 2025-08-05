#include "nyla.hpp"
#include <print>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

namespace nyla {

cstr
strXcbEventType(u8 type)
{
  switch (type) {

#define CASE(X)                                                                \
  case X:                                                                      \
    return #X;

    CASE(XCB_CREATE_NOTIFY)
    CASE(XCB_DESTROY_NOTIFY)
    CASE(XCB_CONFIGURE_REQUEST)
    CASE(XCB_CONFIGURE_NOTIFY)
    CASE(XCB_MAP_REQUEST)
    CASE(XCB_MAP_NOTIFY)
    CASE(XCB_CLIENT_MESSAGE)

#undef CASE
  }

  LOG(type);
  return "unknown";
}

/*
 * Consider switching to C because I don't see how C++ helps here...
 */

static void
handleMapRequest(xcb_map_request_event_t& e, xcb_connection_t* conn)
{
  xcb_map_window(conn, e.window);
}

static void
handleMapNotify(xcb_map_notify_event_t& e,
                std::unordered_map<xcb_window_t, Client>& clients)
{
  if (e.override_redirect)
    return;

  clients[e.window] = {};
  LOG(std::format("added client {}", e.window));
}

static void
handleUnmapNotify(xcb_unmap_notify_event_t& e,
                  std::unordered_map<xcb_window_t, Client>& clients)
{
  clients.erase(e.window);
}

static void
handleKeyPress(xcb_key_press_event_t& e)
{
  std::println("keypress! {}", e.detail);
}

void
run()
{
  std::unordered_map<xcb_window_t, Client> clients;
  std::array<xcb_window_t, 4> tileAssignments;

  int iscreen;
  xcb_connection_t* conn = xcb_connect(nullptr, &iscreen);
  assert(conn && "could not connect to X server");

  xcb_screen_t* screen = xcb_aux_get_screen(conn, iscreen);
  assert(screen && "could not get X screen");

  bool ok = !XCB_CHECKED(xcb_change_window_attributes,
                         screen->root,
                         XCB_CW_EVENT_MASK,
                         (u32[]){ XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                                  XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY });
  assert(ok && "could not change root window attributes");

  {
    xcb_key_symbols_t* syms = xcb_key_symbols_alloc(conn);
    xcb_keycode_t* keycodes;

    keycodes = xcb_key_symbols_get_keycode(syms, XK_a);
    static xcb_keycode_t aKeycode = *keycodes;
    free(keycodes);

    xcb_key_symbols_get_keycode(syms, XK_s);

    xcb_key_symbols_get_keycode(syms, XK_d);

    xcb_key_symbols_get_keycode(syms, XK_f);

    xcb_grab_key(conn,
                 0,
                 screen->root,
                 XCB_MOD_MASK_4,
                 *xcb_key_symbols_get_keycode(syms, XK_k),
                 XCB_GRAB_MODE_ASYNC,
                 XCB_GRAB_MODE_ASYNC);

    xcb_flush(conn);

    xcb_key_symbols_free(syms);
  }

  while (true) {
    while (true) {
      xcb_generic_event_t* event = xcb_poll_for_event(conn);
      if (!event)
        break;

      u8 eventType = event->response_type & ~0x80;
      if (eventType != XCB_MOTION_NOTIFY) {
        LOG(std::format("received event {}", strXcbEventType(eventType)));
      }

      switch (eventType) {
        case XCB_MAP_REQUEST:
          handleMapRequest(*(xcb_map_request_event_t*)event, conn);
          break;
        case XCB_MAP_NOTIFY:
          handleMapNotify(*(xcb_map_notify_event_t*)event, clients);
          break;
        case XCB_UNMAP_NOTIFY:
          handleUnmapNotify(*(xcb_unmap_notify_event_t*)event, clients);
          break;
        case XCB_KEY_PRESS:
          handleKeyPress(*(xcb_key_press_event_t*)event);
          break;
      }
    }

    // do this via manual tile assignemnt?
#if 0
        xcb_configure_window(
          wm.conn,
          window,
          XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
            XCB_CONFIG_WINDOW_HEIGHT,
          (u32[]){
            0, 0, wm.screen->width_in_pixels, wm.screen->height_in_pixels });
#endif

    xcb_flush(conn);
  }

  xcb_disconnect(conn);
}

}
