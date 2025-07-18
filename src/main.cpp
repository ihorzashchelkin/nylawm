#include <iostream>
#include <print>

#include "conf.hpp"
#include "wm_instance.hpp"

static wm::Instance* instance;

namespace wm {

void quit() { instance->quit(); }
void spawn(const char* const command[]) { instance->spawn(command); }

}

int main(int argc, char* argv[])
{
    if (argc == 2 && std::string_view { argv[1] } == "-v") {
        std::println(wm::conf::version);
        return EXIT_SUCCESS;
    }

    if (argc > 1) {
        std::println("usage: ", argv[0], " [-v]");
        return EXIT_SUCCESS;
    }

    instance = new wm::Instance();
    if (!instance->try_init()) {
        if (!wm::conf::display_fallback)
            return EXIT_FAILURE;

        std::cerr << "trying " << wm::conf::display_fallback << " fallback..."
                  << std::endl;

        auto display_env = std::string("DISPLAY=") + wm::conf::display_fallback;
        putenv(const_cast<char*>(display_env.c_str()));

        if (!instance->try_init())
            return EXIT_FAILURE;
    }

    instance->run();
}
