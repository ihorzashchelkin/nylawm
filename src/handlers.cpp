#include <iostream>
#include <print>
#include <xcb/composite.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "internal.hpp"

namespace cirnowm {

void
WindowManager::HandleConfigureRequest(
  const xcb_configure_request_event_t* aEvent)
{
  if (IsDebug())
    std::println(std::clog, "ConfigureRequest from {}", aEvent->window);

  uint32_t mask = 0;
  std::vector<uint32_t> values;
  values.reserve(7);

  auto appendIf = [&](uint32_t flag, uint32_t val) {
    if (aEvent->value_mask & flag) {
      mask |= flag;
      values.push_back(val);
    }
  };

  appendIf(XCB_CONFIG_WINDOW_X, aEvent->x);
  appendIf(XCB_CONFIG_WINDOW_Y, aEvent->y);
  appendIf(XCB_CONFIG_WINDOW_WIDTH, aEvent->width);
  appendIf(XCB_CONFIG_WINDOW_HEIGHT, aEvent->height);
  appendIf(XCB_CONFIG_WINDOW_BORDER_WIDTH, aEvent->border_width);
  appendIf(XCB_CONFIG_WINDOW_SIBLING, aEvent->sibling);
  appendIf(XCB_CONFIG_WINDOW_STACK_MODE, aEvent->stack_mode);

  xcb_configure_window(mEwmh.connection, aEvent->window, mask, values.data());
  xcb_flush(mEwmh.connection);
}

void
WindowManager::HandleConfigureNotify(const xcb_configure_notify_event_t* aEvent)
{
}

void
WindowManager::HandleCreateNotify(const xcb_create_notify_event_t* aEvent)
{
}

void
WindowManager::HandleDestroyNotify(const xcb_destroy_notify_event_t* aEvent)
{
}

void
WindowManager::HandleEnterNotify(const xcb_enter_notify_event_t* aEvent)
{
}

void
WindowManager::HandleExpose(const xcb_expose_event_t* aEvent)
{
}

void
WindowManager::HandleFocusIn(const xcb_focus_in_event_t* aEvent)
{
}

void
WindowManager::HandleKeyPress(const xcb_key_press_event_t* aEvent)
{
  for (auto& keybind : mKeybinds) {
    if (keybind.mKeycode == aEvent->detail && keybind.mMods == aEvent->state) {
      keybind.mAction(this);
      break;
    }
  }
}

void
WindowManager::HandleKeyRelease(const xcb_key_release_event_t* aEvent)
{
}

void
WindowManager::HandleButtonPress(const xcb_button_press_event_t* aEvent)
{
}

void
WindowManager::HandleButtonRelease(const xcb_button_release_event_t* aEvent)
{
}

void
WindowManager::HandleResizeRequest(const xcb_resize_request_event_t* Event)
{
}

void
WindowManager::HandleUnmapNotify(const xcb_unmap_notify_event_t* Event)
{
}

void
WindowManager::HandleClientMessage(const xcb_client_message_event_t* aEvent)
{
  if (IsDebug())
    std::println(std::clog, "ClientMessage from {}", aEvent->window);

  if (aEvent->type == mEwmh._NET_WM_STATE) {
    // TODO:
  }
}

void
WindowManager::HandlePropertyNotify(const xcb_property_notify_event_t* aEvent)
{
  if (IsDebug())
    std::println(std::clog, "PropertyNotify from {}", aEvent->window);
}

void
WindowManager::HandleMotionNotify(const xcb_motion_notify_event_t* aEvent)
{
}

void
WindowManager::HandleMapRequest(const xcb_map_request_event_t* aEvent)
{
  xcb_map_window(mEwmh.connection, aEvent->window);
  xcb_flush(mEwmh.connection);
}

void
WindowManager::HandleMapNotify(const xcb_map_notify_event_t* aEvent)
{
  if (IsDebug())
    std::println(std::clog, "MapNotify from {}", aEvent->window);

  if (!mClients.contains(aEvent->window)) {
    xcb_get_window_attributes_reply_t* reply = xcb_get_window_attributes_reply(
      mEwmh.connection,
      xcb_get_window_attributes(mEwmh.connection, aEvent->window),
      nullptr);

    if (!reply || reply->override_redirect)
      return;

    if (xcb_request_check(
          mEwmh.connection,
          xcb_composite_redirect_window_checked(
            mEwmh.connection, aEvent->window, XCB_COMPOSITE_REDIRECT_MANUAL)))
      return;

    mClients.emplace(aEvent->window, Client{});

    xcb_pixmap_t pixmap = xcb_generate_id(mEwmh.connection);
    xcb_composite_name_window_pixmap(mEwmh.connection, aEvent->window, pixmap);
  }
}

void
WindowManager::HandleMappingNotify(const xcb_mapping_notify_event_t* aEvent)
{ // TODO: Update mappings here!
}

void
WindowManager::HandleError(const xcb_generic_error_t* aError)
{
  std::println(std::cerr, "Error!");
}

void
WindowManager::HandleEvent(const xcb_generic_event_t* aEvent)
{
  uint8_t eventType = aEvent->response_type & ~0x80;
  switch (eventType) {
    case XCB_CLIENT_MESSAGE:
      HandleClientMessage(
        reinterpret_cast<const xcb_client_message_event_t*>(aEvent));
      break;

    case XCB_MAP_REQUEST:
      HandleMapRequest(
        reinterpret_cast<const xcb_map_request_event_t*>(aEvent));
      break;

    case XCB_MAP_NOTIFY:
      HandleMapNotify(reinterpret_cast<const xcb_map_notify_event_t*>(aEvent));
      break;

    case XCB_CONFIGURE_REQUEST:
      HandleConfigureRequest(
        reinterpret_cast<const xcb_configure_request_event_t*>(aEvent));
      break;

    case XCB_CONFIGURE_NOTIFY:
      HandleConfigureNotify(
        reinterpret_cast<const xcb_configure_notify_event_t*>(aEvent));
      break;

    case XCB_MOTION_NOTIFY:
      HandleMotionNotify(
        reinterpret_cast<const xcb_motion_notify_event_t*>(aEvent));
      break;

    case XCB_CREATE_NOTIFY:
      HandleCreateNotify(
        reinterpret_cast<const xcb_create_notify_event_t*>(aEvent));
      break;

    case XCB_DESTROY_NOTIFY:
      HandleDestroyNotify(
        reinterpret_cast<const xcb_destroy_notify_event_t*>(aEvent));
      break;

    case XCB_ENTER_NOTIFY:
      HandleEnterNotify(
        reinterpret_cast<const xcb_enter_notify_event_t*>(aEvent));
      break;

    case XCB_EXPOSE:
      HandleExpose(reinterpret_cast<const xcb_expose_event_t*>(aEvent));
      break;

    case XCB_FOCUS_IN:
      HandleFocusIn(reinterpret_cast<const xcb_focus_in_event_t*>(aEvent));
      break;

    case XCB_KEY_PRESS:
      HandleKeyPress(reinterpret_cast<const xcb_key_press_event_t*>(aEvent));
      break;

    case XCB_KEY_RELEASE:
      HandleKeyRelease(
        reinterpret_cast<const xcb_key_release_event_t*>(aEvent));
      break;

    case XCB_BUTTON_PRESS:
      HandleButtonPress(
        reinterpret_cast<const xcb_button_press_event_t*>(aEvent));
      break;

    case XCB_BUTTON_RELEASE:
      HandleButtonRelease(
        reinterpret_cast<const xcb_button_release_event_t*>(aEvent));
      break;

    case XCB_MAPPING_NOTIFY:
      HandleMappingNotify(
        reinterpret_cast<const xcb_mapping_notify_event_t*>(aEvent));
      break;

    case XCB_PROPERTY_NOTIFY:
      HandlePropertyNotify(
        reinterpret_cast<const xcb_property_notify_event_t*>(aEvent));
      break;

    case XCB_RESIZE_REQUEST:
      HandleResizeRequest(
        reinterpret_cast<const xcb_resize_request_event_t*>(aEvent));
      break;

    case XCB_UNMAP_NOTIFY:
      HandleUnmapNotify(
        reinterpret_cast<const xcb_unmap_notify_event_t*>(aEvent));
      break;

    case 0:
      HandleError(reinterpret_cast<const xcb_generic_error_t*>(aEvent));
      break;

    default:
      std::println(std::clog, "unhandled event {}", eventType);
      break;
  }
}

}
