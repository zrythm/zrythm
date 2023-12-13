// clang-format off
// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
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
// clang-format on

#include <math.h>
#include <stdlib.h>

#include "dsp/ditherer.h"

void
ditherer_reset (Ditherer * self, int num_bits)
{
  self->random1 = 0;
  self->random2 = 0;
  self->amp = 0;
  self->offset = 0;
  self->s1 = 0;
  self->s2 = 0;

  float wordLen = powf (2.0f, (float) (num_bits - 1));
  float invWordLen = 1.0f / wordLen;
  self->amp = invWordLen / (float) RAND_MAX;
  self->offset = invWordLen * 0.5f;
}

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
  channels_t channels)
{
  for (channels_t i = 0; i < channels; i++)
    {
      for (size_t j = 0; j < n_frames; j++)
        {
          self->random2 = self->random1;
          self->random1 = rand ();

          float * in_ptr = &frames[channels * j + i];
          float   in = *in_ptr;

          // check for dodgy numbers coming in..
          if (in < -0.000001f || in > 0.000001f)
            {
              in += 0.5f * (self->s1 + self->s1 - self->s2);
              float out =
                in + self->offset
                + (self->amp * (float) (self->random1 - self->random2));
              *in_ptr = out;
              self->s2 = self->s1;
              self->s1 = in - out;
            }
          else
            {
              *in_ptr = in;
              in += 0.5f * (self->s1 + self->s1 - self->s2);
              float out =
                in + self->offset
                + (self->amp * (float) (self->random1 - self->random2));
              self->s2 = self->s1;
              self->s1 = in - out;
            }
        }
    }
}
