#include <cstdint>
#include <functional>

#ifdef _MSC_VER
#include "windows/debugio_impl.h"
#else
#include "linux/debugio_impl.h"
#endif

namespace debugio {
    struct Buffer {
        int32_t processID;
        uint8_t data[4096 - sizeof(int32_t)];
    };

    class Monitor : public MonitorImpl {
    public:
        Monitor()
        {
        }

        ~Monitor()
        {
            MonitorImpl::stop();
            MonitorImpl::close();
        }

        int start(std::function<int(Buffer*)> callback)
        {
            int ret = MonitorImpl::open();
            if (ret != 0) {
                return ret;
            }
            return MonitorImpl::start(callback);
        }

        int stop()
        {
            int ret = MonitorImpl::stop();
            if (ret != 0) {
                return ret;
            }
            return MonitorImpl::close();
        }
    };

    int write(const char* data);
}
