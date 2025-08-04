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

#include <X11/X.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/keysym.h>
#include <xcb/composite.h>
#include <xcb/dri3.h>
#include <xcb/xcb.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GL/glcorearb.h>

namespace nyla {

struct State;

struct Keybind
{
  unsigned int mod;
  int key;
  void (*action)(State&);
};

struct Client
{
  Pixmap pixmap;
  int x;
  int y;
  int width;
  int height;
  int borderWidth;
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
    Display* x11;
    EGLDisplay egl;
    xcb_connection_t* xcb;
  } dpy;

  Window window;

  struct
  {
    EGLContext context;
    EGLSurface surface;
  } egl;

  std::vector<Keybind> keybinds;
  std::unordered_map<Window, Client> clients;
};

const char*
initX11(State& state, std::span<const Keybind> keybinds);

const char*
initEgl(State& state);

void
run(State& state);

void
spawn(State& state, const char* const command[]);

void
quit(State& state);

void
handleEvent(State& state, XEvent* event);

}
