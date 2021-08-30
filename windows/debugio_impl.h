#include <atomic>
#include <functional>
#include <windows.h>

namespace debugio {
    struct Buffer;

    class MonitorImpl {
    private:
        HANDLE hDBWinBufferReady;
        HANDLE hDBWinDataReady;
        HANDLE hDBWinBuffer;
        Buffer* pDBWinBuffer;

        HANDLE hMonitorThread;

        std::function<int(Buffer*)> _callback;
        std::atomic<bool> wantThreadStop;

        static DWORD monitor_thread(LPVOID param);

    public:
        MonitorImpl()
            : hDBWinBufferReady(nullptr)
            , hDBWinDataReady(nullptr)
            , hDBWinBuffer(nullptr)
            , pDBWinBuffer(nullptr)
            , hMonitorThread(nullptr)
            , wantThreadStop(false)
        {
        }

        virtual ~MonitorImpl()
        {
            stop();
            close();
        }

    protected:
        int open();
        int close();
        int start(std::function<int(Buffer*)> callback);
        int stop();
    };
}
