// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_BACKTRACE_H__
#define __UTILS_BACKTRACE_H__

#include <string>

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
  static void init_signal_handlers ();

public:
  /**
   * Returns the backtrace with @p depth number of elements and a string
   * prefix @p prefix.
   *
   * @param write_to_file Whether to write the backtrace to a file.
   */
  std::string get_backtrace (std::string prefix, int depth, bool write_to_file);
};

}; // namespace zrythm::utils

#endif
