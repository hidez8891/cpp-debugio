
#pragma warning(disable : 4267)
#pragma warning(disable : 4996)

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

#ifdef _MSC_VER
    SetPriorityClass(::GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(monitor.native_handle(), THREAD_PRIORITY_TIME_CRITICAL);
#else
    struct sched_param param = {
        .sched_priority = 99,
    };
    pthread_setschedparam(monitor.native_handle(), SCHED_FIFO, &param);
#endif

    sleep_for(2000ms);
    monitor.stop();

    fprintf(stderr, "Reader process Finished\n");
    return 0;
}
