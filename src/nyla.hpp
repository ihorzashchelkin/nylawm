#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <print>
#include <unordered_map>
#include <vector>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

namespace nyla {

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef const char* cstr;

#define nil nullptr;

#define LOG(arg) std::println(std::cerr, "{}:{} {}", __FILE__, __LINE__, arg)

#define XCB_CHECKED(NAME, ...)                                                 \
  xcb_request_check(conn, NAME##_checked(conn, __VA_ARGS__))

struct Client
{
  u8 itile;
};

struct WindowManager
{
  xcb_connection_t* conn;
  xcb_screen_t* screen;
  std::unordered_map<xcb_window_t, Client> clients;
  std::array<xcb_window_t, 4> tileAssignments;
};

void
run();

}
