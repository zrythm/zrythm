// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once
#include <atomic>

#include <QObject>

/**
 * @brief Interface for real-time property updates.
 */
class IRealtimeProperty
{
public:
  virtual ~IRealtimeProperty () = default;

  /**
   * @brief Process pending updates from real-time thread.
   * @return true if updates were processed
   */
  virtual bool processUpdates () = 0;
};

/**
 * @brief Template class for real-time safe property updates between threads.
 *
 * Provides atomic updates with separate main thread and real-time thread values.
 */
template <typename T> class RealtimeProperty : public IRealtimeProperty
{
public:
  RealtimeProperty (T initial = T{}) : value_ (initial), pending_ (initial) { }

  // Real-time safe setter
  void setRT (const T &newValue)
  {
    pending_.store (newValue, std::memory_order_release);
    has_update_.store (true, std::memory_order_release);
  }

  // Real-time safe getter
  T getRT () const { return pending_.load (std::memory_order_acquire); }

  // Non-RT getter (for main thread/property system)
  T get () const { return value_; }

  // Non-RT setter
  void set (const T &newValue)
  {
    value_ = newValue;
    pending_.store (newValue, std::memory_order_release);
    has_update_.store (false, std::memory_order_release);
  }

  bool processUpdates () override
  {
    if (!has_update_.load (std::memory_order_acquire))
      {
        return false;
      }

    value_ = pending_.load (std::memory_order_acquire);
    has_update_.store (false, std::memory_order_release);
    return true;
  }

private:
  T                 value_;   // Main thread value
  std::atomic<T>    pending_; // Real-time thread value
  std::atomic<bool> has_update_{ false };
};