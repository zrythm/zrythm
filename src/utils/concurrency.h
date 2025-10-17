// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <cassert>
#include <semaphore>

#include <moodycamel/lightweightsemaphore.h>

/**
 * @brief RAII class for managing the lifetime of an atomic bool.
 *
 * This class provides a convenient and safe way to manage the lifetime of an
 * atomic bool using the RAII (Resource Acquisition Is Initialization) idiom.
 * When an instance of AtomicBoolRAII is created, it sets the associated atomic
 * bool to true. When the instance goes out of scope, the destructor sets the
 * atomic bool back to false.
 *
 * Usage example:
 * @code
 * std::atomic<bool> flag = false;
 *
 * void someFunction() {
 *     AtomicBoolRAII raii(flag);
 *     // Do some work while the flag is set to true
 * }
 *
 * // After someFunction() returns, the flag is automatically set back to false
 * @endcode
 */
class AtomicBoolRAII
{
private:
  std::atomic<bool> &m_flag;

public:
  explicit AtomicBoolRAII (std::atomic<bool> &flag) : m_flag (flag)
  {
    m_flag.store (true, std::memory_order_release);
  }

  ~AtomicBoolRAII () { m_flag.store (false, std::memory_order_release); }

  // Disable copy and move operations
  AtomicBoolRAII (const AtomicBoolRAII &) = delete;
  AtomicBoolRAII &operator= (const AtomicBoolRAII &) = delete;
  AtomicBoolRAII (AtomicBoolRAII &&) = delete;
  AtomicBoolRAII &operator= (AtomicBoolRAII &&) = delete;
};

/**
 * @brief RAII wrapper class for std::binary_semaphore.
 *
 * This class provides a convenient and safe way to acquire and release a binary
 * semaphore using the RAII (Resource Acquisition Is Initialization) idiom. It
 * ensures that the semaphore is properly released when the wrapper object goes
 * out of scope.
 */
template <typename SemaphoreType = std::binary_semaphore> class SemaphoreRAII
{
public:
  /**
   * @brief Constructor that attempts to acquire the semaphore.
   *
   * @param semaphore The semaphore to acquire.
   */
  explicit SemaphoreRAII (SemaphoreType &semaphore, bool force_acquire = false)
      : semaphore_ (semaphore)
  {
    if (force_acquire)
      {
        if constexpr (
          std::is_same_v<SemaphoreType, moodycamel::LightweightSemaphore>)
          {
            semaphore_.wait ();
          }
        else
          {
            semaphore_.acquire ();
          }
        acquired_ = true;
      }
    else
      {
        if constexpr (
          std::is_same_v<SemaphoreType, moodycamel::LightweightSemaphore>)
          {
            acquired_ = semaphore_.tryWait ();
          }
        else
          {
            acquired_ = semaphore_.try_acquire ();
          }
      }
  }

  /**
   * @brief Destructor that releases the semaphore if it was acquired.
   */
  ~SemaphoreRAII ()
  {
    if (acquired_)
      {
        if constexpr (
          std::is_same_v<SemaphoreType, moodycamel::LightweightSemaphore>)
          {
            semaphore_.signal ();
          }
        else
          {
            semaphore_.release ();
          }
      }
  }

  SemaphoreRAII (const SemaphoreRAII &) = delete;
  SemaphoreRAII &operator= (const SemaphoreRAII &) = delete;
  SemaphoreRAII (SemaphoreRAII &&) = default;
  SemaphoreRAII &operator= (SemaphoreRAII &&) = default;

  /**
   * @brief Attempts to acquire the semaphore if it hasn't been acquired already.
   *
   * @return true if the semaphore is successfully acquired, false otherwise.
   */
  bool try_acquire ()
  {
    if (!acquired_)
      {
        if constexpr (
          std::is_same_v<SemaphoreType, moodycamel::LightweightSemaphore>)
          {
            acquired_ = semaphore_.tryWait ();
          }
        else
          {
            acquired_ = semaphore_.try_acquire ();
          }
      }
    return acquired_;
  }

  /**
   * @brief Releases the semaphore if it was previously acquired.
   */
  void release ()
  {
    if (acquired_)
      {
        if constexpr (
          std::is_same_v<SemaphoreType, moodycamel::LightweightSemaphore>)
          {
            semaphore_.signal ();
          }
        else
          {
            semaphore_.release ();
          }
        acquired_ = false;
      }
  }

  /**
   * @brief Checks if the semaphore is currently acquired by the wrapper object.
   *
   * @return true if the semaphore is acquired, false otherwise.
   */
  bool is_acquired () const { return acquired_; }

private:
  SemaphoreType &semaphore_; ///< Reference to the binary semaphore.
  bool acquired_ = false;    ///< Flag indicating if the semaphore is acquired.
};
