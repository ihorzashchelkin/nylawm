#include <experimental/scope>
#include <iostream>
#include <memory>
#include <sched.h>
#include <stdexcept>
#include <thread>
#include <unistd.h>

#include "../src/conf.hpp"
#include "../src/wm_instance.hpp"

class WMTest {
public:
    WMTest()
    {
        xephyr_pid = fork();
        if (xephyr_pid < 0) {
            throw new std::runtime_error("fork() failed");
        }
        if (xephyr_pid == 0) {
            execvp("/usr/bin/Xephyr", const_cast<char**>((const char*[]) { "-br", "-ac", "-noreset", "+extension", "RANDR", "+extension", "GLX", "-screen", "1280x720", ":1", nullptr }));
            std::cerr << "execvp() failed" << std::endl;
            std::exit(1);
        }
    }
    virtual ~WMTest()
    {
        std::cout << "killing..." << std::endl;
        kill(xephyr_pid, SIGKILL);
    }

    void run()
    {
        const auto conf = std::make_shared<MyConfiguration>();
        wm::WMInstance instance(conf);

        std::thread t { [&]() {
            instance.try_init();
            instance.run();
        } };

        test(&instance);

        instance.quit();
        std::cout << "exiting..." << std::endl;
        t.join();
        std::cout << "exited" << std::endl;
    }

    virtual void test(wm::WMInstance* instance) = 0;

private:
    pid_t xephyr_pid;
};

class BasicWMTest : public WMTest {
public:
    BasicWMTest()
        : WMTest()
    {
    }

    void test(wm::WMInstance* instance) override
    {
        sleep(2);
    }
};

int main()
{
    BasicWMTest().run();
}
