#ifndef IPC_POSIX_SHARED_MEMORY_H_
#define IPC_POSIX_SHARED_MEMORY_H_

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

namespace ipc::posix {

    template <typename T>
    class shared_memory {
    private:
        int fd;
        char* name;
        T* data;
        bool created;

        // noncopyable
        shared_memory(const shared_memory&) = delete;
        shared_memory& operator=(const shared_memory&) = delete;

    public:
        shared_memory();
        shared_memory(const char* name);
        ~shared_memory();

        shared_memory(shared_memory&&);
        shared_memory& operator=(shared_memory&&);

        T& operator*();
        T* operator->();
        T* operator&();

        bool is_opened() const;
        bool is_created() const;

        int open();
        int close();
    };

    template <typename T>
    shared_memory<T>::shared_memory()
        : fd(-1)
        , name(nullptr)
        , data(nullptr)
        , created(false)
    {
    }

    template <typename T>
    shared_memory<T>::shared_memory(const char* name)
        : shared_memory<T>()
    {
        this->name = strdup(("/" + std::string(name)).c_str());
    }

    template <typename T>
    shared_memory<T>::~shared_memory()
    {
        errno = close();
        free(name);
    }

    template <typename T>
    shared_memory<T>::shared_memory(shared_memory&& other)
        : shared_memory<T>()
    {
        std::swap(fd, other.fd);
        std::swap(name, other.name);
        std::swap(data, other.data);
        std::swap(created, other.created);
    }

    template <typename T>
    shared_memory<T>& shared_memory<T>::operator=(shared_memory&& other)
    {
        std::swap(fd, other.fd);
        std::swap(name, other.name);
        std::swap(data, other.data);
        return *this;
    }

    template <typename T>
    T& shared_memory<T>::operator*()
    {
        return *data;
    }

    template <typename T>
    T* shared_memory<T>::operator->()
    {
        return data;
    }

    template <typename T>
    T* shared_memory<T>::operator&()
    {
        return data;
    }

    template <typename T>
    bool shared_memory<T>::is_opened() const
    {
        return fd != -1;
    }

    template <typename T>
    bool shared_memory<T>::is_created() const
    {
        return created;
    }

    template <typename T>
    int shared_memory<T>::open()
    {
        if (is_opened()) {
            return 0;
        }

        created = true;
        fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0660);
        if (fd == -1) {
            created = false;
            fd = shm_open(name, O_RDWR | O_CREAT, 0660);
            if (fd == -1) {
                return errno;
            }
        }
        if (ftruncate(fd, sizeof(T)) != 0) {
            close();
            return errno;
        }

        void* p = mmap(
            NULL, sizeof(T),
            PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0);
        if (p == MAP_FAILED) {
            close();
            return errno;
        }
        data = reinterpret_cast<T*>(p);

        return 0;
    }

    template <typename T>
    int shared_memory<T>::close()
    {
        if (data != nullptr) {
            if (munmap(data, sizeof(T)) != 0) {
                return errno;
            }
            data = nullptr;
        }
        if (fd != -1) {
            if (::close(fd) != 0) {
                return errno;
            }
            fd = -1;
        }
        if (shm_unlink(name) != 0) {
            return errno;
        }

        return 0;
    }
}

#endif /* IPC_POSIX_SHARED_MEMORY_H_ */