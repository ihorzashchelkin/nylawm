#pragma once

#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <format>
#include <iostream>
#include <iterator>
#include <print>
#include <span>
#include <thread>
#include <unordered_map>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/dri3.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GL/glcorearb.h>

namespace nyla {

struct State;

struct Keybind
{
  int mod;
  uint32_t key;
  void (*action)(State&);
};

struct Client
{
  xcb_pixmap_t pixmap;
  int16_t x;
  int16_t y;
  uint16_t width;
  uint16_t height;
  uint16_t borderWidth;
};

struct State
{
  enum
  {
    Flag_Running,
    Flag_Count
  };
  std::bitset<Flag_Count> flags;

  struct
  {
    xcb_connection_t* conn;
    xcb_screen_t* screen;
    xcb_window_t window;

#ifndef NDEBUG
    xcb_errors_context_t* errorContext;
#endif

    uint8_t pendingRequests;
  } xcb;

  struct
  {
    EGLDisplay dpy;
    EGLContext context;
    EGLSurface surface;
  } egl;

  std::vector<Keybind> keybinds;
  std::unordered_map<xcb_window_t, Client> clients;
};

const char*
initXcb(State& state, std::span<const Keybind> keybinds);

const char*
initEgl(State& state);

void
run(State& state);

void
spawn(State& state, const char* const command[]);

void
quit(State& state);

void
handleEvent(State& state, xcb_generic_event_t* event);

}
