#include "../debugio.h"
#include "helper.h"

#include <cstdio>

int main()
{
    fprintf(stderr, "pid = %d\n", helper::getpid());

    for (int i = 0; i < 10; ++i) {
        std::string msg = "Hello! " + std::to_string(i);

        debugio::write(msg.c_str());
        fprintf(stderr, "[WRITE] %s\n", msg.c_str());

        helper::sleep(500);
    }
}