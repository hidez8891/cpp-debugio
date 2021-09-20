#pragma warning(disable : 4267)
#pragma warning(disable : 4996)

#include "../include/debugio.h"

#include <condition_variable>
#include <csignal>
#include <mutex>

std::mutex mtx;
std::condition_variable do_shutdown;

void signal_handler(int signal)
{
    do_shutdown.notify_all();
}

int main()
{
    std::signal(SIGINT, signal_handler);

    debugio::Monitor monitor;
    monitor.start([](debugio::Buffer* buf) -> int {
        fprintf(stdout, "[pid:%d] %s\n", buf->processID, buf->data);
        return 0;
    });

    std::unique_lock<std::mutex> lk(mtx);
    do_shutdown.wait(lk); // wait for shutdown

    monitor.stop();
    return 0;
}
