// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright 2011-2022 David Robillard <d@drobilla.net>
 * SPDX-License-Identifier: ISC
 */

#ifndef __UTILS_RING_BUFFER_H__
#define __UTILS_RING_BUFFER_H__

#include <atomic>
#include <cstddef>
#include <memory>

/**
 * @brief A ring buffer implementation for storing elements of type `T`.
 *
 * The `RingBuffer` class provides a thread-safe, fixed-size buffer that allows
 * for efficient reading and writing of elements. It uses atomic operations to
 * ensure thread safety.
 *
 * The buffer has a fixed capacity, which is one more than the size specified
 * in the constructor. This extra slot is used to differentiate between a full
 * and an empty buffer.
 *
 * The `write` and `read` functions return `true` if the operation was
 * successful, and `false` otherwise (e.g., if the buffer is full or empty,
 * respectively).
 *
 * The `reset` function can be used to reset the read and write heads to the
 * beginning of the buffer.
 *
 * The `capacity`, `write_space`, and `read_space` functions can be used to
 * query the current state of the buffer.
 */
template <typename T> class RingBuffer
{
public:
  explicit RingBuffer (size_t size)
      : size_ (size + 1), buf_ (std::make_unique<T[]> (size_)), read_head_ (0),
        write_head_ (0)
  {
  }

  RingBuffer (const RingBuffer &other)
      : size_ (other.size_), buf_ (std::make_unique<T[]> (other.size_)),
        read_head_ (other.read_head_.load ()),
        write_head_ (other.write_head_.load ())
  {
    std::copy (other.buf_.get (), other.buf_.get () + size_, buf_.get ());
  }

  RingBuffer (RingBuffer &&other) noexcept
      : size_ (other.size_), buf_ (std::move (other.buf_)),
        read_head_ (other.read_head_.load ()),
        write_head_ (other.write_head_.load ())
  {
    other.size_ = 0;
    other.read_head_.store (0);
    other.write_head_.store (0);
  }

#if 0
  RingBuffer &operator= (const RingBuffer &other)
  {
    if (this != &other)
      {
        size_ = other.size_;
        buf_ = std::make_unique<T[]> (other.size_);
        std::copy (other.buf_.get (), other.buf_.get () + size_, buf_.get ());
        read_head_.store (other.read_head_.load ());
        write_head_.store (other.write_head_.load ());
      }
    return *this;
  }

  RingBuffer &operator= (RingBuffer &&other) noexcept
  {
    if (this != &other)
      {
        size_ = other.size_;
        buf_ = std::move (other.buf_);
        read_head_.store (other.read_head_.load ());
        write_head_.store (other.write_head_.load ());
        other.size_ = 0;
        other.read_head_.store (0);
        other.write_head_.store (0);
      }
    return *this;
  }
#endif

  bool write (const T &src)
  {
    const size_t write_pos = write_head_.load (std::memory_order_relaxed);
    const size_t next_pos = increment_pos (write_pos);
    if (next_pos == read_head_.load (std::memory_order_acquire))
      {
        return false;
      }
    buf_[write_pos] = src;
    write_head_.store (next_pos, std::memory_order_release);
    return true;
  }

  void force_write (const T &src)
  {
    const size_t write_pos = write_head_.load (std::memory_order_relaxed);
    const size_t next_pos = increment_pos (write_pos);

    if (next_pos == read_head_.load (std::memory_order_acquire))
      {
        // Buffer is full, drop the element at the read head
        read_head_.store (
          increment_pos (read_head_.load (std::memory_order_relaxed)),
          std::memory_order_release);
      }

    buf_[write_pos] = src;
    write_head_.store (next_pos, std::memory_order_release);
  }

  bool write_multiple (const T * src, size_t count)
  {
    const size_t write_pos = write_head_.load (std::memory_order_relaxed);
    const size_t read_pos = read_head_.load (std::memory_order_acquire);
    const size_t available_space = (read_pos - write_pos - 1 + size_) % size_;

    if (count > available_space)
      {
        return false;
      }

    for (size_t i = 0; i < count; ++i)
      {
        buf_[(write_pos + i) % size_] = src[i];
      }

    write_head_.store ((write_pos + count) % size_, std::memory_order_release);
    return true;
  }

  void force_write_multiple (const T * src, size_t count)
  {
    size_t write_pos = write_head_.load (std::memory_order_relaxed);
    size_t read_pos = read_head_.load (std::memory_order_acquire);

    for (size_t i = 0; i < count; ++i)
      {
        buf_[write_pos] = src[i];
        write_pos = increment_pos (write_pos);

        if (write_pos == read_pos)
          {
            read_pos = increment_pos (read_pos);
          }
      }

    write_head_.store (write_pos, std::memory_order_release);
    read_head_.store (read_pos, std::memory_order_release);
  }

  bool skip (size_t num_elements)
  {
    const size_t read_pos = read_head_.load (std::memory_order_relaxed);
    const size_t write_pos = write_head_.load (std::memory_order_acquire);
    const size_t available_elements = (write_pos - read_pos) % size_;

    if (num_elements > available_elements)
      {
        return false;
      }

    read_head_.store (
      (read_pos + num_elements) % size_, std::memory_order_release);
    return true;
  }

  bool read (T &dst)
  {
    const size_t read_pos = read_head_.load (std::memory_order_relaxed);
    if (read_pos == write_head_.load (std::memory_order_acquire))
      {
        return false;
      }
    dst = buf_[read_pos];
    read_head_.store (increment_pos (read_pos), std::memory_order_release);
    return true;
  }

  /**
   * @brief Peek a single element from the ring buffer without moving the read
   * head.
   *
   * This function attempts to peek the next element from the current read
   * position without modifying the state of the buffer.
   *
   * @param[out] dst Reference to the variable where the peeked element will be
   * stored.
   * @return true if an element was successfully peeked, false if the buffer is
   * empty.
   */
  bool peek (T &dst) const
  {
    const size_t read_pos = read_head_.load (std::memory_order_acquire);
    const size_t write_pos = write_head_.load (std::memory_order_acquire);

    if (read_pos == write_pos)
      {
        return false; // Buffer is empty
      }

    dst = buf_[read_pos];
    return true;
  }

  /**
   * @brief Peek multiple elements from the ring buffer without moving the read
   * head.
   *
   * This function attempts to peek up to the requested number of elements from
   * the ring buffer, starting from the current read position. It does not
   * modify the state of the buffer.
   *
   * @param[out] dst Pointer to the destination array where peeked elements will
   * be stored.
   * @param[in] count Maximum number of elements to peek.
   * @return The number of elements actually peeked, which may be less than or
   * equal to count.
   */
  size_t peek_multiple (T * dst, size_t count) const
  {
    const size_t read_pos = read_head_.load (std::memory_order_acquire);
    const size_t write_pos = write_head_.load (std::memory_order_acquire);
    const size_t available = (write_pos - read_pos + size_) % size_;

    size_t peeked = std::min (count, available);

    for (size_t i = 0; i < peeked; ++i)
      {
        dst[i] = buf_[(read_pos + i) % size_];
      }

    return peeked;
  }

  void reset ()
  {
    read_head_.store (0, std::memory_order_relaxed);
    write_head_.store (0, std::memory_order_relaxed);
  }

  size_t capacity () const { return size_ - 1; }

  size_t write_space () const
  {
    size_t read_pos = read_head_.load (std::memory_order_relaxed);
    size_t write_pos = write_head_.load (std::memory_order_relaxed);
    if (write_pos >= read_pos)
      {
        return (size_ - write_pos) + read_pos - 1;
      }
    else
      {
        return read_pos - write_pos - 1;
      }
  }

  size_t read_space () const
  {
    size_t read_pos = read_head_.load (std::memory_order_relaxed);
    size_t write_pos = write_head_.load (std::memory_order_relaxed);
    if (write_pos >= read_pos)
      {
        return write_pos - read_pos;
      }
    else
      {
        return (size_ - read_pos) + write_pos;
      }
  }

  bool can_read_multiple (size_t count) const { return count <= read_space (); }

  bool read_multiple (T * dst, size_t count)
  {
    const size_t read_pos = read_head_.load (std::memory_order_relaxed);
    const size_t write_pos = write_head_.load (std::memory_order_acquire);
    const size_t available = (write_pos - read_pos + size_) % size_;

    if (count > available)
      {
        return false;
      }

    for (size_t i = 0; i < count; ++i)
      {
        dst[i] = buf_[(read_pos + i) % size_];
      }

    read_head_.store ((read_pos + count) % size_, std::memory_order_release);
    return true;
  }

private:
  size_t increment_pos (size_t pos) const { return (pos + 1) % size_; }

  const size_t         size_;
  std::unique_ptr<T[]> buf_;
  std::atomic<size_t>  read_head_;
  std::atomic<size_t>  write_head_;
};

#endif // __UTILS_RING_BUFFER_H__