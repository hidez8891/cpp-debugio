#ifndef DEBUGIO_IMPL_H_
#define DEBUGIO_IMPL_H_

#include "shared_event.h"
#include "shared_memory.h"
#include <atomic>
#include <functional>
#include <windows.h>

namespace debugio {
    struct Buffer {
        int32_t processID;
        uint8_t data[4096 - sizeof(int32_t)];
    };

    class MonitorImpl {
    private:
        shared_event eventBufferReady;
        shared_event eventDataReady;
        shared_memory<Buffer> shmBuffer;

        HANDLE hMonitorThread;

        std::function<int(Buffer*)> callback;
        std::atomic<bool> wantThreadStop;

        static DWORD monitor_thread(LPVOID param);

    public:
        MonitorImpl();
        ~MonitorImpl();

    protected:
        int open();
        int close();
        int start(std::function<int(Buffer*)> callback);
        int stop();
    };
}

#endif