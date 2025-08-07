#pragma once

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xkb.h>
#include <xcb/xproto.h>

#include <X11/keysym.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define NYLA_DLOG_FMT(fmt, ...)                                                \
  do {                                                                         \
    fprintf(logFile, "%s:%d " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);      \
    fflush(logFile);                                                           \
  } while (0)
#define NYLA_DLOGd(i) NYLA_DLOG_FMT("%d", i)
#define NYLA_DLOGs(s) NYLA_DLOG_FMT("%s", s)

#define NYLA_ASSERT(that, msg)                                                 \
  if (!that) {                                                                 \
    NYLA_DLOGs(#that " failed");                                               \
    *((volatile int*)0) = 0;                                                   \
    __builtin_unreachable();                                                   \
  }

#define NYLA_ARRAY_LEN(arr) (sizeof(arr) / sizeof(*arr))
#define NYLA_ARRAY_END(arr) (arr + NYLA_ARRAY_LEN(arr))

#define NYLA_ZERO(ptr) memset((char*)ptr, 0, sizeof(*ptr))
#define NYLA_ZERO_AFTER(ptr, member)                                           \
  memset((char*)(ptr) + offsetof(__typeof__(*(ptr)), member) +                 \
           sizeof((ptr)->member),                                              \
         0,                                                                    \
         sizeof(*(ptr)) - offsetof(__typeof__(*(ptr)), member) -               \
           sizeof((ptr)->member));

#define NYLA_XCB_CHECKED(name, ...)                                            \
  xcb_request_check(conn, name##_checked(conn, __VA_ARGS__))
#define NYLA_XCB_REPLY(name, ...)                                              \
  name##_reply(conn, name(conn, __VA_ARGS__), NULL)

#define NYLA_UNREACHABLE NYLA_ASSERT(false && "unreachable");
#define NYLA_DEFAULT_UNREACHABLE                                               \
  default:                                                                     \
    NYLA_UNREACHABLE

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
  xcb_window_t window;
  u32 flags;
  NylaClientGeom oldGeom;
  NylaClientGeom curGeom;
} NylaClient;

#define NYLA_FOREACH(elem, start, end)                                         \
  for (__typeof__(*(start))* elem = (start); elem != (end); ++elem)

#define NYLA_FOREACH_IF(elem, start, end, condition)                           \
  NYLA_FOREACH(elem, start, end)                                               \
  if (condition)

#define NYLA_XEVENT_HANDLER(type)                                              \
  static void nyla_handle_##type(xcb_##type##_event_t* e)
#define NYLA_XEVENTS(X)                                                        \
  X(XCB_CREATE_NOTIFY, create_notify)                                          \
  X(XCB_DESTROY_NOTIFY, destroy_notify)                                        \
  X(XCB_CONFIGURE_REQUEST, configure_request)                                  \
  X(XCB_CONFIGURE_NOTIFY, configure_notify)                                    \
  X(XCB_MAP_REQUEST, map_request)                                              \
  X(XCB_MAP_NOTIFY, map_notify)                                                \
  X(XCB_MAPPING_NOTIFY, mapping_notify)                                        \
  X(XCB_UNMAP_NOTIFY, unmap_notify)                                            \
  X(XCB_CLIENT_MESSAGE, client_message)                                        \
  X(XCB_KEY_PRESS, key_press)                                                  \
  X(XCB_KEY_RELEASE, key_release)                                              \
  X(XCB_FOCUS_IN, focus_in)                                                    \
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

void
run();
