/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/sample_playback.h"

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
void
sample_playback_init (
  SamplePlayback * self,
  float **         buf,
  long             buf_size,
  int              channels,
  float            vol,
  int              start_offset)
{
  self->buf = buf;
  self->buf_size = buf_size;
  self->volume = vol;
  self->offset = 0;
  self->channels = channels;
  self->start_offset = start_offset;
}
