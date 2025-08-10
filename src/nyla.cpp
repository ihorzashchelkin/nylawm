#include "nyla.hpp"
#include "overlay.cpp"

enum
{
    ClientFlagMapped = 1
};

typedef struct
{
    xcb_window_t win;
    u32 flags;
} Client;

#define ATOMS(X)                                                                                                       \
    X(UTF8_STRING)                                                                                                     \
    X(WM_NAME)                                                                                                         \
    X(_NET_WM_NAME)

#define X(atom) static xcb_atom_t ATOM_##atom;
ATOMS(X)
#undef X

#define HANDLER(type) static void nyla_handle_##type(xcb_##type##_event_t* e)
#define EVENTS(X)                                                                                                      \
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

// clang-format off
#define KEYS(X) \
    X(q, 0) X(w, 0) X(e, 0) X(r, 0) X(t, 0) \
    X(a, 0) X(s, 1) X(d, 1) X(f, 0) X(g, 0) \
    X(z, 0) X(x, 0) X(c, 0) X(v, 0) X(b, 0)
// clang-format on

#define X(key, grabbed) static xcb_keycode_t key##Keycode;
KEYS(X)
#undef X

static const char* termCommand[] = {"ghostty", NULL};

static const char** argv;
int logFd;
Display* dpy;
xcb_connection_t* conn;
xcb_screen_t* screen;

static Client clients[64];
static xcb_keycode_t chordKey;
xcb_window_t overlayWindow;
static u8 activeCol = 0;

enum
{
    ActiveClientUnset = 0xFF
};
static u8 activeClients[2];

char* get_window_name(xcb_window_t win)
{
    xcb_reply_var(reply, xcb_get_property, 0, win, ATOM__NET_WM_NAME, ATOM_UTF8_STRING, 0, 64);
    if (!reply)
        return NULL;

    int len = xcb_get_property_value_length(reply);
    if (!len)
    {
        free(reply);
        reply = xcb_reply(xcb_get_property, 0, win, ATOM_WM_NAME, XCB_ATOM_STRING, 0, 64);
        if (!reply)
            return NULL;

        len = xcb_get_property_value_length(reply);
        if (!len)
        {
            free(reply);
            return NULL;
        }
    }

    char* name = new char[len + 1];
    memcpy(name, xcb_get_property_value(reply), len);
    name[len] = '\0';
    free(reply);
    return name;
}

void spawn(const char* const command[])
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

void restart()
{
    close(xcb_get_file_descriptor(conn));
    execv(argv[0], (char**)argv);
    exit(EXIT_FAILURE);
}

void focus_window(xcb_window_t window)
{
    xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
    xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_BORDER_WIDTH, (u32[]){2});
}

void map_keyboard()
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
                conn, 0, screen->root, XCB_MOD_MASK_4, key##Keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);        \
    }                                                                                                                  \
    while (false);
    KEYS(X)
#undef X

    xcb_key_symbols_free(syms);
}

void arrange()
{
    for (uint i = 0; i < std::size(activeClients); ++i)
    {
        if (!activeClients[i])
            continue;

        Client* client = clients + activeClients[i];
        xcb_configure_window(conn,
                             client->win,
                             (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                              XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE),
                             (u32[]){screen->width_in_pixels / 2 * i,
                                     0,
                                     (u32)(screen->width_in_pixels / 2),
                                     screen->height_in_pixels,
                                     XCB_STACK_MODE_ABOVE});
        xcb_map_window(conn, client->win);
        focus_window(client->win);
    }
}

u8 is_client_active(int clientIndex)
{
    for (uint i = 0; i < std::size(activeClients); ++i)
        if (activeClients[i] == clientIndex)
            return true;
    return false;
}

HANDLER(map_request)
{
    xcb_map_window(conn, e->window);
}

HANDLER(map_notify)
{
    if (e->override_redirect)
        return;

    for (auto& client : clients)
    {
        if (client.win == e->window)
        {
            client.flags |= ClientFlagMapped;
            break;
        }
    }
}

HANDLER(unmap_notify)
{
    for (uint i = 0; i < std::size(clients); ++i)
    {
        Client* client = clients + i;

        if (client->win == e->window)
        {
            client->flags &= ~ClientFlagMapped;

            for (uint j = 0; j < std::size(activeClients); ++j)
            {
                if (activeClients[j] == i)
                {
                    activeClients[j] = 0;
                    break;
                }
            }
        }
    }
}

HANDLER(key_press)
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
        if (e->detail == xKeycode)
        {
            xor_swap(activeClients[0], activeClients[1]);
            arrange();
            return;
        }

        if (e->detail == fKeycode)
        {
            for (uint i = 0; i < std::size(clients); ++i)
            {
                Client* client = clients + i;
                if (client->win)
                {
                    if (!is_client_active(i))
                    {
                        activeClients[activeCol] = i;
                        arrange();
                    }

                    break;
                }
            }

            return;
        }

        if (e->detail == tKeycode)
        {
            spawn(termCommand);
            return;
        }

        return;
    }

    if (chordKey == sKeycode)
    {
        if (e->detail == fKeycode)
        {
            activeCol = !activeCol;
            if (activeClients[activeCol])
                focus_window(clients[activeClients[activeCol]].win);

            return;
        }

        if (e->detail == rKeycode)
            restart();

        if (e->detail == qKeycode)
            exit(EXIT_SUCCESS);

        return;
    }
}

HANDLER(key_release)
{
    if (e->detail == chordKey)
        chordKey = 0;
}

HANDLER(create_notify)
{
    if (e->override_redirect || e->parent != screen->root)
        return;

    for (auto& client : clients)
    {
        if (!client.win)
        {
            client.win = e->window;
            zero_after(&client, win);
            break;
        }
    }
}

HANDLER(destroy_notify)
{
    debug_fmt("removing %dl", e->window);

    for (auto& client : clients)
    {
        if (client.win == e->window)
        {
            zero(&client);
            break;
        }
    }
}

int get_client_index(xcb_window_t win)
{
    int i = 0;
    for (auto& client : clients)
    {
        if (client.win == win)
            return i;
        i++;
    }
    return -1;
}

HANDLER(configure_request)
{
    if (is_client_active(get_client_index(e->window)))
    {
        arrange();
    }
    else
    {
        xcb_configure_window(conn,
                             e->window,
                             (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                              XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE),
                             (u32[]){screen->width_in_pixels, 0, 20, 20, XCB_STACK_MODE_BELOW});
    }
}

HANDLER(configure_notify) {}

HANDLER(client_message) {}

HANDLER(mapping_notify)
{
    // TODO: only if keyboard mapping
    map_keyboard();
}

HANDLER(focus_in)
{
#if 0
    if (activeClient)
        nyla_focus_window(conn, activeClient->window);
#endif
}

HANDLER(focus_out)
{
    xcb_configure_window(conn, e->event, XCB_CONFIG_WINDOW_BORDER_WIDTH, (u32[]){0});

#if 0
    if (activeClient)
        nyla_focus_window(conn, activeClient->window);
#endif
}

int main(int argc, const char* _argv[])
{
    argv = _argv;
    memset(activeClients, ActiveClientUnset, std::size(activeClients));

    logFd = open("/home/izashchelkin/nylalog", O_APPEND | O_TRUNC | O_WRONLY | O_CREAT);
    if (!logFd)
    {
        logFd = stderr->_fileno;
        assert(false);
    }

    {
        dpy = XOpenDisplay(NULL);
        assert(dpy && "could not connect to X server");

        conn = XGetXCBConnection(dpy);
        assert(conn && "could not connect get xcb connection");

        XSetEventQueueOwner(dpy, XCBOwnsEventQueue);

        screen = xcb_aux_get_screen(conn, DefaultScreen(dpy));
        assert(screen && "could not get X screen");

        bool ok = !xcb_checked(xcb_change_window_attributes,
                               screen->root,
                               XCB_CW_EVENT_MASK,
                               (u32[]){XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                                       XCB_EVENT_MASK_FOCUS_CHANGE});
        assert(ok && "could not change root window attributes");

#define X(name)                                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        xcb_reply_var(r, xcb_intern_atom, false, std::size(#name) - 1, #name);                                         \
        assert(r && "could not intern " #name);                                                                        \
        ATOM_##name = r->atom;                                                                                         \
        free(r);                                                                                                       \
    }                                                                                                                  \
    while (0);
        ATOMS(X)
#undef X
    }

    map_keyboard();

    {
        xcb_reply_var(reply, xcb_query_tree, screen->root);

        Client* client = clients;
        xcb_window_t* it = xcb_query_tree_children(reply);

        for (int i = 0; i < xcb_query_tree_children_length(reply); ++i, ++it)
        {
            xcb_window_t win = *it;

            debug_fmt("scan found: %d", win);

            xcb_reply_var(reply, xcb_get_window_attributes, win);
            if (!reply)
                continue;
            if (reply->override_redirect)
            {
                free(reply);
                continue;
            }
            free(reply);

            (client++)->win = win;

            char* name = get_window_name(win);
            if (name)
            {
                debug_fmts(name);
                free(name);
            }
            else
            {
                debug_fmts("name is null!");
            }

            xcb_unmap_window(conn, *it);
        }
    }

    initOverlay();

    while (true)
    {
        xcb_flush(conn);
        xcb_generic_event_t* e = xcb_wait_for_event(conn);
        do
        {
            switch (e->response_type & ~0x80)
            {
#define X(_event, _type)                                                                                               \
    case _event: nyla_handle_##_type((xcb_##_type##_event_t*)(void*)e); break;
                EVENTS(X)
#undef X
                case 0:
                {
                    xcb_generic_error_t* error = (xcb_generic_error_t*)e;
                    debug_fmt("ERROR! %d", error->error_code);
                    break;
                }
            }

            e = xcb_poll_for_event(conn);
        }
        while (e);
    }
}
