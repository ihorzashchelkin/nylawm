#include "nyla.hpp"

namespace nyla {

static void
grabKeys(State& state)
{
  for (auto& keybind : state.keybinds) {
    XGrabKey(state.dpy.x11,
             keybind.key,
             keybind.mod,
             DefaultRootWindow(state.dpy.x11),
             0,
             GrabModeAsync,
             GrabModeAsync);
  }
  XFlush(state.dpy.x11);
}

const char*
initX11(State& state, std::span<const Keybind> keybinds)
{
  state.dpy.x11 = XOpenDisplay(nullptr);
  if (!state.dpy.x11)
    return "could not open display";

  int screen = DefaultScreen(state.dpy.x11);

  state.window = XCreateWindow(state.dpy.x11,
                               DefaultRootWindow(state.dpy.x11),
                               0,
                               0,
                               DisplayWidth(state.dpy.x11, screen),
                               DisplayHeight(state.dpy.x11, screen),
                               0,
                               CopyFromParent,
                               InputOutput,
                               CopyFromParent,
                               0,
                               nullptr);
  if (!state.window)
    return "could not create window";

  {
    XSetWindowAttributes attr{ .event_mask =
                                 SubstructureRedirectMask |
                                 SubstructureNotifyMask | PropertyChangeMask |
                                 ExposureMask | ButtonPressMask |
                                 ButtonReleaseMask | PointerMotionMask |
                                 KeyPressMask | KeyReleaseMask };
    XChangeWindowAttributes(
      state.dpy.x11, DefaultRootWindow(state.dpy.x11), CWEventMask, &attr);
  }

  XMapWindow(state.dpy.x11, state.window);
  XFlush(state.dpy.x11);

  { // TODO: improve this
    for (auto& keybind : keybinds) {
      state.keybinds.emplace_back(
        Keybind{ .mod = keybind.mod,
                 .key = XKeysymToKeycode(state.dpy.x11, keybind.key),
                 .action = keybind.action });
    }
  }

  grabKeys(state);

  state.dpy.xcb = XGetXCBConnection(state.dpy.x11);
  return nullptr;
}

}
