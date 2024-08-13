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
  if (!(G_LIKELY ((a) comparator (b)))) \
    { \
      z_error ( \
        "Assertion failed: {} ({}) {} {} ({})", #a, (gint64) (a), #comparator, \
        #b, (gint64) (b)); \
      return val; \
    }

#define z_return_if_fail_cmp(a, comparator, b) \
  z_return_val_if_fail_cmp (a, comparator, b, )

#define z_warn_if_fail_cmp(a, comparator, b) \
  if (!(G_LIKELY (a comparator b))) \
    { \
      z_warning ( \
        "Assertion failed: {} ({}) {} {} ({})", #a, (gint64) (a), #comparator, \
        #b, (gint64) (b)); \
    }

/**
 * @}
 */

#endif
