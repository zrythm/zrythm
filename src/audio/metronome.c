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

#include <stdlib.h>

#include "config.h"
#include "audio/metronome.h"
#include "utils/audio.h"

#include <gtk/gtk.h>

#include "ext/audio_decoder/ad.h"

/**
 * Initializes the Metronome by loading the samples
 * into memory.
 */
void
metronome_init (
  Metronome * self)
{
  /* free if previously initialized */
  if (self->emphasis_path)
    g_free (self->emphasis_path);
  if (self->normal_path)
    g_free (self->normal_path);
  if (self->emphasis)
    free (self->emphasis);
  if (self->normal)
    free (self->normal);

  self->emphasis_path =
    g_strdup (
      METRONOME_SAMPLES_DIR "/square_emphasis.wav");
  self->normal_path =
    g_strdup (
      METRONOME_SAMPLES_DIR "/square_normal.wav");

  /* open with ad */
  struct adinfo nfo;
  SRC_DATA src_data;

  /* decode */
  audio_decode (
    &nfo, &src_data, &self->emphasis,
    &self->emphasis_size, self->emphasis_path);
  self->emphasis_channels = nfo.channels;
  audio_decode (
    &nfo, &src_data, &self->normal,
    &self->normal_size, self->normal_path);
  self->normal_channels = nfo.channels;
}

/**
 * Fills the given frame buffer with metronome audio
 * based on the current position.
 *
 * @param buf The frame buffer.
 * @param g_start_frame The global start position in
 *   frames.
 * @param nframes Number of frames to fill. These must
 *   not exceed the buffer size.
 */
void
metronome_fill_buffer (
  Metronome * self,
  float *     buf,
  const long  g_start_frame,
  const int   nframes)
{
}
