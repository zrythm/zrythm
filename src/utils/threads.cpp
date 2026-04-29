// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/threads.h"

#ifndef _WIN32
#  include <pthread.h>
#endif
#include <thread>

#include <fmt/std.h>

#include <fmt/format.h>

namespace zrythm::utils
{

std::string
get_current_thread_name ()
{
#ifndef _WIN32
  char buf[64] = {};
  pthread_getname_np (pthread_self (), buf, sizeof (buf));
  if (buf[0] != '\0')
    return buf;
#endif
  return fmt::format ("{}", std::this_thread::get_id ());
}

} // namespace zrythm::utils
