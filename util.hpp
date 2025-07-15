#pragma once

#include <iostream>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>

inline void log_xcb_error(xcb_connection_t *connection,
                          const xcb_generic_error_t *error) {
  static xcb_errors_context_t *error_context = nullptr;
  if (!error_context)
    xcb_errors_context_new(connection, &error_context);

  auto &errorcode = error->error_code;
  std::cerr << "error: " << int(errorcode) << " Bad"
            << xcb_errors_get_name_for_error(error_context, errorcode, nullptr)
            << std::endl;
}
