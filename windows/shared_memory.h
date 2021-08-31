#ifndef SHARED_MEMORY_H_
#define SHARED_MEMORY_H_

#pragma comment(lib, "Advapi32.lib")

#include <string.h>
#include <windows.h>

template <typename T>
class shared_memory {
private:
    HANDLE hFileMap;
    char* pName;
    T* pBuffer;
    bool bMemOwner;

public:
    shared_memory(const char* name);
    ~shared_memory();

    T& operator*();
    T* operator->();
    T* operator&();

    bool is_owner() const;
    bool is_opened() const;

    int open();
    int close();
};

template <typename T>
inline shared_memory<T>::shared_memory(const char* name)
    : hFileMap(nullptr)
    , pBuffer(nullptr)
    , bMemOwner(false)
{
    this->pName = strdup(name);
}

template <typename T>
inline shared_memory<T>::~shared_memory()
{
    close();
    free(pName);
}

template <typename T>
inline T& shared_memory<T>::operator*()
{
    return *pBuffer;
}

template <typename T>
inline T* shared_memory<T>::operator->()
{
    return pBuffer;
}

template <typename T>
inline T* shared_memory<T>::operator&()
{
    return pBuffer;
}

template <typename T>
inline bool shared_memory<T>::is_owner() const
{
    return bMemOwner;
}

template <typename T>
inline bool shared_memory<T>::is_opened() const
{
    return hFileMap != nullptr;
}

template <typename T>
inline int shared_memory<T>::open()
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
    bMemOwner = false;
    hFileMap = ::OpenFileMappingA(FILE_MAP_READ, FALSE, pName);
    if (hFileMap == nullptr) {
        bMemOwner = true;
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
    }

    pBuffer = static_cast<T*>(::MapViewOfFile(
        hFileMap,
        FILE_MAP_READ,
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
inline int shared_memory<T>::close()
{
    if (hFileMap != nullptr) {
        CloseHandle(hFileMap);
        hFileMap = nullptr;
    }
    return 0;
}

#endif