#include <windows.h>

#include <cstdio>
#include <string>

int main()
{
    fprintf(stderr, "pid = %d\n", GetCurrentProcessId());
    for (int i=0; i<10; ++i) {
        std::string msg = "Hello! " + std::to_string(i);
        OutputDebugStringA(msg.c_str());
        fprintf(stderr, "[LOG] %s\n", msg.c_str());
        Sleep(500);
    }
    return 0; 
}
