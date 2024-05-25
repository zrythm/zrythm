// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ZRYTHM_JUCE_H__
#define __ZRYTHM_JUCE_H__

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#if defined(IS_DEBUG_BUILD) && IS_DEBUG_BUILD
#define JUCE_DEBUG 1
#else
#define JUCE_DEBUG 0
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "ext/juce/modules/juce_core/juce_core.h"
#pragma GCC diagnostic pop

#endif /* __ZRYTHM_JUCE_H__ */
