#include "shared_condition.h"
#include "shared_memory.h"
#include <atomic>
#include <functional>
#include <pthread.h>

namespace debugio {
    struct Buffer;

    class MonitorImpl {
    private:
        shared_condition events_buffer_ready;
        shared_condition events_data_ready;
        shared_memory<Buffer> data_buffer;

        pthread_t thread;

        std::function<int(Buffer*)> callback;
        std::atomic<bool> wantThreadStop;

        static void* monitor_thread(void*);

    public:
        MonitorImpl();
        virtual ~MonitorImpl();

    protected:
        int open();
        int close();
        int start(std::function<int(Buffer*)> callback);
        int stop();
    };

    int write(const char* data);
}