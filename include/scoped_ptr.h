#ifndef SCOPED_PTR_H
#define SCOPED_PTR_H

#include "noncopyable.h"
#include "nonmovable.h"

template <typename T, T* (*init)(), void (*cleanup)(T*)>
class ScopedPtr : private noncopyable, private nonmovable {
public:
    ScopedPtr()
    {
        m_ptr = init();
    }
    ~ScopedPtr()
    {
        cleanup(m_ptr);
    }

    T* Get()
    {
        return m_ptr;
    }
    const T* Get() const
    {
        return m_ptr;
    }

    T* operator->()
    {
        return m_ptr;
    }
    const T* operator->() const
    {
        return m_ptr;
    }

    operator bool() const
    {
        return m_ptr != nullptr;
    }

private:
    T* m_ptr;
};

#endif // SCOPED_PTR_H
