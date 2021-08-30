#ifndef SHARED_CONDITION_H_
#define SHARED_CONDITION_H_

#include "shared_memory.h"
#include "shared_mutex.h"
#include <pthread.h>
#include <string>
#include <time.h>

class shared_condition {
private:
    shared_memory<pthread_cond_t> shm;
    shared_mutex mutex;
    char name_buf[BUFSIZ] = { '\0' };

public:
    shared_condition(const char* name);
    virtual ~shared_condition();

    bool is_owner() const;
    bool is_opened() const;

    int open();
    int close(bool force_delete = false);

    int wait(int wait_ms = -1);
    int notify();
    int notify_all();
};

inline shared_condition::shared_condition(const char* name)
    : shm(name)
    , mutex(strcat(strcat(name_buf, name), "_mutex"))
{
}

inline shared_condition::~shared_condition()
{
    errno = close();
}

inline bool shared_condition::is_owner() const
{
    return shm.is_owner();
}

inline bool shared_condition::is_opened() const
{
    return shm.is_opened();
}

inline int shared_condition::open()
{
    if (shm.is_opened()) {
        return 0;
    }

    int err = shm.open();
    if (err != 0) {
        return err;
    }
    err = mutex.open();
    if (err != 0) {
        return err;
    }

    if (!shm.is_owner()) {
        return 0;
    }

    {
        pthread_condattr_t cond_attr;
        if (pthread_condattr_init(&cond_attr) != 0
            || pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED) != 0
            || pthread_cond_init(&shm, &cond_attr) != 0) {
            err = errno;
            pthread_condattr_destroy(&cond_attr);
            mutex.close();
            shm.close();
            return err;
        }
        pthread_condattr_destroy(&cond_attr);
    }

    return 0;
}

inline int shared_condition::close(bool force_delete)
{
    if (!shm.is_opened()) {
        return 0;
    }

    if (shm.is_owner() || force_delete) {
        int err = pthread_cond_destroy(&shm);
        if (err != 0) {
            return err;
        }
    }
    return mutex.close(force_delete) || shm.close(force_delete);
}

inline int shared_condition::wait(int wait_ms)
{
    if (!shm.is_opened()) {
        return -1;
    }

    int err;
    if ((err = mutex.lock(wait_ms)) != 0) {
        return err;
    }
    if (wait_ms < 0) {
        err = pthread_cond_wait(&shm, &mutex);
    } else {
        timespec timeout;
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += wait_ms / 1000;
        timeout.tv_nsec += wait_ms % 1000 * 1000;

        err = pthread_cond_timedwait(&shm, &mutex, &timeout);
    }
    if (err != 0) {
        mutex.unlock();
        return err;
    }
    if ((err = mutex.unlock()) != 0) {
        return err;
    }
    return 0;
}

inline int shared_condition::notify()
{
    if (!shm.is_opened()) {
        return -1;
    }

    int err;
    if ((err = mutex.lock()) != 0) {
        return err;
    }
    if ((err = pthread_cond_signal(&shm)) != 0) {
        mutex.unlock();
        return err;
    }
    if ((err = mutex.unlock()) != 0) {
        return err;
    }
    return 0;
}

inline int shared_condition::notify_all()
{
    if (!shm.is_opened()) {
        return -1;
    }

    int err;
    if ((err = mutex.lock()) != 0) {
        return err;
    }
    if ((err = pthread_cond_broadcast(&shm)) != 0) {
        mutex.unlock();
        return err;
    }
    if ((err = mutex.unlock()) != 0) {
        return err;
    }
    return 0;
}

#endif