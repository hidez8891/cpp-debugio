#include <pthread.h>

#include <atomic>
#include <functional>

namespace debugio
{
    struct Buffer;

    class MonitorImpl
    {
    private:
        int pipe_fd;

        pthread_t thread;

        std::function<int(Buffer*)> callback;
        std::atomic<bool> wantThreadStop;

        static void* monitor_thread(void*);

    public:
        MonitorImpl() :
            pipe_fd(0),
            thread(0),
            wantThreadStop(false)
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