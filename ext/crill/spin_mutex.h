// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef CRILL_SPIN_MUTEX_H
#define CRILL_SPIN_MUTEX_H

#include <atomic>
#include <mutex>

#include <crill/progressive_backoff_wait.h>

namespace crill
{

// crill::spin_mutex is a spinlock with progressive backoff for safely and
// efficiently synchronising a real-time thread with other threads.
//
// crill::spin_mutex meets the standard C++ requirements for mutex
// [thread.req.lockable.req] and can therefore be used with std::scoped_lock
// and std::unique_lock as a drop-in replacement for std::mutex.
//
// try_lock() and unlock() are implemented by setting a std::atomic_flag and
// are therefore always wait-free. This is the main difference to a std::mutex
// which doesn't have a wait-free unlock() as std::mutex::unlock() may perform
// a system call to wake up a waiting thread.
//
// lock() is implemented with crill::progressive_backoff_wait, to prevent
// wasting energy and allow other threads to progress.
//
// crill::spin_mutex is not recursive; repeatedly locking it on the same thread
// is undefined behaviour (in practice, it will probably deadlock your app).
class spin_mutex
{
public:
  // Effects: Acquires the lock. If necessary, blocks until the lock can be
  // acquired. Preconditions: The current thread does not already hold the lock.
  void lock () noexcept
  {
    progressive_backoff_wait ([this] { return try_lock (); });
  }

  // Effects: Attempts to acquire the lock without blocking.
  // Returns: true if the lock was acquired, false otherwise.
  // Non-blocking guarantees: wait-free.
  bool try_lock () noexcept
  {
    return !flag.test_and_set (std::memory_order_acquire);
  }

  // Effects: Releases the lock.
  // Preconditions: The lock is being held by the current thread.
  // Non-blocking guarantees: wait-free.
  void unlock () noexcept { flag.clear (std::memory_order_release); }

private:
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

} // namespace crill

#endif // CRILL_SPIN_MUTEX_H
