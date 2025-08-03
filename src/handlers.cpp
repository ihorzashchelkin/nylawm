#include "src/nyla.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <cstdint>

namespace nyla {

static void
handleConfigureRequest(State& state, XConfigureRequestEvent& event)
{
#ifndef NDEBUG
  std::println(std::clog, "ConfigureRequest from {}", event.window);
#endif

  uint32_t changesMask = 0;
  XWindowChanges changes{};

  if (event.value_mask & CWX) {
    changesMask |= CWX;
    changes.x = event.x;
  }
  if (event.value_mask & CWY) {
    changesMask |= CWY;
    changes.y = event.y;
  }
  if (event.value_mask & CWWidth) {
    changesMask |= CWWidth;
    changes.width = event.width;
  }
  if (event.value_mask & CWHeight) {
    changesMask |= CWHeight;
    changes.height = event.height;
  }
  if (event.value_mask & CWBorderWidth) {
    changesMask |= CWBorderWidth;
    changes.border_width = event.border_width;
  }
  if (event.value_mask & CWSibling) {
    changesMask |= CWSibling;
    changes.sibling = event.above;
  }
  if (event.value_mask & CWStackMode) {
    changesMask |= CWStackMode;
    changes.stack_mode = event.detail;
  }

  XConfigureWindow(state.dpy.x11, event.window, changesMask, &changes);
  XFlush(state.dpy.x11);
}

static void
handleConfigureNotify(State& state, XConfigureEvent& event)
{
  if (event.override_redirect || !state.clients.contains(event.window))
    return;

#ifndef NDEBUG
  std::println(std::clog, "ConfigureNotify from {}", event.window);
#endif

  auto& client = state.clients.at(event.window);
  client.x = event.x;
  client.y = event.y;
  client.width = event.width;
  client.height = event.height;
  client.borderWidth = event.border_width;
}

static void
handleCreateNotify(State& state, XCreateWindowEvent& event)
{
  if (event.override_redirect)
    return;

  XCompositeRedirectWindow(
    state.dpy.x11, event.window, CompositeRedirectManual);
  XFlush(state.dpy.x11);

  state.clients.emplace(event.window,
                        Client{
                          .x = event.x,
                          .y = event.y,
                          .width = event.width,
                          .height = event.height,
                          .borderWidth = event.border_width,
                        });
}

static void
handleDestroyNotify(State& state, XDestroyWindowEvent& event)
{
  state.clients.erase(event.window);
}

static void
handleEnterNotify(State& state, XEnterWindowEvent& event)
{
}

static void
handleExpose(State& state, XExposeEvent& event)
{
}

static void
handleFocusIn(State& state, XFocusInEvent& event)
{
}

static void
handleKeyPress(State& state, XKeyPressedEvent& event)
{
  for (auto& keybind : state.keybinds) {
    if (keybind.key == event.keycode && keybind.mod == event.state) {
      keybind.action(state);
      break;
    }
  }
}

static void
handleKeyRelease(State& state, XKeyReleasedEvent& event)
{
}

static void
handleButtonPress(State& state, XButtonPressedEvent& event)
{
}

static void
handleButtonRelease(State& state, XButtonReleasedEvent& event)
{
}

static void
handleResizeRequest(State& state, XResizeRequestEvent& event)
{
}

static void
handleUnmapNotify(State& state, XUnmapEvent& event)
{
}

static void
handleClientMessage(State& state, XClientMessageEvent& event)
{ // TODO: have to unredirect full screen clients and redirect back when
  // not fullscreen
  // TODO: have to think about how apps do borderless fullscreen
#ifndef NDEBUG
  std::println(std::clog, "ClientMessage from {}", event.window);
#endif
}

static void
handlePropertyNotify(State& state, XPropertyEvent& event)
{
#ifndef NDEBUG
  std::println(std::clog, "PropertyNotify from {}", event.window);
#endif
}

static void
handleMotionNotify(State& state, XMotionEvent& event)
{
}

static void
handleMapRequest(State& state, XMapRequestEvent& event)
{
  if (!state.clients.contains(event.window))
    return;

#ifndef NDEBUG
  std::println("MapRequest from {}", event.window);
#endif

  XMapWindow(state.dpy.x11, event.window);
  XFlush(state.dpy.x11);
}

static void
handleMapNotify(State& state, XMapEvent& event)
{
  if (event.override_redirect || !state.clients.contains(event.window))
    return;

#ifndef NDEBUG
  std::println(std::clog, "MapNotify from {}", event.window);
#endif

  Client& client = state.clients.at(event.window);
  auto _ = client;
}

static void
handleMappingNotify(State& state, XMappingEvent& event)
{ // TODO: Update mappings here!
}

void
handleEvent(State& state, XEvent* event)
{
  switch (event->type) {
    case ClientMessage:
      handleClientMessage(state, *(XClientMessageEvent*)event);
      break;

    case MapRequest:
      handleMapRequest(state, *(XMapRequestEvent*)event);
      break;

    case MapNotify:
      handleMapNotify(state, *(XMapEvent*)event);
      break;

    case ConfigureRequest:
      handleConfigureRequest(state, *(XConfigureRequestEvent*)event);
      break;

    case ConfigureNotify:
      handleConfigureNotify(state, *(XConfigureEvent*)event);
      break;

    case MotionNotify:
      handleMotionNotify(state, *(XMotionEvent*)event);
      break;

    case CreateNotify:
      handleCreateNotify(state, *(XCreateWindowEvent*)event);
      break;

    case DestroyNotify:
      handleDestroyNotify(state, *(XDestroyWindowEvent*)event);
      break;

    case EnterNotify:
      handleEnterNotify(state, *(XEnterWindowEvent*)event);
      break;

    case Expose:
      handleExpose(state, *(XExposeEvent*)event);
      break;

    case FocusIn:
      handleFocusIn(state, *(XFocusInEvent*)event);
      break;

    case KeyPress:
      handleKeyPress(state, *(XKeyPressedEvent*)event);
      break;

    case KeyRelease:
      handleKeyRelease(state, *(XKeyReleasedEvent*)event);
      break;

    case ButtonPress:
      handleButtonPress(state, *(XButtonPressedEvent*)event);
      break;

    case ButtonRelease:
      handleButtonRelease(state, *(XButtonReleasedEvent*)event);
      break;

    case MappingNotify:
      handleMappingNotify(state, *(XMappingEvent*)event);
      break;

    case PropertyNotify:
      handlePropertyNotify(state, *(XPropertyEvent*)event);
      break;

    case ResizeRequest:
      handleResizeRequest(state, *(XResizeRequestEvent*)event);
      break;

    case UnmapNotify:
      handleUnmapNotify(state, *(XUnmapEvent*)event);
      break;

    case 0:
      std::println(std::clog, "Error!");
      break;

    default:
      std::println(std::clog, "unhandled event {}", event->type);
      break;
  }
}

}
