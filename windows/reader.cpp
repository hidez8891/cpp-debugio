#include <windows.h>

#include <cstdio>
#include <csignal>
#include <functional>
#include <atomic>

#pragma comment(lib, "Advapi32.lib")

struct DBWinBuffer
{
    DWORD processID;
    char  data[4096 - sizeof(DWORD)];
};

class DebugMonitor
{
private:
    HANDLE hDBWinMutex;
    HANDLE hDBWinBufferReady;
    HANDLE hDBWinDataReady;
    HANDLE hDBWinBuffer;
    struct DBWinBuffer *pDBWinBuffer;
    HANDLE hMonitorThread;

    std::function<void(struct DBWinBuffer*)> _callback;
    std::atomic<bool> wantThreadStop;

    static DWORD thread_body(LPVOID param)
    {
        auto *self = reinterpret_cast<class DebugMonitor *>(param);
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

public:
    DebugMonitor() :
        hDBWinMutex(nullptr),
        hDBWinBufferReady(nullptr),
        hDBWinDataReady(nullptr),
        hDBWinBuffer(nullptr),
        pDBWinBuffer(nullptr),
        hMonitorThread(nullptr),
        wantThreadStop(false)
    {
    }

    DWORD open()
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
        
        // Mutex :: DBWinMutex
//        hDBWinMutex = ::OpenMutexA(
//            MUTEX_ALL_ACCESS,
//            FALSE,
//            "DBWinMutex"
//        );
//        if (hDBWinMutex == nullptr) {
//            fprintf(stderr, "fail open DBWinMutex: errno=%d\n", ::GetLastError());
//            return 1;
//        }

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
                sizeof(struct DBWinBuffer),
                "DBWIN_BUFFER"
            );
            if (hDBWinBuffer == nullptr) {
                fprintf(stderr, "fail open/create DBWIN_BUFFER: errno=%d\n", ::GetLastError());
                return 1;
            }
        }
        
        pDBWinBuffer = static_cast<struct DBWinBuffer*>(::MapViewOfFile(
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

    DWORD start(std::function<void(struct DBWinBuffer*)> callback)
    {
        stop();

        _callback = callback;

        hMonitorThread = ::CreateThread(
            NULL,
            0,
            DebugMonitor::thread_body,
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
    
    void stop()
    {
        if (hMonitorThread != nullptr) {
            wantThreadStop.store(true);
            ::WaitForSingleObject(hMonitorThread, INFINITE);
            hMonitorThread = nullptr;
            wantThreadStop.store(false);
        }
    }

    void close()
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
        if (hDBWinMutex != nullptr) {
            CloseHandle(hDBWinMutex);
            hDBWinMutex = nullptr;
        }
    }
};

DebugMonitor debug_monitor{};

void signal_handler(int signum)
{
    debug_monitor.stop();
    debug_monitor.close();
}

int main()
{
    if (std::signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "fail regist signal handler\n");
        return 1;
    }

    if (debug_monitor.open() != 0) {
        debug_monitor.close();
        return 1;
    }

    debug_monitor.start([](DBWinBuffer *buf){
        fprintf(stderr, "[DEBUG] %d:%s\n", buf->processID, buf->data);
    });
    
    Sleep(10 * 1000);

    debug_monitor.stop();
    debug_monitor.close();

    return 0;
}