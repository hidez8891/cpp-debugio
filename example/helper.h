#ifdef _MSC_VER
# include <Windows.h>
#else
# include <sys/types.h>
# include <unistd.h>
#endif

#include <chrono>
#include <thread>

namespace helper
{
    int getpid()
    {
#ifdef _MSC_VER
        return static_cast<int>(::GetCurrentProcessId());
#else
        return static_cast<int>(::getpid());
#endif
    }
    
    void sleep(unsigned int milli_seconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(milli_seconds));
    }
}