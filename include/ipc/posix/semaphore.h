#ifndef IPC_POSIX_SEMAPHORE_H_
#define IPC_POSIX_SEMAPHORE_H_

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <utility>

namespace ipc::posix {

    class semaphore {
    private:
        sem_t* sem;
        char* name;

        // noncopyable
        semaphore(const semaphore&) = delete;
        semaphore& operator=(const semaphore&) = delete;

    public:
        semaphore();
        semaphore(const char* name);
        ~semaphore();

        semaphore(semaphore&&);
        semaphore& operator=(semaphore&&);

        bool is_opened() const;

        int open(int value = 1);
        int close();

        int lock(int wait_ms = -1);
        int trylock();
        int unlock();
    };

    inline semaphore::semaphore()
        : sem(SEM_FAILED)
        , name(nullptr)
    {
    }

    inline semaphore::semaphore(const char* name)
        : semaphore()
    {
        this->name = strdup(("/" + std::string(name)).c_str());
    }

    inline semaphore::~semaphore()
    {
        errno = close();
        free(name);
    }

    inline semaphore::semaphore(semaphore&& other)
        : semaphore()
    {
        std::swap(sem, other.sem);
        std::swap(name, other.name);
    }

    inline semaphore& semaphore::operator=(semaphore&& other)
    {
        std::swap(sem, other.sem);
        std::swap(name, other.name);
        return *this;
    }

    inline bool semaphore::is_opened() const
    {
        return sem != SEM_FAILED;
    }

    inline int semaphore::open(int value)
    {
        if (is_opened()) {
            return 0;
        }

        sem = sem_open(name, O_RDWR | O_CREAT, 0660, value);
        if (sem == SEM_FAILED) {
            return errno;
        }

        return 0;
    }

    inline int semaphore::close()
    {
        if (sem != SEM_FAILED) {
            if (sem_close(sem) != 0) {
                return errno;
            }
            sem = SEM_FAILED;
        }
        if (sem_unlink(name) != 0) {
            return errno;
        }
        return 0;
    }

    inline int semaphore::lock(int wait_ms)
    {
        if (!is_opened()) {
            return -1;
        }

        if (wait_ms < 0) {
            return sem_wait(sem);
        }

        timespec timeout;
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += wait_ms / 1000;
        timeout.tv_nsec += wait_ms % 1000 * 1000;

        return sem_timedwait(sem, &timeout);
    }

    inline int semaphore::trylock()
    {
        if (!is_opened()) {
            return -1;
        }
        return sem_trywait(sem);
    }

    inline int semaphore::unlock()
    {
        if (!is_opened()) {
            return -1;
        }
        return sem_post(sem);
    }
}

#endif /* IPC_POSIX_SEMAPHORE_H_ */