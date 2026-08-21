// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/logger.h"
#include "utils/threads.h"

#ifndef _WIN32
#  include <pthread.h>
#endif
#if !defined _WIN32 && defined __GLIBC__
#  include <dlfcn.h>
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

// This function incorporates work covered by the following copyright and
// permission notice (from Ardour's libs/pbd/pthread_utils.cc):
//
// Copyright (C) 2002-2015 Paul Davis <paul@linuxaudiosystems.com>
// Copyright (C) 2007-2009 David Robillard <d@drobilla.net>
// Copyright (C) 2015-2024 Robin Gareus <robin@gareus.org>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// SPDX-License-Identifier: GPL-2.0-or-later
size_t
rt_worker_stack_size (const size_t base_bytes)
{
#if !defined _WIN32 && defined __GLIBC__
  size_t pt_min_stack = 16384;

#  ifdef PTHREAD_STACK_MIN
  pt_min_stack = static_cast<size_t> (PTHREAD_STACK_MIN);
#  endif

  void * handle = dlopen (nullptr, RTLD_LAZY);
  if (handle == nullptr)
    {
      z_warning ("dlopen failed: {}", dlerror ());
      return base_bytes;
    }

  /* This function is internal (it has a GLIBC_PRIVATE) version, but
   * available via weak symbol, or dlsym, and returns
   *
   * GLRO(dl_pagesize) + __static_tls_size + PTHREAD_STACK_MIN
   */
  auto * __pthread_get_minstack = (size_t (*) (const pthread_attr_t *)) dlsym (
    handle, "__pthread_get_minstack");

  if (__pthread_get_minstack != nullptr)
    {
      pthread_attr_t attr;
      pthread_attr_init (&attr);
      const size_t rv = __pthread_get_minstack (&attr);
      pthread_attr_destroy (&attr);
      dlclose (handle);
      if (rv < pt_min_stack)
        {
          z_warning (
            "__pthread_get_minstack returned {}, less than PTHREAD_STACK_MIN "
            "({}); not padding the stack size",
            rv, pt_min_stack);
          return base_bytes;
        }
      return base_bytes + (rv - pt_min_stack);
    }
  dlclose (handle);
#endif
  return base_bytes;
}

} // namespace zrythm::utils
