// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
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
      g_critical ( \
        "Assertion failed: %s (%" G_GINT64_FORMAT \
        ") " \
        "%s %s (%" G_GINT64_FORMAT ")", \
        #a, (gint64) (a), #comparator, #b, (gint64) (b)); \
      return val; \
    }

#define z_return_if_fail_cmp(a, comparator, b) \
  z_return_val_if_fail_cmp (a, comparator, b, )

#define z_warn_if_fail_cmp(a, comparator, b) \
  if (!(G_LIKELY (a comparator b))) \
    { \
      g_warning ( \
        "Assertion failed: %s (%" G_GINT64_FORMAT \
        ") " \
        "%s %s (%" G_GINT64_FORMAT ")", \
        #a, (gint64) (a), #comparator, #b, (gint64) (b)); \
    }

/**
 * @}
 */

#endif
