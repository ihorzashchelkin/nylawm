#include "nyla.h"
#include <xcb/xproto.h>

void
nyla_spawn(xcb_connection_t* conn, const char* const command[])
{
    if (fork() == 0)
    {
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
}

void
nyla_restart(xcb_connection_t* conn, const char* const command[])
{
    close(xcb_get_file_descriptor(conn));
    execv(command[0], (char**)command);
    exit(EXIT_FAILURE);
}

void
nyla_focus_window(xcb_connection_t* conn, xcb_window_t window)
{
    xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
}
