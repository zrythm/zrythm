// SPDX-FileCopyrightText: © 2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <string>

namespace backward
{
class SignalHandling;
}

namespace zrythm::utils
{

class Backtrace
{
public:
  Backtrace ();

  /**
   * @brief To be called once at the beginning of the program to setup the
   * signal handlers.
   */
  static std::unique_ptr<backward::SignalHandling> init_signal_handlers ();

public:
  /**
   * Returns the backtrace with @p depth number of elements and a string
   * prefix @p prefix.
   *
   * @param write_to_file Whether to write the backtrace to a file.
   */
  std::string
  get_backtrace (std::string prefix, size_t depth, bool write_to_file);
};

}; // namespace zrythm::utils
