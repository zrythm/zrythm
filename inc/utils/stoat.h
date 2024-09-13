// SPDX-FileCopyrightText: © 2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_STOAT_H__
#define __UTILS_STOAT_H__

#if defined(__clang__)
#  ifdef ATTR_REALTIME
#    undef ATTR_REALTIME
#  endif
#  ifdef ATTR_NONREALTIME
#    undef ATTR_NONREALTIME
#  endif
#  define ATTR_REALTIME __attribute__ ((annotate ("realtime")))
#  define ATTR_NONREALTIME __attribute__ ((annotate ("nonrealtime")))
#else
#  define ATTR_REALTIME
#  define ATTR_NONREALTIME
#endif

#endif
