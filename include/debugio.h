#ifndef DEBUGIO_H_
#define DEBUGIO_H_

#include <functional>
#include <thread>

#ifdef _MSC_VER
#include "ipc/win32/event.h"
#include "ipc/win32/shared_memory.h"
#define BUILD_WIN32_
#else
#include "ipc/posix/event.h"
#include "ipc/posix/shared_memory.h"
#define BUILD_POSIX_
#endif

namespace debugio {
    const char* EVENTS_BUFFER_READY_ID = "DBWIN_BUFFER_READY";
    const char* EVENTS_DATA_READY_ID = "DBWIN_DATA_READY";
    const char* DATA_BUFFER_ID = "DBWIN_BUFFER";

#ifdef BUILD_WIN32_
    typedef DWORD pid_t;
    using namespace ipc::win32;
#else
    typedef ::pid_t pid_t;
    using namespace ipc::posix;
#endif

    /***************************************************************************
     * DebugIO Buffer Monitor
    */

    struct Buffer {
        pid_t processID;
        char data[4096 - sizeof(pid_t)];
    };

    /***************************************************************************
     * DebugIO Base
    */
    class DebugIoBase {
    protected:
        shared_memory<Buffer> buffer;
        event buffer_ready;
        event data_ready;

        int open();
        int close();

    public:
        DebugIoBase();
        virtual ~DebugIoBase();
    };

    inline DebugIoBase::DebugIoBase()
        : buffer(DATA_BUFFER_ID)
        , buffer_ready(EVENTS_BUFFER_READY_ID)
        , data_ready(EVENTS_DATA_READY_ID)
    {
    }

    inline DebugIoBase::~DebugIoBase()
    {
        close();
    }

    inline int DebugIoBase::open()
    {
        int err;
        if ((err = buffer.open()) != 0) {
            close();
            return err;
        }
        if ((err = buffer_ready.open()) != 0) {
            close();
            return err;
        }
        if ((err = data_ready.open()) != 0) {
            close();
            return err;
        }
        return 0;
    }

    inline int DebugIoBase::close()
    {
        int err;
        if ((err = buffer.close()) != 0) {
            return err;
        }
        if ((err = buffer_ready.close()) != 0) {
            return err;
        }
        if ((err = data_ready.close()) != 0) {
            return err;
        }
        return 0;
    }

    /***************************************************************************
     * DebugIO Buffer Monitor
    */
    class Monitor : DebugIoBase {
    private:
        std::jthread monitor;

    public:
        Monitor();
        virtual ~Monitor();

        int start(std::function<int(Buffer*)> callback);
        int stop();
    };

    inline Monitor::Monitor()
        : monitor()
    {
    }

    inline Monitor::~Monitor()
    {
        stop();
    }

    inline int Monitor::start(std::function<int(Buffer*)> callback)
    {
        int ret = open();
        if (ret != 0) {
            return ret;
        }

        if (monitor.joinable()) {
            return -1;
        }

        monitor = std::jthread([this, callback](std::stop_token st) mutable {
            this->buffer_ready.notify();
            while (!st.stop_requested()) {
                if (this->data_ready.wait(100) == 0) {
                    callback(&buffer);
                    this->buffer_ready.notify();
                }
            }
        });
        return 0;
    }

    inline int Monitor::stop()
    {
        if (!monitor.joinable()) {
            return 0;
        }

        monitor.request_stop();
        monitor.join();

        return close();
    }

    /***************************************************************************
     * DebugIO Buffer Writer
    */
    class Writer : public DebugIoBase {
    public:
        Writer() = default;
        virtual ~Writer() = default;

        int write(const char* data, size_t size = 0);
    };

    int Writer::write(const char* data, size_t size)
    {
        int err;
        if ((err = open()) != 0) {
            return err;
        }

        Buffer buf;
        memset(&buf, 0, sizeof(Buffer));

#ifdef BUILD_WIN32_
        buf.processID = GetCurrentProcessId();
#else
        buf.processID = getpid();
#endif

        if (size == 0) {
            size = strlen(data);
        }
        memcpy(buf.data, data, size);

        if (buffer_ready.wait(1) == 0) {
            memset(&buffer, 0, sizeof(Buffer));
            memcpy(&buffer, &buf, sizeof(Buffer));
            data_ready.notify();
        }

        return 0;
    }

    /***************************************************************************
     * DebugIO Buffer Write Function
    */
    int write_string(const char* str)
    {
#ifdef BUILD_WIN32_
        OutputDebugStringA(str);
#else
        static Writer writer;
        writer.write(str);
#endif
        return strlen(str);
    }
}

#endif /* DEBUGIO_H_ */