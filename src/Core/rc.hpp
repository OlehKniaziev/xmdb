#pragma once

#include "new.hpp"
#include "ok.hpp"
#include "util.hpp"

namespace xmdb
{
template <typename T>
class Rc
{
public:
    Rc(ok::Allocator *allocator, T *ptr) :
        m_allocator{allocator},
        m_control_block{new(allocator) ControlBlock{
                .count = 1,
                .ptr = ptr,
        }}
    {
    }

    Rc(const Rc<T> &other) :
        m_allocator{other.m_allocator}, m_control_block{other.m_control_block}
    {
        acquire();
    }

    Rc &operator=(const Rc<T> &other)
    {
        m_allocator = other.m_allocator;
        m_control_block = other.m_control_block;

        acquire();
    }

    T *operator->()
    {
        return m_control_block->ptr;
    }

    const T *operator->() const
    {
        return m_control_block->ptr;
    }

    operator T *()
    {
        return m_control_block->ptr;
    }

    void release()
    {
        --m_control_block->count;

        if (m_control_block->count == 0 && m_allocator != nullptr)
        {
            m_allocator->dealloc(m_control_block->ptr, 1);
            m_allocator->dealloc(m_control_block, 1);

            m_allocator = nullptr;
            m_control_block = nullptr;
        }
    }

    ~Rc()
    {
        release();
    }

private:
    void acquire()
    {
        ++m_control_block->count;
    }

    struct ControlBlock
    {
        // @Multithreading This should be atomic.
        U64 count;
        T *ptr;
    };

    ok::Allocator *m_allocator;
    ControlBlock *m_control_block;
};

template <typename T, typename... Args>
Rc<T> make_rc(ok::Allocator *allocator, Args &&...args)
{
    return Rc<T>{allocator, new (allocator) T{forward<Args>(args)...}};
}
} // namespace xmdb
