#include "../include/debugio.h"

#include <chrono>
#include <cstdio>
#include <thread>

int main()
{
    using std::this_thread::sleep_for;
    using namespace std::literals::chrono_literals;

    debugio::Monitor monitor;

    monitor.start([](debugio::Buffer* buf) -> int {
        fprintf(stderr, "[READ] pid=%d msg=%s\n", buf->processID, buf->data);
        return 0;
    });

    sleep_for(500ms);

    monitor.stop();
    return 0;
}
