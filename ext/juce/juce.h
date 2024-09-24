// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ZRYTHM_JUCE_H__
#define __ZRYTHM_JUCE_H__

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wundef"
#  ifndef __clang__
#    pragma GCC diagnostic ignored "-Wduplicated-branches"
#    pragma GCC diagnostic ignored "-Wanalyzer-use-of-uninitialized-value"
#  endif // __clang__
#endif   // __GNUC__

#include <modules/juce_audio_basics/juce_audio_basics.h>
#include <modules/juce_core/juce_core.h>
// #include "ext/juce/modules/juce_dsp/juce_dsp.h"
#include <modules/juce_audio_formats/juce_audio_formats.h>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif // __GNUC__

#endif /* __ZRYTHM_JUCE_H__ */
