#ifndef SHARED_MEMORY_H_
#define SHARED_MEMORY_H_

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

template <typename T>
class shared_memory {
private:
    int fd;
    char* name;
    T* data;
    bool mem_owner;

public:
    shared_memory(const char* name);
    ~shared_memory();

    T& operator*();
    T* operator->();
    T* operator&();

    bool is_owner() const;
    bool is_opened() const;

    int open();
    int close(bool force_delete = false);
};

template <typename T>
shared_memory<T>::shared_memory(const char* name)
    : fd(-1)
    , name(nullptr)
    , data(nullptr)
    , mem_owner(false)
{
    this->name = strdup(name);
}

template <typename T>
shared_memory<T>::~shared_memory()
{
    errno = close();
    free(name);
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
bool shared_memory<T>::is_owner() const
{
    return mem_owner;
}

template <typename T>
bool shared_memory<T>::is_opened() const
{
    return fd != -1;
}

template <typename T>
int shared_memory<T>::open()
{
    if (is_opened()) {
        return 0;
    }

    mem_owner = true;
    fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0660);
    if (fd == -1) {
        mem_owner = false;
        fd = shm_open(name, O_RDWR, 0660);
        if (fd == -1) {
            return errno;
        }
    }
    if (ftruncate(fd, sizeof(pthread_mutex_t)) != 0) {
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
int shared_memory<T>::close(bool force_delete)
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
    if (mem_owner || force_delete) {
        if (shm_unlink(name) != 0) {
            return errno;
        }
        mem_owner = false;
    }

    return 0;
}

#endif