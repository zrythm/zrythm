// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_DEBUG_H__
#define __UTILS_DEBUG_H__

/**
 * @addtogroup utils
 *
 * @{
 */

#define z_return_val_if_fail_cmp(a, comparator, b, val) \
  if (!((a) comparator (b))) [[unlikely]] \
    { \
      z_error ( \
        "Assertion failed: `{}` ({}) {} `{}` ({})", #a, (a), #comparator, #b, \
        (b)); \
      return val; \
    }

#define z_return_if_fail_cmp(a, comparator, b) \
  z_return_val_if_fail_cmp (a, comparator, b, )

#define z_warn_if_fail_cmp(a, comparator, b) \
  if (!((a) comparator (b))) [[unlikely]] \
    { \
      z_warning ( \
        "Assertion failed: `{}` ({}) {} `{}` ({})", #a, (a), #comparator, #b, \
        (b)); \
    }

#ifdef _MSC_VER
#  include <intrin.h>
#  define DEBUG_BREAK() __debugbreak ()
#elif defined(__APPLE__)
#  include <TargetConditionals.h>
#  if TARGET_OS_MAC
#    define DEBUG_BREAK() __builtin_debugtrap ()
#  endif
#elif defined(__linux__) || defined(__unix__)
#  include <csignal>
#  define DEBUG_BREAK() raise (SIGTRAP)
#else
#  define DEBUG_BREAK() \
    do \
      { \
        volatile int * p = 0; \
        *p = 0; \
      } \
    while (0)
#endif

/**
 * @}
 */

#endif
