#include "nyla.hpp"

namespace nyla {

static void
grabKeys(State& state)
{
  for (auto& keybind : state.keybinds) {
    xcb_grab_key(state.xcb.conn,
                 0,
                 state.xcb.screen->root,
                 keybind.mod,
                 keybind.key,
                 XCB_GRAB_MODE_ASYNC,
                 XCB_GRAB_MODE_ASYNC);
  }
  xcb_flush(state.xcb.conn);
}

const char*
initXcb(State& state, std::span<const Keybind> keybinds)
{
  int iscreen;
  state.xcb.conn = xcb_connect(nullptr, &iscreen);
  if (xcb_connection_has_error(state.xcb.conn))
    return "could not open display";

  state.xcb.screen = xcb_aux_get_screen(state.xcb.conn, iscreen);
  if (!state.xcb.screen)
    return "could not get screen";

  if (xcb_errors_context_new(state.xcb.conn, &state.xcb.errorContext))
    return "could not create xcb error context";

  state.xcb.window = xcb_generate_id(state.xcb.conn);
  if (xcb_request_check(
        state.xcb.conn,
        xcb_create_window_checked(state.xcb.conn,
                                  XCB_COPY_FROM_PARENT,
                                  state.xcb.window,
                                  state.xcb.screen->root,
                                  0,
                                  0,
                                  state.xcb.screen->width_in_pixels,
                                  state.xcb.screen->height_in_pixels,
                                  0,
                                  XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                  XCB_COPY_FROM_PARENT,
                                  0,
                                  nullptr))) {
    return "could not create window";
  }

#if 0
  uint32_t rootEventMask =
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
    XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_EXPOSURE |
    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
    XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_KEY_PRESS |
    XCB_EVENT_MASK_KEY_RELEASE;
  if (xcb_request_check(
        state.xcb.conn,
        xcb_change_window_attributes_checked(state.xcb.conn,
                                             state.xcb.screen->root,
                                             XCB_CW_EVENT_MASK,
                                             &rootEventMask))) {
    return "could not change root window attributes. is another wm already "
           "running?";
  }
#endif

  if (xcb_request_check(
        state.xcb.conn,
        xcb_map_window_checked(state.xcb.conn, state.xcb.window))) {
    return "could not map window";
  }

  xcb_key_symbols_t* syms = xcb_key_symbols_alloc(state.xcb.conn);
  for (auto& keybind : keybinds) {
    xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(syms, keybind.key);
    if (!keycodes)
      return "could not get keycodes for a keysym";

    for (xcb_keycode_t* p = keycodes; *p != XCB_NO_SYMBOL; ++p) {
      state.keybinds.emplace_back(
        Keybind{ .mod = keybind.mod, .key = *p, .action = keybind.action });
    }
    free(keycodes);
  }
  free(syms);

  grabKeys(state);

  return nullptr;
}

}
