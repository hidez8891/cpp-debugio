#include "debugio_impl.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char* FIFO_PATH = "/tmp/debugio_pipe";

namespace debugio
{
    struct Buffer
    {
        int32_t processID;
        uint8_t data[4096 - sizeof(int32_t)];
    };

    void* MonitorImpl::monitor_thread(void* param)
    {
        auto *self = reinterpret_cast<MonitorImpl*>(param);
        
        struct pollfd fds;
        memset(&fds, 0, sizeof(fds));
        fds.fd = self->pipe_fd;
        fds.events = POLLHUP | POLLERR | POLLIN;
        
        while (!self->wantThreadStop.load()) {
            int err = poll(&fds, 1, 100);
            if (err == -1)
                break; // error caused
            if (err == 0)
                continue; // timeout

            if (fds.revents & POLLHUP || fds.revents & POLLERR)
                break; // error caused

            Buffer buf;
            int n = ::read(self->pipe_fd, &buf, sizeof(Buffer));
            if (n != sizeof(Buffer))
                continue; // error caused

            self->callback(&buf);
        }

        return nullptr;
    }

    int MonitorImpl::open()
    {
        if (pipe_fd == 0) {
            ::mkfifo(FIFO_PATH, 0666);
            
            int fd = ::open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
            if (fd == -1)
                return -errno;
            
            pipe_fd = fd;
        }
        return 0;
    }

    int MonitorImpl::close()
    {
        stop();

        if (pipe_fd != 0) {
            ::close(pipe_fd);
            pipe_fd = 0;
        }
        return 0;
    }

    int MonitorImpl::start(std::function<int(Buffer*)> callback)
    {
        stop();

        this->callback = callback;

        int err = pthread_create(&thread, NULL, MonitorImpl::monitor_thread, this);
        if (err != 0)
            return err;

        return 0;
    }

    int MonitorImpl::stop()
    {
        if (thread != 0) {
            wantThreadStop.store(true);
            pthread_join(thread, NULL);

            thread = 0;
            wantThreadStop.store(false);
        }
        return 0;
    }
}

namespace debugio
{
    struct DebugWriter
    {
        int pipe_fd;

        DebugWriter() :
            pipe_fd(0)
        {
        }
        
        ~DebugWriter()
        {
            close();
        }
        
        int open();
        int close();
        int write(void *data, int size);
    };
    
    int DebugWriter::open()
    {
        if (pipe_fd == 0) {
            int fd = ::open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
            if (fd == -1)
                return -errno;
            pipe_fd = fd;
        }
        return 0;
    }

    int DebugWriter::close()
    {
        if (pipe_fd != 0) {
            ::close(pipe_fd);
            pipe_fd = 0;
        }
        return 0;
    }

    int DebugWriter::write(void *data, int size)
    {
        int err = open();
        if (err != 0)
            return -err;
        
        struct pollfd fds;
        memset(&fds, 0, sizeof(fds));
        fds.fd = pipe_fd;
        fds.events = POLLHUP | POLLERR | POLLOUT;

        err = poll(&fds, 1, 100);
        if (err == -1)
            return -errno;
        if (err == 0)
            return -ETIME;

        if (fds.revents & POLLHUP || fds.revents & POLLERR) {
            close();
            return -EBUSY;
        }

        Buffer buf;
        memset(&buf, 0, sizeof(Buffer));
        buf.processID = getpid();
        memcpy(buf.data, data, size);
        return ::write(pipe_fd, &buf, sizeof(Buffer));
    }

    static DebugWriter writer;

    int write(const char *data)
    {
        int size = strlen(data) + 1;
        int n = writer.write((void*)data, size);
        if (n == -1)
            return -errno;
        if (n != sizeof(Buffer))
            return 0;
        return size - 1;
    }
}
