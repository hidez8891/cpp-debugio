#include "debugio_impl.h"
#include <cstring>

#pragma comment(lib, "Advapi32.lib")

namespace debugio
{
    DWORD MonitorImpl::monitor_thread(LPVOID param)
    {
        auto *self = reinterpret_cast<class MonitorImpl *>(param);
        SetEvent(self->hDBWinBufferReady);

        while (!self->wantThreadStop.load()) {
            DWORD ret = ::WaitForSingleObject(self->hDBWinDataReady, 100);
            if (ret == WAIT_OBJECT_0) {
                self->_callback(self->pDBWinBuffer);
                SetEvent(self->hDBWinBufferReady);
            }
        }
        return 0;
    }

    int MonitorImpl::open()
    {
        // security
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = &sd;
        
        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
            fprintf(stderr, "fail initialize security descriptor: errno=%d\n", ::GetLastError());
            return 1;
        }
        if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE)) {
            fprintf(stderr, "fail set security descriptor: errno=%d\n", ::GetLastError());
            return 1;
        }
        
        // Event :: DBWIN_BUFFER_READY
        hDBWinBufferReady = ::OpenEventA(
            EVENT_ALL_ACCESS,
            FALSE,
            "DBWIN_BUFFER_READY"    
        );
        if (hDBWinBufferReady == nullptr) {
            hDBWinBufferReady = ::CreateEventA(
                &sa,
                FALSE,
                TRUE,
                "DBWIN_BUFFER_READY"    
            );
            if (hDBWinBufferReady == nullptr) {
                fprintf(stderr, "fail open/create DBWIN_BUFFER_READY: errno=%d\n", ::GetLastError());
                return 1;
            }
        }
        
        // Event :: DBWIN_DATA_READY
        hDBWinDataReady = ::OpenEventA(
            EVENT_ALL_ACCESS,
            FALSE,
            "DBWIN_DATA_READY"    
        );
        if (hDBWinDataReady == nullptr) {
            hDBWinDataReady = ::CreateEventA(
                &sa,
                FALSE,
                TRUE,
                "DBWIN_DATA_READY"    
            );
            if (hDBWinDataReady == nullptr) {
                fprintf(stderr, "fail open/create DBWIN_DATA_READY: errno=%d\n", ::GetLastError());
                return 1;
            }
        }
        
        // Shared Memory :: DBWIN_BUFFER
        hDBWinBuffer = ::OpenFileMappingA(
            FILE_MAP_READ,
            FALSE,
            "DBWIN_BUFFER"
        );
        if (hDBWinBuffer == nullptr) {
            hDBWinBuffer = ::CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                &sa,
                PAGE_READWRITE,
                0,
                4096, // sizeof(debugio::Buffer)
                "DBWIN_BUFFER"
            );
            if (hDBWinBuffer == nullptr) {
                fprintf(stderr, "fail open/create DBWIN_BUFFER: errno=%d\n", ::GetLastError());
                return 1;
            }
        }
        
        pDBWinBuffer = static_cast<Buffer*>(::MapViewOfFile(
            hDBWinBuffer,
            FILE_MAP_READ,
            0,
            0,
            512
        ));
        if (pDBWinBuffer == nullptr) {
            fprintf(stderr, "fail memory mapping DBWIN_BUFFER: errno=%d\n", ::GetLastError());
            return 1;
        }

        return 0;
    }

    int MonitorImpl::start(std::function<int(Buffer*)> callback)
    {
        stop();

        _callback = callback;

        hMonitorThread = ::CreateThread(
            NULL,
            0,
            MonitorImpl::monitor_thread,
            this,
            0,
            NULL
        );
        if (hMonitorThread == nullptr) {
            fprintf(stderr, "fail start monitor thread: errno=%d\n", ::GetLastError());
            return 1;
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

        if (hDBWinBuffer != nullptr) {
            CloseHandle(hDBWinBuffer);
            hDBWinBuffer = nullptr;
        }
        if (hDBWinDataReady != nullptr) {
            CloseHandle(hDBWinDataReady);
            hDBWinDataReady = nullptr;
        }
        if (hDBWinBufferReady != nullptr) {
            CloseHandle(hDBWinBufferReady);
            hDBWinBufferReady = nullptr;
        }

        return 0;
    }

    int write(const char *data)
    {
        OutputDebugStringA(data);
        return strlen(data);
    }
};
