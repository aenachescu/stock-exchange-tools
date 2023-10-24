#ifndef SCOPED_PTR_H
#define SCOPED_PTR_H

#include "noncopyable.h"
#include "nonmovable.h"

template <typename T, T* (*init)(), void (*cleanup)(T*)>
class ScopedPtr : private noncopyable, private nonmovable {
public:
    ScopedPtr()
    {
        ptr = init();
    }
    ~ScopedPtr()
    {
        cleanup(ptr);
    }

    T* get()
    {
        return ptr;
    }
    const T* get() const
    {
        return ptr;
    }

    T* operator->()
    {
        return ptr;
    }
    const T* operator->() const
    {
        return ptr;
    }

    operator bool() const
    {
        return ptr != nullptr;
    }

private:
    T* ptr;
};

#endif // SCOPED_PTR_H
