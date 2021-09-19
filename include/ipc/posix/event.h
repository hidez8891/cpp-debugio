#ifndef IPC_POSIX_EVENT_H_
#define IPC_POSIX_EVENT_H_

#include "semaphore.h"
#include "shared_memory.h"
#include <string.h>

namespace ipc::posix {

    class event {
    private:
        semaphore guard, sync;
        shared_memory<bool> signal;

        // noncopyable
        event(const event&) = delete;
        event& operator=(const event&) = delete;

    public:
        event();
        event(const char* name);
        ~event();

        event(event&&);
        event& operator=(event&&);

        bool is_opened() const;

        int open();
        int close();

        int wait(int wait_ms = -1);
        int notify();
    };

    inline event::event()
        : guard()
        , sync()
        , signal()
    {
    }

    inline event::event(const char* name)
        : guard((std::string(name) + "_guard").c_str())
        , sync((std::string(name) + "_sync").c_str())
        , signal(name)
    {
    }

    inline event::~event()
    {
        close();
    }

    inline event::event(event&& other)
        : event()
    {
        std::swap(guard, other.guard);
        std::swap(sync, other.sync);
        std::swap(signal, other.signal);
    }

    inline event& event::operator=(event&& other)
    {
        std::swap(guard, other.guard);
        std::swap(sync, other.sync);
        std::swap(signal, other.signal);
        return *this;
    }

    inline bool event::is_opened() const
    {
        return signal.is_opened();
    }

    inline int event::open()
    {
        if (is_opened()) {
            return 0;
        }

        int err;
        if ((err = guard.open(1)) != 0) {
            return err;
        }
        if ((err = sync.open(0)) != 0) {
            return err;
        }
        if ((err = signal.open()) != 0) {
            return err;
        }

        if (signal.is_created()) {
            if (guard.lock(100) == 0) {
                *signal = false;
                guard.unlock();
            }
        }

        return 0;
    }

    inline int event::close()
    {
        int err;
        if ((err = guard.close()) != 0) {
            return err;
        }
        if ((err = sync.close()) != 0) {
            return err;
        }
        if ((err = signal.close()) != 0) {
            return err;
        }
        return 0;
    }

    inline int event::wait(int wait_ms)
    {
        if (!is_opened()) {
            return -1;
        }

        guard.lock();
        while (*signal == false) {
            guard.unlock();

            int err = sync.lock(wait_ms);
            if (err != 0) {
                return err;
            }

            guard.lock();
        }
        *signal = false;
        guard.unlock();
        return 0;
    }

    inline int event::notify()
    {
        if (!is_opened()) {
            return -1;
        }

        guard.lock();
        *signal = true;
        guard.unlock();
        sync.unlock();

        return 0;
    }
}
#endif /* IPC_POSIX_EVENT_H_ */