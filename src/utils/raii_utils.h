// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cassert>

/**
 * @brief RAII wrapper that sets a bool to true on construction and false on
 * destruction.
 */
class ScopedBool
{
public:
  explicit ScopedBool (bool &flag) : flag_ (flag)
  {
    assert (!flag_);
    flag_ = true;
  }
  ~ScopedBool ()
  {
    assert (flag_);
    flag_ = false;
  }

  ScopedBool (const ScopedBool &) = delete;
  ScopedBool &operator= (const ScopedBool &) = delete;
  ScopedBool (ScopedBool &&) = delete;
  ScopedBool &operator= (ScopedBool &&) = delete;

private:
  bool &flag_;
};
