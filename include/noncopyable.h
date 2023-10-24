#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

struct noncopyable
{
    noncopyable()  = default;
    ~noncopyable() = default;

    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

    noncopyable(noncopyable&&) = default;
    noncopyable& operator=(noncopyable&&) = default;
};

#endif // NONCOPYABLE_H
