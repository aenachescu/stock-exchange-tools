#ifndef NONMOVABLE_H
#define NONMOVABLE_H

struct nonmovable
{
    nonmovable()  = default;
    ~nonmovable() = default;

    nonmovable(const nonmovable&) = default;
    nonmovable& operator=(const nonmovable&) = default;

    nonmovable(nonmovable&&) = delete;
    nonmovable& operator=(nonmovable&&) = delete;
};

#endif // NONMOVABLE_H
