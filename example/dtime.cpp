#pragma warning(disable : 4267)
#pragma warning(disable : 4996)

#include "../include/debugio.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

std::atomic<bool> do_shutdown;

void signal_handler(int signal)
{
    do_shutdown.store(true);
}

int main()
{
    using std::chrono::system_clock;
    using std::this_thread::sleep_for;
    using namespace std::literals::chrono_literals;

    std::signal(SIGINT, signal_handler);

    while (!do_shutdown.load()) {
        auto t = system_clock::to_time_t(system_clock::now());
        auto lt = std::localtime(&t);

        std::stringstream ss;
        ss << std::put_time(lt, "%c");

        debugio::write_string(ss.str().c_str());
        std::cout << ss.str() << std::endl;

        sleep_for(1s);
    }

    return 0;
}
