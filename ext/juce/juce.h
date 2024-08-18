// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ZRYTHM_JUCE_H__
#define __ZRYTHM_JUCE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#  pragma GCC diagnostic ignored "-Wanalyzer-use-of-uninitialized-value"
#endif
#include "ext/juce/modules/juce_core/juce_core.h"
#include "ext/juce/modules/juce_audio_basics/juce_audio_basics.h"
//#include "ext/juce/modules/juce_dsp/juce_dsp.h"
#include "ext/juce/modules/juce_audio_formats/juce_audio_formats.h"
#pragma GCC diagnostic pop

#endif /* __ZRYTHM_JUCE_H__ */
