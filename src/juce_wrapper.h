// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ZRYTHM_JUCE_H__
#define __ZRYTHM_JUCE_H__

#include "zrythm-config.h"

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wundef"
#  ifndef __clang__
#    pragma GCC diagnostic ignored "-Wduplicated-branches"
#    pragma GCC diagnostic ignored "-Wanalyzer-use-of-uninitialized-value"
#  endif // __clang__
#endif   // __GNUC__

// clang-format off
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
// #include "ext/juce/modules/juce_dsp/juce_dsp.h"
#include "juce_audio_formats/juce_audio_formats.h"
#include "juce_audio_processors/juce_audio_processors.h"
#if ZRYTHM_WITH_JUCE_CLAP_HOSTING
#include "juce_clap_hosting.h"
#endif
// clang-format on

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif // __GNUC__

#endif /* __ZRYTHM_JUCE_H__ */
