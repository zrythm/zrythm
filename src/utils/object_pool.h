// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <vector>

#include "utils/mpmc_queue.h"

/**
 * @brief Thread-safe, realtime-safe object pool.
 *
 * @tparam T The type of objects to be pooled. Must be default-constructible.
 */

template <typename T, bool EnableDebug = false>
class [[deprecated ("Use a pre-filled MPMCQueue instead")]] ObjectPool
{
public:
  static_assert (
    std::is_default_constructible_v<T>,
    "T must be default-constructible");

  ObjectPool (size_t initial_capacity = 64) { reserve (initial_capacity); }

  T * acquire ()
  {
    T * object;
    if (available_.pop_front (object))
      {
        if constexpr (EnableDebug)
          {
            num_in_use.fetch_add (1, std::memory_order_relaxed);
          }
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
    if constexpr (EnableDebug)
      {
        num_in_use.fetch_add (-1);
      }
  }

  void reserve (size_t size)
  {
    available_.reserve (size);
    while (size_ < size)
      {
        expand ();
      }
  }

  auto get_num_in_use () const { return num_in_use.load (); }
  auto get_capacity () const { return capacity_.load (); }
  auto get_size () const { return size_.load (); }

private:
  void expand ()
  {
    std::lock_guard<std::mutex> lock (expand_mutex_);

    // Check again after acquiring lock in case another thread already expanded
    if (size_ == capacity_)
      {
        size_t old_capacity = capacity_;
        if (capacity_ == 0)
          {
            capacity_ = 1;
          }
        capacity_.store (capacity_ * 2);

        buffer_.reserve (capacity_);

        for (size_t i = old_capacity; i < capacity_; ++i)
          {
            buffer_.emplace_back (std::make_unique<T> ());
            available_.push_back (buffer_.back ().get ());
          }

        size_.store (capacity_.load ());
      }
  }

  std::vector<std::unique_ptr<T>> buffer_;
  MPMCQueue<T *>                  available_;
  std::atomic<size_t>             capacity_{ 0 };
  std::atomic<size_t>             size_{ 0 };
  std::mutex                      expand_mutex_;

  // Debug counter
  std::atomic<size_t> num_in_use;
};
