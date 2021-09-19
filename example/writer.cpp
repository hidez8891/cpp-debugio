#pragma warning(disable : 4267)
#pragma warning(disable : 4996)

#include "../include/debugio.h"

#ifdef _MSC_VER
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <chrono>
#include <cstdio>
#include <thread>

int main()
{
    using std::this_thread::sleep_for;
    using namespace std::literals::chrono_literals;

#ifdef _MSC_VER
    fprintf(stderr, "writer pid = %d\n", GetCurrentProcessId());
#else
    fprintf(stderr, "writer pid = %d\n", getpid());
#endif

    for (int i = 0; i < 10; ++i) {
        std::string msg = "Hello! " + std::to_string(i);

        debugio::write_string(msg.c_str());
        fprintf(stderr, "[WRITE] %s\n", msg.c_str());

        sleep_for(100ms);
    }

    sleep_for(100ms);

    fprintf(stderr, "Writer process Finished\n");
    return 0;
}