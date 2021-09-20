#ifndef IPC_WIN32_SHARED_MEMORY_H_
#define IPC_WIN32_SHARED_MEMORY_H_

#pragma comment(lib, "Advapi32.lib")

#include <string.h>
#include <windows.h>

namespace ipc::win32 {

    template <typename T>
    class shared_memory {
    private:
        HANDLE hFileMap;
        char* pName;
        T* pBuffer;
        bool bCreated;

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
        int close(bool destroy = true);
    };

    template <typename T>
    shared_memory<T>::shared_memory()
        : hFileMap(nullptr)
        , pName(nullptr)
        , pBuffer(nullptr)
        , bCreated(false)
    {
    }

    template <typename T>
    shared_memory<T>::shared_memory(const char* name)
        : shared_memory<T>()
    {
        this->pName = strdup(name);
    }

    template <typename T>
    shared_memory<T>::~shared_memory()
    {
        errno = close();
        free(pName);
    }

    template <typename T>
    shared_memory<T>::shared_memory(shared_memory&& other)
        : shared_memory<T>()
    {
        std::swap(hFileMap, other.hFileMap);
        std::swap(pName, other.pName);
        std::swap(pBuffer, other.pBuffer);
        std::swap(bCreated, other.bCreated);
    }

    template <typename T>
    shared_memory<T>& shared_memory<T>::operator=(shared_memory&& other)
    {
        std::swap(hFileMap, other.hFileMap);
        std::swap(pName, other.pName);
        std::swap(pBuffer, other.pBuffer);
        std::swap(bCreated, other.bCreated);
        return *this;
    }

    template <typename T>
    T& shared_memory<T>::operator*()
    {
        return *pBuffer;
    }

    template <typename T>
    T* shared_memory<T>::operator->()
    {
        return pBuffer;
    }

    template <typename T>
    T* shared_memory<T>::operator&()
    {
        return pBuffer;
    }

    template <typename T>
    bool shared_memory<T>::is_opened() const
    {
        return hFileMap != nullptr;
    }

    template <typename T>
    bool shared_memory<T>::is_created() const
    {
        return bCreated;
    }

    template <typename T>
    int shared_memory<T>::open()
    {
        if (is_opened()) {
            return 0;
        }

        // security
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;

        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = &sd;

        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
            return ::GetLastError();
        }
        if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE)) {
            return ::GetLastError();
        }

        // Shared Memory
        hFileMap = ::CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            &sa,
            PAGE_READWRITE,
            0,
            sizeof(T),
            pName);
        if (hFileMap == nullptr) {
            return ::GetLastError();
        }
        bCreated = (::GetLastError() != ERROR_ALREADY_EXISTS);

        pBuffer = static_cast<T*>(::MapViewOfFile(
            hFileMap,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            512));
        if (pBuffer == nullptr) {
            int err = ::GetLastError();
            close();
            return err;
        }

        return 0;
    }

    template <typename T>
    int shared_memory<T>::close(bool destroy /* unused */)
    {
        if (pBuffer != nullptr) {
            UnmapViewOfFile(pBuffer);
            pBuffer = nullptr;
        }
        if (hFileMap != nullptr) {
            CloseHandle(hFileMap);
            hFileMap = nullptr;
        }
        return 0;
    }
}

#endif /* IPC_WIN32_SHARED_MEMORY_H_ */