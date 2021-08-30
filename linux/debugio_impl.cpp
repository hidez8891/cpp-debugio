#include "debugio_impl.h"

const char* EVENTS_BUFFER_READY_ID = "/debugio_events_buffer_ready";
const char* EVENTS_DATA_READY_ID = "/debugio_events_data_ready";
const char* DATA_BUFFER_ID = "/debugio_data_buffer";

namespace debugio {
    struct Buffer {
        int32_t processID;
        uint8_t data[4096 - sizeof(int32_t)];
    };

    void* MonitorImpl::monitor_thread(void* param)
    {
        auto* self = reinterpret_cast<MonitorImpl*>(param);

        self->events_buffer_ready.notify_all();
        while (!self->wantThreadStop.load()) {
            int err = self->events_data_ready.wait(1000);
            if (err == 0) {
                self->callback(&(self->data_buffer));
            } else if (err != ETIME) {
                sleep(1);
            }
            self->events_buffer_ready.notify_all();
        }
        return nullptr;
    }

    MonitorImpl::MonitorImpl()
        : events_buffer_ready(EVENTS_BUFFER_READY_ID)
        , events_data_ready(EVENTS_DATA_READY_ID)
        , data_buffer(DATA_BUFFER_ID)
        , thread(0)
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
        if ((err = events_buffer_ready.open()) != 0) {
            close();
            return err;
        }
        if ((err = events_data_ready.open()) != 0) {
            close();
            return err;
        }
        if ((err = data_buffer.open()) != 0) {
            close();
            return err;
        }
        return 0;
    }

    int MonitorImpl::close()
    {
        stop();

        events_buffer_ready.close();
        events_data_ready.close();
        data_buffer.close();
        return 0;
    }

    int MonitorImpl::start(std::function<int(Buffer*)> callback)
    {
        stop();

        this->callback = callback;
        return pthread_create(&thread, NULL, MonitorImpl::monitor_thread, this);
    }

    int MonitorImpl::stop()
    {
        if (thread != 0) {
            wantThreadStop.store(true);
            pthread_join(thread, NULL);

            thread = 0;
            wantThreadStop.store(false);
        }
        return 0;
    }
}

namespace debugio {
    struct DebugWriter {
        shared_condition events_buffer_ready;
        shared_condition events_data_ready;
        shared_memory<Buffer> data_buffer;

        DebugWriter();
        ~DebugWriter();

        int open();
        int close();
        int write(void* data, int size);
    };

    DebugWriter::DebugWriter()
        : events_buffer_ready(EVENTS_BUFFER_READY_ID)
        , events_data_ready(EVENTS_DATA_READY_ID)
        , data_buffer(DATA_BUFFER_ID)
    {
    }

    DebugWriter::~DebugWriter()
    {
        close();
    }

    int DebugWriter::open()
    {
        int err;
        if ((err = events_buffer_ready.open()) != 0) {
            close();
            return err;
        }
        if ((err = events_data_ready.open()) != 0) {
            close();
            return err;
        }
        if ((err = data_buffer.open()) != 0) {
            close();
            return err;
        }
        return 0;
    }

    int DebugWriter::close()
    {
        events_buffer_ready.close();
        events_data_ready.close();
        data_buffer.close();
        return 0;
    }

    int DebugWriter::write(void* data, int size)
    {
        Buffer buf;
        memset(&buf, 0, sizeof(Buffer));
        buf.processID = getpid();
        memcpy(buf.data, data, size);

        if (events_buffer_ready.wait(100) != 0) {
            memcpy(&data_buffer, &buf, sizeof(Buffer));
            events_data_ready.notify_all();
        }

        return size;
    }

    static DebugWriter writer;

    int write(const char* data)
    {
        writer.open();

        int size = strlen(data) + 1;
        int n = writer.write((void*)data, size);
        if (n == -1) {
            return -errno;
        }
        if (n != size) {
            return 0;
        }
        return size - 1;
    }
}
