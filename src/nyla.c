#include "nyla.h"

#define X(key) static xcb_keycode_t key##Keycode;
NYLA_KEYS(X)
#undef X

const char*
strXcbEventType(u8 type)
{
  switch (type) {
#define X(a, b)                                                                                                                                                                                        \
  case a: return #a;
    NYLA_XEVENTS(X)
#undef X
  }
  return "unknown";
}

NYLA_XEVENT_HANDLER(map_request)
{
  xcb_map_window(conn, e->window);
}

NYLA_XEVENT_HANDLER(map_notify)
{
  if (e->override_redirect)
    return;

  NYLA_FOREACH_CLIENTS (client) {
    if (!client->window) {
      client->window = e->window;
      memset((char*)client + sizeof(client->window), 0, sizeof(NylaClient) - sizeof(client->window));
      break;
    }
  }
}

NYLA_XEVENT_HANDLER(unmap_notify)
{
  NYLA_FOREACH_CLIENTS (client) {
    if (client->window == e->window) {
      memset(client, 0, sizeof(*client));
      break;
    }
  }
}

NYLA_XEVENT_HANDLER(key_press)
{
  NYLA_DLOGd(e->detail);
}

NYLA_XEVENT_HANDLER(create_notify) {}

NYLA_XEVENT_HANDLER(destroy_notify) {}

NYLA_XEVENT_HANDLER(configure_request) {}

NYLA_XEVENT_HANDLER(configure_notify) {}

NYLA_XEVENT_HANDLER(client_message) {}

int
main()
{
  int iscreen;
  xcb_connection_t* conn = xcb_connect(NULL, &iscreen);
  assert(conn && "could not connect to X server");

  xcb_screen_t* screen = xcb_aux_get_screen(conn, iscreen);
  assert(screen && "could not get X screen");

  bool ok = !NYLA_XCB_CHECKED(xcb_change_window_attributes, screen->root, XCB_CW_EVENT_MASK, (u32[]){XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY});
  assert(ok && "could not change root window attributes");

  {
    xcb_key_symbols_t* syms = xcb_key_symbols_alloc(conn);

#define X(key)                                                                                                                                                                                         \
  do {                                                                                                                                                                                                 \
    xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(syms, XK_##key);                                                                                                                             \
    key##Keycode = *keycodes;                                                                                                                                                                          \
    free(keycodes);                                                                                                                                                                                    \
    xcb_grab_key(conn, 0, screen->root, XCB_MOD_MASK_4, key##Keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);                                                                                       \
  } while (false);
    NYLA_KEYS(X)
#undef X

    xcb_flush(conn);
    xcb_key_symbols_free(syms);
  }

  NylaClient clients[64] = {0};

  while (true) {
    while (true) {
      xcb_generic_event_t* e = xcb_poll_for_event(conn);
      if (!e)
        break;

      u8 type = e->response_type & ~0x80;
      if (type != XCB_MOTION_NOTIFY) {
        NYLA_DLOG_FMT("received event: %s", strXcbEventType(type));
      }

      switch (type) {
#define X(event, type)                                                                                                                                                                                 \
  case event: nyla_handle_##type((void*)e, conn, clients, NYLA_END(clients)); break;
        NYLA_XEVENTS(X)
#undef X
        NYLA_DEFAULT_UNREACHABLE
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
