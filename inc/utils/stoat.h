// SPDX-FileCopyrightText: © 2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_STOAT_H__
#define __UTILS_STOAT_H__

#if defined(__clang__)
#  ifdef REALTIME
#    undef REALTIME
#  endif
#  ifdef NONREALTIME
#    undef NONREALTIME
#  endif
#  define REALTIME __attribute__ ((annotate ("realtime")))
#  define NONREALTIME \
    __attribute__ ((annotate ("nonrealtime")))
#else
#  define REALTIME
#  define NONREALTIME
#endif

#endif
