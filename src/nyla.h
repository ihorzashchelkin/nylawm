#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
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

#define NYLA_LEN(arr) (sizeof(arr) / sizeof(*arr))
#define NYLA_END(arr) (arr + NYLA_LEN(arr))

#define NYLA_DLOG_FMT(fmt, ...)                                                \
  printf("%s:%d " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);
#define NYLA_DLOGd(i) NYLA_DLOG_FMT("%d", i)
#define NYLA_DLOGs(s) NYLA_DLOG_FMT("%s", s)

#define NYLA_XCB_CHECKED(name, ...)                                            \
  xcb_request_check(conn, name##_checked(conn, __VA_ARGS__))

#define NYLA_UNREACHABLE                                                       \
  do {                                                                         \
    __builtin_unreachable();                                                   \
    assert(false && "unreachable");                                            \
  } while (0);
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

#define NYLA_FOREACH_CLIENTS(client)                                           \
  for (NylaClient* client = clients; clients != clientsEnd; ++clients)
#define NYLA_XEVENT_HANDLER(type)                                              \
  static void nyla_handle_##type(xcb_##type##_event_t* e,                      \
                                 xcb_connection_t* conn,                       \
                                 NylaClient* clients,                          \
                                 NylaClient* clientsEnd)
#define NYLA_XEVENTS(X)                                                        \
  X(XCB_CREATE_NOTIFY, create_notify)                                          \
  X(XCB_DESTROY_NOTIFY, destroy_notify)                                        \
  X(XCB_CONFIGURE_REQUEST, configure_request)                                  \
  X(XCB_CONFIGURE_NOTIFY, configure_notify)                                    \
  X(XCB_MAP_REQUEST, map_request)                                              \
  X(XCB_MAP_NOTIFY, map_notify)                                                \
  X(XCB_UNMAP_NOTIFY, unmap_notify)                                            \
  X(XCB_KEY_PRESS, key_press)                                                  \
  X(XCB_CLIENT_MESSAGE, client_message)

// clang-format off
#define NYLA_KEYS(X)                                                           \
  X(q) X(w) X(e) X(r) X(t)                                                     \
  X(a) X(s) X(d) X(f) X(g)                                                     \
  X(z) X(x) X(c) X(v) X(b)
// clang-format on

void
run();
