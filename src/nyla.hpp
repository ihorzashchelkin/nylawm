#pragma once

#include <iterator>

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/X.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glcorearb.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define debug_fmt(fmt, ...) dprintf(logFd, "[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);
#define debug_fmtd(i) debug_fmt("%d", i)
#define debug_fmts(s) debug_fmt("%s", s)

#define assert(that)                                                                                                   \
    if (!(that))                                                                                                       \
    {                                                                                                                  \
        debug_fmts(#that);                                                                                             \
        *((volatile int*)0) = 0;                                                                                       \
        __builtin_unreachable();                                                                                       \
    }

#define xor_swap(a, b)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        (a) = (a) ^ (b);                                                                                               \
        (b) = (a) ^ (b);                                                                                               \
        (a) = (a) ^ (b);                                                                                               \
    }                                                                                                                  \
    while (0)

#define zero(ptr) memset((char*)ptr, 0, sizeof(*ptr))
#define zero_after(ptr, member)                                                                                        \
    memset((char*)(ptr) + offsetof(__typeof__(*(ptr)), member) + sizeof((ptr)->member),                                \
           0,                                                                                                          \
           sizeof(*(ptr)) - offsetof(__typeof__(*(ptr)), member) - sizeof((ptr)->member));

#define xcb_checked(name, ...) xcb_request_check(conn, name##_checked(conn, __VA_ARGS__))
#define xcb_reply(name, ...) name##_reply(conn, name(conn, __VA_ARGS__), NULL)
#define xcb_reply_var(var, name, ...) name##_reply_t* var = xcb_reply(name, __VA_ARGS__)

// clang-format off
#define KEYS(X) \
    X(q, 0) X(w, 0) X(e, 0) X(r, 0) X(t, 0) \
    X(a, 0) X(s, 1) X(d, 1) X(f, 0) X(g, 0) \
    X(z, 0) X(x, 0) X(c, 0) X(v, 0) X(b, 0)
// clang-format on

extern int logFd;
extern Display* dpy;
extern xcb_connection_t* conn;
extern xcb_window_t overlayWindow;
extern xcb_screen_t* screen;
