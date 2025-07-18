#include <iostream>

#include "conf.hpp"
#include "window_manager.hpp"

int main(int argc, char* argv[])
{
    if (argc == 2 && std::string_view { argv[1] } == "-v") {
        std::cout << "v0.0.1" << std::endl;
        return EXIT_SUCCESS;
    }

    if (argc > 1) {
        std::cout << "usage: " << argv[0] << " [-v]" << std::endl;
        return EXIT_SUCCESS;
    }

    const MyConfiguration conf;
    wm::WindowManager instance(conf);

    if (!instance.try_init()) {
        const auto& display_fallback = conf.display_fallback();
        if (!display_fallback)
            return EXIT_FAILURE;

        std::cerr << "trying " << display_fallback << " fallback..." << std::endl;
        auto display_env = std::string("DISPLAY=") + display_fallback;
        putenv(const_cast<char*>(display_env.c_str()));
        if (!instance.try_init())
            return EXIT_FAILURE;
    }

    instance.run();
}
