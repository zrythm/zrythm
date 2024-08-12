// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef CRILL_SEQLOCK_OBJECT_H
#define CRILL_SEQLOCK_OBJECT_H

#include <cstring>
#include <atomic>

namespace crill {

// A portable C++ implementation of a seqlock inspired by Hans Boehm's paper
// "Can Seqlocks Get Along With Programming Language Memory Models?"
// and the C implementation in jemalloc.
//
// This version allows only a single writer. Writes are guaranteed wait-free.
// It also allows multiple concurrent readers, which are wait-free against
// each other, but can block if there is a concurrent write.
template <typename T>
class seqlock_object
{
public:
    static_assert(std::is_trivially_copyable_v<T>);

    // Creates a seqlock_object with a default-constructed value.
    seqlock_object()
    {
        store(T());
    }

    // Creates a seqlock_object with the given value.
    seqlock_object(T t)
    {
        store(t);
    }

    // Reads and returns the current value.
    // Non-blocking guarantees: wait-free if there are no concurrent writes,
    // otherwise none.
    T load() const noexcept
    {
        T t;
        while (!try_load(t)) /* keep trying */;
        return t;
    }

    // Attempts to read the current value and write it into the passed-in object.
    // Returns: true if the read succeeded, false otherwise.
    // Non-blocking guarantees: wait-free.
    bool try_load(T& t) const noexcept
    {
        std::size_t buffer[buffer_size];

        std::size_t seq1 = seq.load(std::memory_order_acquire);
        if (seq1 % 2 != 0)
            return false;

        for (std::size_t i = 0; i < buffer_size; ++i)
            buffer[i] = data[i].load(std::memory_order_relaxed);

        std::atomic_thread_fence(std::memory_order_acquire);

        std::size_t seq2 = seq.load(std::memory_order_relaxed);
        if (seq1 != seq2)
            return false;

        std::memcpy(&t, buffer, sizeof(T));
        return true;
    }

    // Updates the current value to the value passed in.
    // Non-blocking guarantees: wait-free.
    void store(T t) noexcept
    {
        std::size_t buffer[buffer_size];
        if constexpr (sizeof(T) % sizeof(std::size_t) != 0)
            buffer[buffer_size - 1] = 0;

        std::memcpy(&buffer, &t, sizeof(T));

        std::size_t old_seq = seq.load(std::memory_order_relaxed);
        seq.store(old_seq + 1, std::memory_order_relaxed);

        std::atomic_thread_fence(std::memory_order_release);

        for (std::size_t i = 0; i < buffer_size; ++i)
            data[i].store(buffer[i], std::memory_order_relaxed);

        seq.store(old_seq + 2, std::memory_order_release);
    }

private:
    static constexpr std::size_t buffer_size = (sizeof(T) + sizeof(std::size_t) - 1) / sizeof(std::size_t);
    std::atomic<std::size_t> data[buffer_size];
    std::atomic<std::size_t> seq = 0;

    static_assert(decltype(seq)::is_always_lock_free);
};

} // namespace crill

#endif //CRILL_SEQLOCK_OBJECT_H
