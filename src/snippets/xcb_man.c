
/*
 * Request without a reply, handling errors in the event loop (default)
 *
 */
void my_example(xcb_connection* conn, xcb_window_t window)
{
    /* This is a request without a reply. Errors will be delivered to the event
     * loop. Getting an error to xcb_map_window most likely is a bug in our
     * program, so we don’t need to check for that in a blocking way. */
    xcb_map_window(conn, window);

    /* ... of course your event loop would not be in the same function ... */
    while ((event = xcb_wait_for_event(conn)) != NULL) {
        if (event->response_type == 0) {
            fprintf("Received X11 error %d\n", error->error_code);
            free(event);
            continue;
        }

        /* ... handle a normal event ... */
    }
}

/*
 * Request without a reply, handling errors directly
 *
 */
void my_example(xcb_connection* conn, xcb_window_t deco, xcb_window_t window)
{
    /* A reparenting window manager wants to know whether a new window was
     * successfully reparented. If not (because the window got destroyed
     * already, for example), it does not make sense to map an empty window
     * decoration at all, so we need to know this right now. */
    xcb_void_cookie_t cookie = xcb_reparent_window_checked(conn, window,
        deco, 0, 0);
    xcb_generic_error_t* error;
    if ((error = xcb_request_check(conn, cookie))) {
        fprintf(stderr, "Could not reparent the window\n");
        free(error);
        return;
    }

    /* ... do window manager stuff here ... */
}

/*
 * Request with a reply, handling errors directly (default)
 *
 */
void my_example(xcb_connection* conn, xcb_window_t window)
{
    xcb_intern_atom_cookie_t cookie;
    xcb_intern_atom_reply_t* reply;
    xcb_generic_error_t* error;

    cookie = xcb_intern_atom(c, 0, strlen("_NET_WM_NAME"), "_NET_WM_NAME");
    /* ... do other work here if possible ... */
    if ((reply = xcb_intern_atom_reply(c, cookie, &error))) {
        printf("The _NET_WM_NAME atom has ID %u0", reply->atom);
        free(reply);
    } else {
        fprintf(stderr, "X11 Error %d\n", error->error_code);
        free(error);
    }
}

/*
 * Request with a reply, handling errors in the event loop
 *
 */
void my_example(xcb_connection* conn, xcb_window_t window)
{
    xcb_intern_atom_cookie_t cookie;
    xcb_intern_atom_reply_t* reply;

    cookie = xcb_intern_atom_unchecked(c, 0, strlen("_NET_WM_NAME"),
        "_NET_WM_NAME");
    /* ... do other work here if possible ... */
    if ((reply = xcb_intern_atom_reply(c, cookie, NULL))) {
printf("The _NET_WM_NAME atom has ID %u0, reply->atom);
free(reply);
    }

    /* ... of course your event loop would not be in the same function ... */
    while ((event = xcb_wait_for_event(conn)) != NULL) {
        if (event->response_type == 0) {
            fprintf("Received X11 error %d\n", error->error_code);
            free(event);
            continue;
        }

        /* ... handle a normal event ... */
    }
}
