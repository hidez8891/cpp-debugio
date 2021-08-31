#ifndef SHARED_EVENT_H_
#define SHARED_EVENT_H_

#pragma comment(lib, "Advapi32.lib")

#include <string.h>
#include <windows.h>

class shared_event {
private:
    HANDLE hEvent;
    char* pName;
    bool bMemOwner;

public:
    shared_event(const char* name);
    ~shared_event();

    bool is_owner() const;
    bool is_opened() const;

    int open();
    int close();
    int wait(int wait_ms = -1);
    int notify_all();
};

inline shared_event::shared_event(const char* name)
    : hEvent(nullptr)
    , bMemOwner(false)
{
    this->pName = strdup(name);
}

inline shared_event::~shared_event()
{
    close();
    free(pName);
}

inline bool shared_event::is_owner() const
{
    return bMemOwner;
}

inline bool shared_event::is_opened() const
{
    return hEvent != nullptr;
}

inline int shared_event::open()
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
    hEvent = ::OpenEventA(EVENT_ALL_ACCESS, FALSE, pName);
    if (hEvent == nullptr) {
        hEvent = ::CreateEventA(&sa, FALSE, TRUE, pName);
        if (hEvent == nullptr) {
            return ::GetLastError();
        }
    }

    return 0;
}

inline int shared_event::close()
{
    if (hEvent != nullptr) {
        CloseHandle(hEvent);
        hEvent = nullptr;
    }
    return 0;
}

inline int shared_event::wait(int wait_ms)
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

inline int shared_event::notify_all()
{
    if (!is_opened()) {
        return -1;
    }

    if (SetEvent(hEvent)) {
        return 0;
    }
    return ::GetLastError();
}

#endif