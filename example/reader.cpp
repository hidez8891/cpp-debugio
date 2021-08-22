#include "../debugio.h"
#include "helper.h"

#include <cstdio>

int main()
{
    debugio::Monitor monitor;

    int ret = monitor.start([](debugio::Buffer *buf) -> int {
        fprintf(stderr, "[READ] pid=%d msg=%s\n", buf->processID, buf->data);
        return 0;
    });
    if (ret != 0) {
        fprintf(stderr, "fail start debugio::Monitor\n");
        return 1;
    }

    helper::sleep(10 * 1000);

    monitor.stop();

    return 0;
}
