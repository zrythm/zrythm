// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 *     ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com
 *
 * Tracktion Engine is published under a dual [GPL3 (or later)](https://www.gnu.org/licenses/gpl-3.0.en.html)/[Commercial license](https://www.tracktion.com/develop/tracktion-engine).
 *
 * ---
 */

/**
 * \file
 *
 * Ditherer.
 */

#ifndef __AUDIO_DITHER_H__
#define __AUDIO_DITHER_H__

#include "utils/types.h"

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Ditherer.
 */
typedef struct Ditherer
{
  int   random1;
  int   random2;
  float amp;
  float offset;
  float s1;
  float s2;
} Ditherer;

void
ditherer_reset (Ditherer * self, int num_bits);

/**
 * Dither given audio.
 *
 * Taken from tracktion_Ditherer.h.
 *
 * @param frames Interleaved frames.
 * @param n_frames Number of frames per channel.
 * @param channels Number of channels.
 */
void
ditherer_process (
  Ditherer * self,
  float *    frames,
  size_t     n_frames,
  channels_t channels);

/**
 * @}
 */

#endif
