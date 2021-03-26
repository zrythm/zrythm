/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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

#include <gtk/gtk.h>

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
void
sample_playback_init (
  SamplePlayback * self,
  sample_t **      buf,
  long             buf_size,
  channels_t       channels,
  float            vol,
  nframes_t        start_offset)
{
  g_return_if_fail (channels > 0);
  self->buf = buf;
  self->buf_size = buf_size;
  self->volume = vol;
  self->offset = 0;
  self->channels = channels;
  self->start_offset = start_offset;
}
