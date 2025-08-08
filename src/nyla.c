#include <X11/X.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xkb.h>
#include <xcb/xproto.h>

#include <GL/gl.h>
#include <GL/glx.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define NYLA_DLOG_FMT(fmt, ...)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(logFile, "%s:%d " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);                                          \
        fflush(logFile);                                                                                               \
    }                                                                                                                  \
    while (0)
#define NYLA_DLOGd(i) NYLA_DLOG_FMT("%d", i)
#define NYLA_DLOGs(s) NYLA_DLOG_FMT("%s", s)

#define NYLA_ASSERT(that, msg)                                                                                         \
    if (!that)                                                                                                         \
    {                                                                                                                  \
        NYLA_DLOGs(msg);                                                                                               \
        *((volatile int*)0) = 0;                                                                                       \
        __builtin_unreachable();                                                                                       \
    }

#define NYLA_ARRAY_LEN(arr) (sizeof(arr) / sizeof(*arr))
#define NYLA_ARRAY_END(arr) (arr + NYLA_ARRAY_LEN(arr))

#define NYLA_STR_LEN(str) (sizeof(str) - 1)

#define NYLA_ZERO(ptr) memset((char*)ptr, 0, sizeof(*ptr))
#define NYLA_ZERO_AFTER(ptr, member)                                                                                   \
    memset((char*)(ptr) + offsetof(__typeof__(*(ptr)), member) + sizeof((ptr)->member),                                \
           0,                                                                                                          \
           sizeof(*(ptr)) - offsetof(__typeof__(*(ptr)), member) - sizeof((ptr)->member));

#define NYLA_XCB_CHECKED(name, ...) xcb_request_check(conn, name##_checked(conn, __VA_ARGS__))
#define NYLA_XCB_REPLY(name, ...) name##_reply(conn, name(conn, __VA_ARGS__), NULL)
#define NYLA_XCB_REPLY_VAR(var, name, ...) name##_reply_t* var = NYLA_XCB_REPLY(name, __VA_ARGS__)

#define NYLA_UNREACHABLE NYLA_ASSERT(false && "unreachable");
#define NYLA_DEFAULT_UNREACHABLE                                                                                       \
    default: NYLA_UNREACHABLE

typedef struct
{
    union
    {
        u64 raw;
        struct
        {
            i16 x;
            i16 y;
            u16 width;
            u16 height;
        };
    };
} NylaClientGeom;

typedef struct
{
    xcb_window_t win;
    u32 flags;
    NylaClientGeom oldGeom;
    NylaClientGeom curGeom;
} NylaClient;

#define NYLA_FOREACH(elem, start, end) for (__typeof__(*(start))* elem = (start); elem != (end); ++elem)

#define NYLA_FOREACH_IF(elem, start, end, condition)                                                                   \
    NYLA_FOREACH (elem, start, end)                                                                                    \
        if (condition)

#define NYLA_ATOMS(X)                                                                                                  \
    X(UTF8_STRING)                                                                                                     \
    X(WM_NAME)                                                                                                         \
    X(_NET_WM_NAME)

#define X(atom) static xcb_atom_t ATOM_##atom;
NYLA_ATOMS(X)
#undef X

#define NYLA_XEVENT_HANDLER(type) static void nyla_handle_##type(xcb_##type##_event_t* e)
#define NYLA_XEVENTS(X)                                                                                                \
    X(XCB_CREATE_NOTIFY, create_notify)                                                                                \
    X(XCB_DESTROY_NOTIFY, destroy_notify)                                                                              \
    X(XCB_CONFIGURE_REQUEST, configure_request)                                                                        \
    X(XCB_CONFIGURE_NOTIFY, configure_notify)                                                                          \
    X(XCB_MAP_REQUEST, map_request)                                                                                    \
    X(XCB_MAP_NOTIFY, map_notify)                                                                                      \
    X(XCB_MAPPING_NOTIFY, mapping_notify)                                                                              \
    X(XCB_UNMAP_NOTIFY, unmap_notify)                                                                                  \
    X(XCB_CLIENT_MESSAGE, client_message)                                                                              \
    X(XCB_KEY_PRESS, key_press)                                                                                        \
    X(XCB_KEY_RELEASE, key_release)                                                                                    \
    X(XCB_FOCUS_IN, focus_in)                                                                                          \
    X(XCB_FOCUS_OUT, focus_out)

// Eine Sache über den X11 passive key grab: jederzeit eine passiv
// gegrabbte Taste gedrückt ist, ist die ganze Tastatur aktiv gegrabbt.

// clang-format off
// 1 bedeutet hier, dass die Taste gegrabbt wird
#define NYLA_KEYS(X)                                                           \
  X(q, 0) X(w, 0) X(e, 0) X(r, 0) X(t, 0)                                      \
  X(a, 0) X(s, 1) X(d, 1) X(f, 0) X(g, 0)                                      \
  X(z, 0) X(x, 0) X(c, 0) X(v, 0) X(b, 0)
// clang-format on

#define X(key, grabbed) static xcb_keycode_t key##Keycode;
NYLA_KEYS(X)
#undef X

static const char* termCommand[] = {"ghostty", NULL};

static const char** argv;
static FILE* logFile;
static Display* dpy;
static xcb_connection_t* conn;
static xcb_screen_t* screen;

static NylaClient clients[64];
static NylaClient* activeClient;
static xcb_keycode_t chordKey;
static xcb_window_t overlayWindow;

char*
nyla_get_window_name(xcb_window_t win)
{
    NYLA_XCB_REPLY_VAR(reply, xcb_get_property, 0, win, ATOM__NET_WM_NAME, ATOM_UTF8_STRING, 0, 64);
    if (!reply)
        return NULL;

    int len = xcb_get_property_value_length(reply);
    if (!len)
    {
        free(reply);
        reply = NYLA_XCB_REPLY(xcb_get_property, 0, win, ATOM_WM_NAME, XCB_ATOM_STRING, 0, 64);
        if (!reply)
            return NULL;

        len = xcb_get_property_value_length(reply);
        if (!len)
        {
            free(reply);
            return NULL;
        }
    }

    char* name = malloc(len + 1);
    memcpy(name, xcb_get_property_value(reply), len);
    name[len] = '\0';
    free(reply);
    return name;
}

void
nyla_spawn(const char* const command[])
{
    if (fork() != 0)
        return;

    close(xcb_get_file_descriptor(conn));

    int devNull = open("/dev/null", O_RDONLY);
    dup2(devNull, STDIN_FILENO);
    close(devNull);

    devNull = open("/dev/null", O_WRONLY);
    dup2(devNull, STDOUT_FILENO);
    dup2(devNull, STDERR_FILENO);
    close(devNull);

    setsid();

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &sa, NULL);

    execvp(command[0], (char**)command);
    exit(EXIT_FAILURE);
}

void
nyla_restart()
{
    close(xcb_get_file_descriptor(conn));
    execv(argv[0], (char**)argv);
    exit(EXIT_FAILURE);
}

void
nyla_focus_window(xcb_connection_t* conn, xcb_window_t window)
{
    xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
}

void
nyla_map_keyboard()
{
    xcb_key_symbols_t* syms = xcb_key_symbols_alloc(conn);

#define X(key, isGrabbed)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(syms, XK_##key);                                         \
        key##Keycode = *keycodes;                                                                                      \
        free(keycodes);                                                                                                \
        if (isGrabbed)                                                                                                 \
            xcb_grab_key(                                                                                              \
              conn, 0, screen->root, XCB_MOD_MASK_4, key##Keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);          \
    }                                                                                                                  \
    while (false);
    NYLA_KEYS(X)
#undef X

    xcb_key_symbols_free(syms);
}

NYLA_XEVENT_HANDLER(map_request) {}

NYLA_XEVENT_HANDLER(map_notify) {}

NYLA_XEVENT_HANDLER(unmap_notify) {}

NYLA_XEVENT_HANDLER(key_press)
{
    if (!chordKey)
    {
        chordKey = e->detail;
        return;
    }
    if (e->detail == chordKey)
        return;

    if (chordKey == dKeycode)
    {
        if (e->detail == fKeycode)
        {
            if (activeClient)
            {
                NYLA_FOREACH_IF (client, activeClient + 1, NYLA_ARRAY_END(clients), client->win)
                {
                    NYLA_DLOG_FMT("using %lu", (u64)client);

                    xcb_map_window(conn, client->win);
                    nyla_focus_window(conn, client->win);
                    xcb_configure_window(conn,
                                         client->win,
                                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                                           XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
                                         (u32[]){screen->width_in_pixels / 4,
                                                 0,
                                                 screen->width_in_pixels / 2,
                                                 screen->height_in_pixels,
                                                 XCB_STACK_MODE_ABOVE});

                    if (activeClient)
                    {
                        xcb_configure_window(conn,
                                             activeClient->win,
                                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                                               XCB_CONFIG_WINDOW_HEIGHT,
                                             (i32[]){-screen->width_in_pixels / 4 * 0,
                                                     0,
                                                     screen->width_in_pixels / 2,
                                                     screen->height_in_pixels});
                    }

                    activeClient = client;
                    return;
                }

                NYLA_FOREACH_IF (client, clients, activeClient, client->win)
                {
                    NYLA_DLOG_FMT("using %lu", (u64)client);

                    xcb_map_window(conn, client->win);
                    nyla_focus_window(conn, client->win);
                    xcb_configure_window(conn,
                                         client->win,
                                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                                           XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
                                         (u32[]){screen->width_in_pixels / 4,
                                                 0,
                                                 screen->width_in_pixels / 2,
                                                 screen->height_in_pixels,
                                                 XCB_STACK_MODE_ABOVE});

                    if (activeClient)
                    {
                        xcb_configure_window(conn,
                                             activeClient->win,
                                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                                               XCB_CONFIG_WINDOW_HEIGHT,
                                             (i32[]){-screen->width_in_pixels / 4 * 0,
                                                     0,
                                                     screen->width_in_pixels / 2,
                                                     screen->height_in_pixels});
                    }

                    activeClient = client;
                    return;
                }
            }
            else
            {
                NYLA_FOREACH_IF (client, clients, NYLA_ARRAY_END(clients), client->win && client != activeClient)
                {
                    NYLA_DLOG_FMT("using %lu", (u64)client);

                    xcb_map_window(conn, client->win);
                    nyla_focus_window(conn, client->win);
                    xcb_configure_window(conn,
                                         client->win,
                                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                                           XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
                                         (u32[]){screen->width_in_pixels / 4,
                                                 0,
                                                 screen->width_in_pixels / 2,
                                                 screen->height_in_pixels,
                                                 XCB_STACK_MODE_ABOVE});

                    if (activeClient)
                    {
                        xcb_configure_window(
                          conn,
                          activeClient->win,
                          XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                            XCB_CONFIG_WINDOW_HEIGHT,
                          (u32[]){
                            screen->width_in_pixels / 4 * 3, 0, screen->width_in_pixels / 2, screen->height_in_pixels});
                    }

                    activeClient = client;
                    break;
                }
            }

            return;
        }

        if (e->detail == tKeycode)
        {
            nyla_spawn(termCommand);
            return;
        }

        return;
    }

    if (chordKey == sKeycode)
    {
        if (e->detail == rKeycode)
            nyla_restart();

        if (e->detail == qKeycode)
            exit(EXIT_SUCCESS);

        return;
    }
}

NYLA_XEVENT_HANDLER(key_release)
{
    if (e->detail == chordKey)
        chordKey = 0;
}

NYLA_XEVENT_HANDLER(create_notify)
{
    if (e->parent != screen->root || e->override_redirect)
        return;

    NYLA_FOREACH_IF (client, clients, NYLA_ARRAY_END(clients), !client->win)
    {
        client->win = e->window;
        NYLA_ZERO_AFTER(client, win);

#if 0
        xcb_map_window(conn, client->window);
        nyla_focus_window(conn, client->window);
        xcb_configure_window(conn,
                             client->window,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
                             (u32[]){screen->width_in_pixels / 4, 0, screen->width_in_pixels / 2, screen->height_in_pixels, XCB_STACK_MODE_ABOVE});

        if (activeClient)
        {
            xcb_configure_window(conn,
                                 activeClient->window,
                                 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                                 (u32[]){screen->width_in_pixels / 4 * 3, 0, screen->width_in_pixels / 2, screen->height_in_pixels});
        }

        activeClient = client;
#endif
        break;
    }
}

NYLA_XEVENT_HANDLER(destroy_notify)
{
    NYLA_DLOG_FMT("removing %dl", e->window);

    NYLA_FOREACH_IF (client, clients, NYLA_ARRAY_END(clients), client->win == e->window)
    {
        NYLA_ZERO(client);
        break;
    }
}

NYLA_XEVENT_HANDLER(configure_request) {}

NYLA_XEVENT_HANDLER(configure_notify) {}

NYLA_XEVENT_HANDLER(client_message) {}

NYLA_XEVENT_HANDLER(mapping_notify)
{
    // TODO: only if keyboard mapping
    nyla_map_keyboard();
}

NYLA_XEVENT_HANDLER(focus_in)
{
#if 0
    if (activeClient)
        nyla_focus_window(conn, activeClient->window);
#endif
}

NYLA_XEVENT_HANDLER(focus_out)
{
#if 0
    if (activeClient)
        nyla_focus_window(conn, activeClient->window);
#endif
}

int
main(int argc, const char* _argv[])
{
    argv = _argv; // argv ist NULL terminated

    {
        logFile = stderr;
        int logFileFd = open("/home/izashchelkin/nylalog", O_APPEND | O_TRUNC | O_WRONLY | O_CREAT);
        NYLA_ASSERT(logFileFd, "could not open log file");
        FILE* tmp = fdopen(logFileFd, "a");
        NYLA_ASSERT(tmp, "could not open log file");
        logFile = tmp;
    }

    {
        dpy = XOpenDisplay(NULL);
        NYLA_ASSERT(dpy, "could not connect to X server");

        conn = XGetXCBConnection(dpy);
        NYLA_ASSERT(conn, "could not connect get xcb connection");

        XSetEventQueueOwner(dpy, XCBOwnsEventQueue);

        screen = xcb_aux_get_screen(conn, DefaultScreen(dpy));
        NYLA_ASSERT(screen, "could not get X screen");

        bool ok = !NYLA_XCB_CHECKED(xcb_change_window_attributes,
                                    screen->root,
                                    XCB_CW_EVENT_MASK,
                                    (u32[]){XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                                            XCB_EVENT_MASK_FOCUS_CHANGE});
        NYLA_ASSERT(ok, "could not change root window attributes");

#define X(name)                                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        NYLA_XCB_REPLY_VAR(r, xcb_intern_atom, false, NYLA_STR_LEN(#name), #name);                                     \
        NYLA_ASSERT(r, "could not intern " #name);                                                                     \
        ATOM_##name = r->atom;                                                                                         \
        free(r);                                                                                                       \
    }                                                                                                                  \
    while (0);
        NYLA_ATOMS(X)
#undef X
    }

    nyla_map_keyboard();

    {
        NYLA_XCB_REPLY_VAR(reply, xcb_query_tree, screen->root);

        NylaClient* client = clients;
        xcb_window_t* it = xcb_query_tree_children(reply);

        for (int i = 0; i < xcb_query_tree_children_length(reply); ++i, ++it)
        {
            xcb_window_t win = *it;

            NYLA_DLOG_FMT("scan found: %d", win);

            NYLA_XCB_REPLY_VAR(reply, xcb_get_window_attributes, win);
            if (!reply)
                continue;
            if (reply->override_redirect)
            {
                free(reply);
                continue;
            }
            free(reply);

            (client++)->win = win;

            char* name = nyla_get_window_name(win);
            if (name)
            {
                NYLA_DLOGs(name);
                free(name);
            }
            else
            {
                NYLA_DLOGs("name is null!");
            }

            xcb_unmap_window(conn, *it);
        }
    }

    {
        overlayWindow = xcb_generate_id(conn);
        bool ok = !NYLA_XCB_CHECKED(xcb_create_window,
                                    XCB_COPY_FROM_PARENT,
                                    overlayWindow,
                                    screen->root,
                                    0,
                                    0,
                                    screen->width_in_pixels,
                                    screen->height_in_pixels,
                                    0,
                                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                    XCB_COPY_FROM_PARENT,
                                    0,
                                    (u32[]){});
        NYLA_ASSERT(ok, "could not create overlay window");

        // glXChooseFBConfig(dpy, DefaultScreen(dpy), const int* attribList, int* nitems);
    }

    while (true)
    {
        xcb_flush(conn);
        xcb_generic_event_t* e = xcb_wait_for_event(conn);
        do
        {
            switch (e->response_type & ~0x80)
            {
#define X(_event, _type)                                                                                               \
    case _event: nyla_handle_##_type((void*)e); break;
                NYLA_XEVENTS(X)
#undef X
                case 0:
                {
                    xcb_generic_error_t* error = (xcb_generic_error_t*)e;
                    NYLA_DLOG_FMT("ERROR! %d", error->error_code);
                    break;
                }
            }

            e = xcb_poll_for_event(conn);
        }
        while (e);
    }
}
