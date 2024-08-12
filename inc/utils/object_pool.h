// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_OBJECT_POOL_H__
#define __UTILS_OBJECT_POOL_H__

#include <memory>
#include <vector>

#include "utils/mpmc_queue.h"

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

  ObjectPool (size_t initial_capacity = 64)
      : capacity_ (initial_capacity), size_ (0)
  {
    expand ();
  }

  T * acquire ()
  {
    T * object;
    if (available_.pop_front (object))
      {
        return object;
      }

    if (size_ == capacity_)
      {
        expand ();
      }

    return acquire ();
  }

  void release (T * object) { available_.push_back (object); }

  void reserve (size_t size)
  {
    while (size_ < size)
      {
        expand ();
      }
  }

private:
  void expand ()
  {
    size_t old_capacity = capacity_;
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
  size_t                          capacity_;
  size_t                          size_;
};

#endif
