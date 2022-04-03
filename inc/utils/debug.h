/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UTILS_DEBUG_H__
#define __UTILS_DEBUG_H__

/**
 * @addtogroup utils
 *
 * @{
 */

#define z_return_val_if_fail_cmp( \
  a, comparator, b, val) \
  if (!(G_LIKELY (a comparator b))) \
    { \
      g_critical ( \
        "Assertion failed: %s (%ld) %s %s (%ld)", \
        #a, (long) a, #comparator, #b, (long) b); \
      return val; \
    }

#define z_return_if_fail_cmp(a, comparator, b) \
  z_return_val_if_fail_cmp (a, comparator, b, )

#define z_warn_if_fail_cmp(a, comparator, b) \
  if (!(G_LIKELY (a comparator b))) \
    { \
      g_warning ( \
        "Assertion failed: %s (%ld) %s %s (%ld)", \
        #a, (long) a, #comparator, #b, (long) b); \
    }

/**
 * @}
 */

#endif
