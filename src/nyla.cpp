#include "nyla.hpp"
#include "src/nylaunity.hpp"
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

void
handleMapRequest(WindowManager& wm, xcb_map_request_event_t& e)
{
  xcb_map_window(wm.conn, e.window);
}

void
handleMapNotify(WindowManager& wm, xcb_map_notify_event_t& e)
{
  if (e.override_redirect)
    return;

  wm.clients[e.window] = {};
  LOG(std::format("added client {}", e.window));
}

void
handleUnmapNotify(WindowManager& wm, xcb_unmap_notify_event_t& e)
{
  wm.clients.erase(e.window);
}

void
handleKeyPress(WindowManager& wm, xcb_key_press_event_t& e)
{
}

void
run()
{
  int iscreen;

  WindowManager wm{};
  auto& conn = wm.conn;
  auto& screen = wm.screen;
  auto& clients = wm.clients;
  auto& tileAssignments = wm.tileAssignments;

  conn = xcb_connect(nullptr, &iscreen);
  assert(conn && "could not connect to X server");

  screen = xcb_aux_get_screen(conn, iscreen);
  assert(screen && "could not get X screen");

  bool ok = !XCB_CHECKED(xcb_change_window_attributes,
                         screen->root,
                         XCB_CW_EVENT_MASK,
                         (u32[]){ XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                                  XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY });
  assert(ok && "could not change root window attributes");

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
          handleMapRequest(wm, *(xcb_map_request_event_t*)event);
          break;
        case XCB_MAP_NOTIFY:
          handleMapNotify(wm, *(xcb_map_notify_event_t*)event);
          break;
        case XCB_UNMAP_NOTIFY:
          handleUnmapNotify(wm, *(xcb_unmap_notify_event_t*)event);
          break;
        case XCB_KEY_PRESS:
          handleKeyPress(wm, *(xcb_key_press_event_t*)event);
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
