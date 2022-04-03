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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com
 *
 * Tracktion Engine is published under a dual [GPL3 (or later)](https://www.gnu.org/licenses/gpl-3.0.en.html)/[Commercial license](https://www.tracktion.com/develop/tracktion-engine).
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
