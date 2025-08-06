#include "nyla.h"

static const char* termCommand[] = {"ghostty", NULL};

static xcb_connection_t* conn;
static int iscreen;
static xcb_screen_t* screen;
static NylaClient clients[64];

static xcb_keycode_t activeChordKey;

#define X(key, grabbed) static xcb_keycode_t key##Keycode;
NYLA_KEYS(X)
#undef X

void
spawn(const char* const command[])
{
  switch (fork()) {
    case 0: {
      close(xcb_get_file_descriptor(conn));

      {
        int devNull = open("/dev/null", O_RDONLY);
        dup2(devNull, STDIN_FILENO);
        close(devNull);
      }

      {
        int devNull = open("/dev/null", O_WRONLY);
        dup2(devNull, STDOUT_FILENO);
        dup2(devNull, STDERR_FILENO);
        close(devNull);
      }

      setsid();

      struct sigaction sa;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = 0;
      sa.sa_handler = SIG_DFL;
      sigaction(SIGCHLD, &sa, NULL);

      execvp(command[0], (char**)command);
      exit(EXIT_FAILURE);
    }
  }
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
  if (!activeChordKey) {
    activeChordKey = e->detail;
  } else {
    if (activeChordKey == dKeycode) {
      if (e->detail == fKeycode)
        spawn(termCommand);
    } else if (activeChordKey == sKeycode) {
      if (e->detail == qKeycode) {
        exit(EXIT_SUCCESS);
      }
    }

    activeChordKey = 0;
  }
}

NYLA_XEVENT_HANDLER(key_release)
{
  if (e->detail == activeChordKey) 
    activeChordKey = 0;
}

NYLA_XEVENT_HANDLER(create_notify) {}

NYLA_XEVENT_HANDLER(destroy_notify) {}

NYLA_XEVENT_HANDLER(configure_request)
{
  static int i;
  ++i;
  xcb_configure_window(
    conn, e->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, (u32[]){0, 0, screen->width_in_pixels / i, screen->height_in_pixels / i});
}

NYLA_XEVENT_HANDLER(configure_notify) {}

NYLA_XEVENT_HANDLER(client_message) {}

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

int
main()
{
  conn = xcb_connect(NULL, &iscreen);
  assert(conn && "could not connect to X server");

  screen = xcb_aux_get_screen(conn, iscreen);
  assert(screen && "could not get X screen");

  bool ok = !NYLA_XCB_CHECKED(xcb_change_window_attributes, screen->root, XCB_CW_EVENT_MASK, (u32[]){XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY});
  assert(ok && "could not change root window attributes");

  {
    xcb_key_symbols_t* syms = xcb_key_symbols_alloc(conn);

#define X(key, grabbed)                                                                                                                                                                                \
  do {                                                                                                                                                                                                 \
    xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(syms, XK_##key);                                                                                                                             \
    key##Keycode = *keycodes;                                                                                                                                                                          \
    free(keycodes);                                                                                                                                                                                    \
    if (grabbed)                                                                                                                                                                                       \
      xcb_grab_key(conn, 0, screen->root, XCB_MOD_MASK_4, key##Keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);                                                                                     \
  } while (false);
    NYLA_KEYS(X)
#undef X

    xcb_flush(conn);
    xcb_key_symbols_free(syms);
  }

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
  case event: nyla_handle_##type((void*)e); break;
        NYLA_XEVENTS(X)
#undef X
      }
    }

    xcb_flush(conn);
  }

  xcb_disconnect(conn);
}
