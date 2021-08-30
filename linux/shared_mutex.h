#ifndef SHARED_MUTEX_H_
#define SHARED_MUTEX_H_

#include "shared_memory.h"
#include <pthread.h>
#include <time.h>

class shared_mutex {
private:
    shared_memory<pthread_mutex_t> shm;

public:
    shared_mutex(const char* name);
    virtual ~shared_mutex();

    pthread_mutex_t* operator&();

    bool is_owner() const;
    bool is_opened() const;

    int open();
    int close(bool force_delete = false);

    int lock(int wait_ms = -1);
    int trylock();
    int unlock();
};

inline shared_mutex::shared_mutex(const char* name)
    : shm(name)
{
}
inline shared_mutex::~shared_mutex()
{
    errno = close();
}

inline pthread_mutex_t* shared_mutex::operator&()
{
    return &shm;
}

inline bool shared_mutex::is_owner() const
{
    return shm.is_owner();
}

inline bool shared_mutex::is_opened() const
{
    return shm.is_opened();
}

inline int shared_mutex::open()
{
    if (shm.is_opened()) {
        return 0;
    }

    int err = shm.open();
    if (err != 0) {
        return err;
    }
    if (!shm.is_owner()) {
        return 0;
    }

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0
        || pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0
        || pthread_mutex_init(&shm, &attr) != 0) {
        err = errno;
        pthread_mutexattr_destroy(&attr);
        shm.close();
        return err;
    }
    pthread_mutexattr_destroy(&attr);

    return 0;
}

inline int shared_mutex::close(bool force_delete)
{
    if (!shm.is_opened()) {
        return 0;
    }

    if (shm.is_owner() || force_delete) {
        int err = pthread_mutex_destroy(&shm);
        if (err != 0) {
            return err;
        }
    }
    return shm.close(force_delete);
}

inline int shared_mutex::lock(int wait_ms)
{
    if (!shm.is_opened()) {
        return -1;
    }

    if (wait_ms < 0) {
        return pthread_mutex_lock(&shm);
    }

    timespec timeout;
    timespec_get(&timeout, TIME_UTC);
    timeout.tv_sec += wait_ms / 1000;
    timeout.tv_nsec += wait_ms % 1000 * 1000;

    return pthread_mutex_timedlock(&shm, &timeout);
}

inline int shared_mutex::trylock()
{
    if (!shm.is_opened()) {
        return -1;
    }
    return pthread_mutex_trylock(&shm);
}

inline int shared_mutex::unlock()
{
    if (!shm.is_opened()) {
        return -1;
    }
    return pthread_mutex_unlock(&shm);
}

#endif