#include <print>

#include "internal.hpp"

namespace cirnowm {

void
WindowManager::HandlePropertyNotify(const xcb_property_notify_event_t* Event)
{
  if (Config.DebugLog)
    std::println("PropertyNotify from {}", Event->window);
}

void
WindowManager::HandleMotionNotify(const xcb_motion_notify_event_t* Event)
{
}

void
WindowManager::HandleMapRequest(const xcb_map_request_event_t* Event)
{
  xcb_map_window(mConn, Event->window);
  xcb_flush(mConn);
}

void
WindowManager::HandleMapNotify(const xcb_map_notify_event_t* Event)
{
}

void
WindowManager::HandleMappingNotify(const xcb_mapping_notify_event_t* Event)
{ // TODO: Update mappings here!
}

void
WindowManager::HandleEvent(const xcb_generic_event_t* aEvent)
{
  uint8_t eventType = aEvent->response_type & ~0x80;
  switch (eventType) {
    case XCB_BUTTON_PRESS:
      HandleButtonPress(
        reinterpret_cast<const xcb_button_press_event_t*>(aEvent));
      break;

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
      std::println(std::cerr, "unhandled event {}", eventType);
      break;
  }
}

}
