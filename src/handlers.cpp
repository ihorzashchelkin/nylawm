#include <X11/Xlib.h>
#include <cstdint>
#include <epoxy/glx_generated.h>
#include <iostream>
#include <print>
#include <xcb/composite.h>
#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xproto.h>

#include "src/nyla.hpp"

namespace nyla::handlers {

void
configureRequest(State& state, xcb_configure_request_event_t& event)
{
#ifndef NDEBUG
  std::println(std::clog, "ConfigureRequest from {}", event.window);
#endif

  uint32_t mask = 0;
  std::vector<uint32_t> values(7);

  auto appendIf = [&](uint32_t flag, uint32_t val) {
    if (event.value_mask & flag) {
      mask |= flag;
      values.push_back(val);
    }
  };

  appendIf(XCB_CONFIG_WINDOW_X, event.x);
  appendIf(XCB_CONFIG_WINDOW_Y, event.y);
  appendIf(XCB_CONFIG_WINDOW_WIDTH, event.width);
  appendIf(XCB_CONFIG_WINDOW_HEIGHT, event.height);
  appendIf(XCB_CONFIG_WINDOW_BORDER_WIDTH, event.border_width);
  appendIf(XCB_CONFIG_WINDOW_SIBLING, event.sibling);
  appendIf(XCB_CONFIG_WINDOW_STACK_MODE, event.stack_mode);

  xcb_configure_window(state.xcb.conn, state.xcb.window, mask, values.data());
  ++state.xcb.pendingRequests;
}

void
configureNotify(State& state, xcb_configure_notify_event_t& event)
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

void
createNotify(State& state, xcb_create_notify_event_t& event)
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

void
destroyNotify(State& state, xcb_destroy_notify_event_t& event)
{
  state.clients.erase(event.window);
}

void
enterNotify(State& state, xcb_enter_notify_event_t& event)
{
}

void
expose(State& state, xcb_expose_event_t& event)
{
}

void
focusIn(State& state, xcb_focus_in_event_t& event)
{
}

void
keyPress(State& state, xcb_key_press_event_t& event)
{
  for (auto& keybind : state.keybinds) {
    if (keybind.key == event.detail && keybind.mod == event.state) {
      keybind.action(state);
      break;
    }
  }
}

void
keyRelease(State& state, xcb_key_release_event_t& event)
{
}

void
buttonPress(State& state, xcb_button_press_event_t& event)
{
}

void
buttonRelease(State& state, xcb_button_release_event_t& event)
{
}

void
resizeRequest(State& state, xcb_resize_request_event_t& event)
{
}

void
unmapNotify(State& state, xcb_unmap_notify_event_t& event)
{
}

void
clientMessage(State& state, xcb_client_message_event_t& event)
{ // TODO: have to unredirect full screen clients and redirect back when
  // not fullscreen
  // TODO: have to think about how apps do borderless fullscreen
#ifndef NDEBUG
  std::println(std::clog, "ClientMessage from {}", event.window);
#endif
}

void
propertyNotify(State& state, xcb_property_notify_event_t& event)
{
#ifndef NDEBUG
  std::println(std::clog, "PropertyNotify from {}", event.window);
#endif
}

void
motionNotify(State& state, xcb_motion_notify_event_t& event)
{
}

void
mapRequest(State& state, xcb_map_request_event_t& event)
{
  if (!state.clients.contains(event.window))
    return;

#ifndef NDEBUG
  std::println("MapRequest from {}", event.window);
#endif

  xcb_map_window(state.xcb.conn, event.window);
  ++state.xcb.pendingRequests;
}

void
mapNotify(State& state, xcb_map_notify_event_t& event)
{
  if (event.override_redirect || !state.clients.contains(event.window))
    return;

#ifndef NDEBUG
  std::println(std::clog, "MapNotify from {}", event.window);
#endif

  auto& client = state.clients.at(event.window);
  auto _ = client;
  // TODO:
}

void
mappingNotify(State& state, xcb_mapping_notify_event_t& event)
{ // TODO: Update mappings here!
}

void
error(State& state, xcb_generic_error_t& aError)
{
  std::println(std::cerr, "Error!");
}

void
genericEvent(State& state, xcb_generic_event_t* event)
{
  uint8_t eventType = event->response_type & ~0x80;
  switch (eventType) {
    case XCB_CLIENT_MESSAGE:
      clientMessage(state, *(xcb_client_message_event_t*)event);
      break;

    case XCB_MAP_REQUEST:
      mapRequest(state, *(xcb_map_request_event_t*)event);
      break;

    case XCB_MAP_NOTIFY:
      mapNotify(state, *(xcb_map_notify_event_t*)event);
      break;

    case XCB_CONFIGURE_REQUEST:
      configureRequest(state, *(xcb_configure_request_event_t*)event);
      break;

    case XCB_CONFIGURE_NOTIFY:
      configureNotify(state, *(xcb_configure_notify_event_t*)event);
      break;

    case XCB_MOTION_NOTIFY:
      motionNotify(state, *(xcb_motion_notify_event_t*)event);
      break;

    case XCB_CREATE_NOTIFY:
      createNotify(state, *(xcb_create_notify_event_t*)event);
      break;

    case XCB_DESTROY_NOTIFY:
      destroyNotify(state, *(xcb_destroy_notify_event_t*)event);
      break;

    case XCB_ENTER_NOTIFY:
      enterNotify(state, *(xcb_enter_notify_event_t*)event);
      break;

    case XCB_EXPOSE:
      expose(state, *(xcb_expose_event_t*)event);
      break;

    case XCB_FOCUS_IN:
      focusIn(state, *(xcb_focus_in_event_t*)event);
      break;

    case XCB_KEY_PRESS:
      keyPress(state, *(xcb_key_press_event_t*)event);
      break;

    case XCB_KEY_RELEASE:
      keyRelease(state, *(xcb_key_release_event_t*)event);
      break;

    case XCB_BUTTON_PRESS:
      buttonPress(state, *(xcb_button_press_event_t*)event);
      break;

    case XCB_BUTTON_RELEASE:
      buttonRelease(state, *(xcb_button_release_event_t*)event);
      break;

    case XCB_MAPPING_NOTIFY:
      mappingNotify(state, *(xcb_mapping_notify_event_t*)event);
      break;

    case XCB_PROPERTY_NOTIFY:
      propertyNotify(state, *(xcb_property_notify_event_t*)event);
      break;

    case XCB_RESIZE_REQUEST:
      resizeRequest(state, *(xcb_resize_request_event_t*)event);
      break;

    case XCB_UNMAP_NOTIFY:
      unmapNotify(state, *(xcb_unmap_notify_event_t*)event);
      break;

    case 0:
      handlers::error(state, *(xcb_generic_error_t*)event);
      break;

    default:
      std::println(std::clog, "unhandled event {}", eventType);
      break;
  }
}

}
