#include <cstdint>
#include <iostream>
#include <print>

#include <X11/Xlib.h>

#include <xcb/composite.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_errors.h>
#include <xcb/xproto.h>

#include "src/nyla.hpp"

namespace nyla {

static void
handleConfigureRequest(State& state, xcb_configure_request_event_t& event)
{
#ifndef NDEBUG
  std::println(std::clog, "ConfigureRequest from {}", event.window);
#endif

  uint32_t valueMask = 0;
  uint32_t values[2];
  int c = 0;

  // TODO:

  if (event.value_mask & XCB_CONFIG_WINDOW_WIDTH) {
    valueMask |= XCB_CONFIG_WINDOW_WIDTH;
    values[c++] = event.width;
  }

  if (event.value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
    valueMask |= XCB_CONFIG_WINDOW_HEIGHT;
    values[c++] = event.height;
  }

  xcb_configure_window(state.xcb.conn, event.window, valueMask, values);
  ++state.xcb.pendingRequests;
}

static void
handleConfigureNotify(State& state, xcb_configure_notify_event_t& event)
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
handleCreateNotify(State& state, xcb_create_notify_event_t& event)
{
  if (event.override_redirect)
    return;

  if (xcb_request_check(
        state.xcb.conn,
        xcb_composite_redirect_window_checked(
          state.xcb.conn, state.xcb.window, XCB_COMPOSITE_REDIRECT_MANUAL)))
    return;

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
handleDestroyNotify(State& state, xcb_destroy_notify_event_t& event)
{
  state.clients.erase(event.window);
}

static void
handleEnterNotify(State& state, xcb_enter_notify_event_t& event)
{
}

static void
handleExpose(State& state, xcb_expose_event_t& event)
{
}

static void
handleFocusIn(State& state, xcb_focus_in_event_t& event)
{
}

static void
handleKeyPress(State& state, xcb_key_press_event_t& event)
{
  for (auto& keybind : state.keybinds) {
    if (keybind.key == event.detail && keybind.mod == event.state) {
      keybind.action(state);
      break;
    }
  }
}

static void
handleKeyRelease(State& state, xcb_key_release_event_t& event)
{
}

static void
handleButtonPress(State& state, xcb_button_press_event_t& event)
{
}

static void
handleButtonRelease(State& state, xcb_button_release_event_t& event)
{
}

static void
handleResizeRequest(State& state, xcb_resize_request_event_t& event)
{
}

static void
handleUnmapNotify(State& state, xcb_unmap_notify_event_t& event)
{
}

static void
handleClientMessage(State& state, xcb_client_message_event_t& event)
{ // TODO: have to unredirect full screen clients and redirect back when
  // not fullscreen
  // TODO: have to think about how apps do borderless fullscreen
#ifndef NDEBUG
  std::println(std::clog, "ClientMessage from {}", event.window);
#endif
}

static void
handlePropertyNotify(State& state, xcb_property_notify_event_t& event)
{
#ifndef NDEBUG
  std::println(std::clog, "PropertyNotify from {}", event.window);
#endif
}

static void
handleMotionNotify(State& state, xcb_motion_notify_event_t& event)
{
}

static void
handleMapRequest(State& state, xcb_map_request_event_t& event)
{
  if (!state.clients.contains(event.window))
    return;

#ifndef NDEBUG
  std::println("MapRequest from {}", event.window);
#endif

  xcb_map_window(state.xcb.conn, event.window);
  ++state.xcb.pendingRequests;
}

static void
handleMapNotify(State& state, xcb_map_notify_event_t& event)
{
  if (event.override_redirect || !state.clients.contains(event.window))
    return;

#ifndef NDEBUG
  std::println(std::clog, "MapNotify from {}", event.window);
#endif

  Client& client = state.clients.at(event.window);
  auto _ = client;

  if (xcb_request_check(
        state.xcb.conn,
        xcb_composite_redirect_window_checked(
          state.xcb.conn, event.window, XCB_COMPOSITE_REDIRECT_MANUAL))) {
    std::println(std::clog, "xcb_composite_redirect_window_checked failed");
  }

  client.pixmap = xcb_generate_id(state.xcb.conn);

  if (xcb_request_check(state.xcb.conn,
                        xcb_composite_name_window_pixmap_checked(
                          state.xcb.conn, event.window, client.pixmap))) {
    std::println(std::clog, "xcb_composite_name_window_pixmap_checked failed");
  }

  xcb_aux_sync(state.xcb.conn);
}

static void
handleMappingNotify(State& state, xcb_mapping_notify_event_t& event)
{ // TODO: Update mappings here!
}

static void
handleXcbError(State& state, xcb_generic_error_t& error)
{
#ifdef NDEBUG
  std::println(std::cerr, "Error!");
#else
  std::println(std::cerr,
               "Bad{}",
               xcb_errors_get_name_for_error(
                 state.xcb.errorContext, error.error_code, nullptr));
#endif
}

void
handleEvent(State& state, xcb_generic_event_t* event)
{
  uint8_t eventType = event->response_type & ~0x80;
  switch (eventType) {
    case XCB_CLIENT_MESSAGE:
      handleClientMessage(state, *(xcb_client_message_event_t*)event);
      break;

    case XCB_MAP_REQUEST:
      handleMapRequest(state, *(xcb_map_request_event_t*)event);
      break;

    case XCB_MAP_NOTIFY:
      handleMapNotify(state, *(xcb_map_notify_event_t*)event);
      break;

    case XCB_CONFIGURE_REQUEST:
      handleConfigureRequest(state, *(xcb_configure_request_event_t*)event);
      break;

    case XCB_CONFIGURE_NOTIFY:
      handleConfigureNotify(state, *(xcb_configure_notify_event_t*)event);
      break;

    case XCB_MOTION_NOTIFY:
      handleMotionNotify(state, *(xcb_motion_notify_event_t*)event);
      break;

    case XCB_CREATE_NOTIFY:
      handleCreateNotify(state, *(xcb_create_notify_event_t*)event);
      break;

    case XCB_DESTROY_NOTIFY:
      handleDestroyNotify(state, *(xcb_destroy_notify_event_t*)event);
      break;

    case XCB_ENTER_NOTIFY:
      handleEnterNotify(state, *(xcb_enter_notify_event_t*)event);
      break;

    case XCB_EXPOSE:
      handleExpose(state, *(xcb_expose_event_t*)event);
      break;

    case XCB_FOCUS_IN:
      handleFocusIn(state, *(xcb_focus_in_event_t*)event);
      break;

    case XCB_KEY_PRESS:
      handleKeyPress(state, *(xcb_key_press_event_t*)event);
      break;

    case XCB_KEY_RELEASE:
      handleKeyRelease(state, *(xcb_key_release_event_t*)event);
      break;

    case XCB_BUTTON_PRESS:
      handleButtonPress(state, *(xcb_button_press_event_t*)event);
      break;

    case XCB_BUTTON_RELEASE:
      handleButtonRelease(state, *(xcb_button_release_event_t*)event);
      break;

    case XCB_MAPPING_NOTIFY:
      handleMappingNotify(state, *(xcb_mapping_notify_event_t*)event);
      break;

    case XCB_PROPERTY_NOTIFY:
      handlePropertyNotify(state, *(xcb_property_notify_event_t*)event);
      break;

    case XCB_RESIZE_REQUEST:
      handleResizeRequest(state, *(xcb_resize_request_event_t*)event);
      break;

    case XCB_UNMAP_NOTIFY:
      handleUnmapNotify(state, *(xcb_unmap_notify_event_t*)event);
      break;

    case 0:
      handleXcbError(state, *(xcb_generic_error_t*)event);
      break;

    default:
      std::println(std::clog, "unhandled event {}", eventType);
      break;
  }
}

}
