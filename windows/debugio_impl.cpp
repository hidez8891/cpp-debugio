#include "debugio_impl.h"
#include <cstring>

const char* EVENTS_BUFFER_READY_ID = "DBWIN_BUFFER_READY";
const char* EVENTS_DATA_READY_ID = "DBWIN_DATA_READY";
const char* DATA_BUFFER_ID = "DBWIN_BUFFER";

namespace debugio {
    DWORD MonitorImpl::monitor_thread(LPVOID param)
    {
        auto* self = reinterpret_cast<class MonitorImpl*>(param);
        self->eventBufferReady.notify_all();

        while (!self->wantThreadStop.load()) {
            if (self->eventDataReady.wait(100) == 0) {
                self->callback(&(self->shmBuffer));
                self->eventBufferReady.notify_all();
            }
        }
        return 0;
    }

    MonitorImpl::MonitorImpl()
        : eventBufferReady(EVENTS_BUFFER_READY_ID)
        , eventDataReady(EVENTS_DATA_READY_ID)
        , shmBuffer(DATA_BUFFER_ID)
        , hMonitorThread(nullptr)
        , callback(nullptr)
        , wantThreadStop(false)
    {
    }

    MonitorImpl::~MonitorImpl()
    {
        close();
    }

    int MonitorImpl::open()
    {
        int err;
        if ((err = eventBufferReady.open()) != 0) {
            close();
            return err;
        }
        if ((err = eventDataReady.open()) != 0) {
            close();
            return err;
        }
        if ((err = shmBuffer.open()) != 0) {
            close();
            return err;
        }
        return 0;
    }

    int MonitorImpl::start(std::function<int(Buffer*)> callback)
    {
        stop();

        this->callback = callback;
        hMonitorThread = ::CreateThread(
            NULL,
            0,
            MonitorImpl::monitor_thread,
            this,
            0,
            NULL);
        if (hMonitorThread == nullptr) {
            return ::GetLastError();
        }

        // monitor priority highest
        ::SetPriorityClass(::GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
        ::SetThreadPriority(hMonitorThread, THREAD_PRIORITY_TIME_CRITICAL);
        return 0;
    }

    int MonitorImpl::stop()
    {
        if (hMonitorThread != nullptr) {
            wantThreadStop.store(true);
            ::WaitForSingleObject(hMonitorThread, INFINITE);

            hMonitorThread = nullptr;
            wantThreadStop.store(false);
        }

        return 0;
    }

    int MonitorImpl::close()
    {
        stop();

        eventBufferReady.close();
        eventDataReady.close();
        shmBuffer.close();
        return 0;
    }

    int write(const char* data)
    {
        OutputDebugStringA(data);
        return strlen(data);
    }
};