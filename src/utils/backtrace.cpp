// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "utils/backtrace.h"
#include "utils/datetime.h"
#include "utils/io.h"

#include <backward.hpp>

namespace zrythm::utils
{

Backtrace::Backtrace () = default;

void
Backtrace::init_signal_handlers ()
{
#if defined(__has_feature) && __has_feature(thread_sanitizer)
#else
  // this attaches signal handlers automatically
  static backward::SignalHandling sh;
#endif
}

std::string
Backtrace::get_backtrace (std::string prefix, size_t depth, bool write_to_file)
{
  // Create a backward::StackTrace object
  backward::StackTrace st;
  st.load_here (depth);

  // skip this function and the one that called it
  st.skip_n_firsts (2);

  // Create a backward::Printer object
  backward::Printer p;

  // Configure the printer (optional)
  p.object = true;
  p.color_mode = backward::ColorMode::always;
  p.address = true;

  // Print the stack trace to a string
  std::ostringstream oss;
  p.print (st, oss);

  return oss.str ();
}

} // namespace zrythm::utils
