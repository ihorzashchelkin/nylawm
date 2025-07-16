#include <iostream>
#include <ostream>
#include <string.h>
#include <string_view>

#include <cstdlib>

#include <sys/wait.h>

#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "config.hpp"
#include "wm.hpp"

static wm::Instance *instance_;
wm::Instance *wm::instance() { return instance_; }

int main(int argc, char *argv[]) {
  if (argc == 2 && std::string_view{argv[1]} == "-v") {
    std::cout << config::version << std::endl;
    return EXIT_SUCCESS;
  }

  if (argc > 1) {
    std::cout << "usage: " << argv[0] << " [-v]" << std::endl;
    return EXIT_SUCCESS;
  }

  instance_ = new wm::Instance();
  if (!instance_->try_init()) {
    if (!config::display_fallback)
      return EXIT_FAILURE;

    std::cerr << "trying " << config::display_fallback << " fallback..."
              << std::endl;

    auto display_env = std::string("DISPLAY=") + config::display_fallback;
    putenv(const_cast<char *>(display_env.c_str()));

    if (!instance_->try_init())
      return EXIT_FAILURE;
  }

  instance_->prepare_wm_process();
  instance_->run();
}
