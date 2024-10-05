// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/utils/backtrace.h"
#include "common/utils/datetime.h"
#include "common/utils/io.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include "backward-cpp/backward.hpp"
#pragma GCC diagnostic pop

Backtrace::Backtrace () = default;

void
Backtrace::init_signal_handlers ()
{
  // this attaches signal handlers automatically
  static backward::SignalHandling sh;
}

std::string
Backtrace::get_backtrace (std::string prefix, int depth, bool write_to_file)
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