#ifndef IPC_WIN32_EVENT_H_
#define IPC_WIN32_EVENT_H_

#pragma comment(lib, "Advapi32.lib")

#include <string.h>
#include <windows.h>

namespace ipc::win32 {

    class event {
    private:
        HANDLE hEvent;
        char* pName;

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
        int close(bool destroy = true);

        int wait(int wait_ms = -1);
        int notify();
    };

    inline event::event()
        : hEvent(nullptr)
        , pName(nullptr)
    {
    }

    inline event::event(const char* name)
        : event()
    {
        this->pName = strdup(name);
    }

    inline event::~event()
    {
        close();
        free(pName);
    }

    inline event::event(event&& other)
        : event()
    {
        std::swap(hEvent, other.hEvent);
        std::swap(pName, other.pName);
    }

    inline event& event::operator=(event&& other)
    {
        std::swap(hEvent, other.hEvent);
        std::swap(pName, other.pName);
        return *this;
    }

    inline bool event::is_opened() const
    {
        return hEvent != nullptr;
    }

    inline int event::open()
    {
        if (is_opened()) {
            return 0;
        }

        // security
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;

        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = &sd;

        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
            return ::GetLastError();
        }
        if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE)) {
            return ::GetLastError();
        }

        // Event
        hEvent = ::CreateEventA(&sa, FALSE, FALSE, pName);
        if (hEvent == nullptr) {
            return ::GetLastError();
        }

        return 0;
    }

    inline int event::close(bool destroy /* unused */)
    {
        if (hEvent != nullptr) {
            CloseHandle(hEvent);
            hEvent = nullptr;
        }
        return 0;
    }

    inline int event::wait(int wait_ms)
    {
        if (!is_opened()) {
            return -1;
        }

        DWORD ret = ::WaitForSingleObject(hEvent, wait_ms);
        if (ret == WAIT_OBJECT_0) {
            return 0;
        } else if (ret == WAIT_FAILED) {
            return ::GetLastError();
        } else {
            return ret;
        }
    }

    inline int event::notify()
    {
        if (!is_opened()) {
            return -1;
        }

        if (!SetEvent(hEvent)) {
            return ::GetLastError();
        }
        return 0;
    }
}
#endif /* IPC_WIN32_EVENT_H_ */