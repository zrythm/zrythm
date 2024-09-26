// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>

/**
 * @brief Real-time safe thread identifier.
 *
 * Provides a unique identifier for each thread without using
 * non-real-time safe operations like std::this_thread::get_id().
 */
class RTThreadId
{
private:
  // Atomic counter for generating unique IDs
  static std::atomic<unsigned int> next_id;

  // The unique ID for this thread
  unsigned int id;

public:
  RTThreadId ();

  using IdType = unsigned int;

  [[nodiscard]] IdType get () const;
  bool         operator== (const RTThreadId &other) const;
  bool         operator!= (const RTThreadId &other) const;
};

// Thread-local instance for easy access to the current thread's ID
extern thread_local RTThreadId current_thread_id;