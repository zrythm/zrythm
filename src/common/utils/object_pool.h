// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_OBJECT_POOL_H__
#define __UTILS_OBJECT_POOL_H__

#include <memory>
#include <vector>

#include "common/utils/mpmc_queue.h"

/**
 * @brief Thread-safe, realtime-safe object pool.
 *
 * @tparam T The type of objects to be pooled. Must be default-constructible.
 */
template <typename T> class ObjectPool
{
public:
  static_assert (
    std::is_default_constructible<T>::value,
    "T must be default-constructible");

  ObjectPool (size_t initial_capacity = 64) { reserve (initial_capacity); }

  T * acquire ()
  {
    T * object;
    if (available_.pop_front (object))
      {
#ifdef DEBUG_OBJECT_POOL
        num_in_use.fetch_add (1, std::memory_order_relaxed);
#endif
        return object;
      }

    if (size_ == capacity_)
      {
        expand ();
      }

    return acquire ();
  }

  void release (T * object)
  {
    available_.push_back (object);
#ifdef DEBUG_OBJECT_POOL
    num_in_use.fetch_add (-1);
#endif
  }

  void reserve (size_t size)
  {
    available_.reserve (size);
    while (size_ < size)
      {
        expand ();
      }
  }

#ifdef DEBUG_OBJECT_POOL
  auto get_num_in_use () const { return num_in_use.load (); }
  auto get_capacity () const { return capacity_; }
  auto get_size () const { return size_; }
#endif

private:
  void expand ()
  {
    size_t old_capacity = capacity_;
    if (capacity_ == 0)
      {
        capacity_ = 1;
      }
    capacity_ *= 2;

    buffer_.reserve (capacity_);

    for (size_t i = old_capacity; i < capacity_; ++i)
      {
        buffer_.emplace_back (std::make_unique<T> ());
        available_.push_back (buffer_.back ().get ());
      }

    size_ = capacity_;
  }

  std::vector<std::unique_ptr<T>> buffer_;
  MPMCQueue<T *>                  available_;
  size_t                          capacity_ = 0;
  size_t                          size_ = 0;

#ifdef DEBUG_OBJECT_POOL
  // Debug counter
  std::atomic<size_t> num_in_use;
#endif
};

#endif
