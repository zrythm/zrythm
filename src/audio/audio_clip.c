/*
 * audio/audio_clip.c - Audio clip for audio regions
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "audio/audio_clip.h"

#include <gtk/gtk.h>

AudioClip *
audio_clip_new (AudioRegion * region,
                float *       buff,
                long          buff_size,
                int           channels,
                char *        filename)
{
  AudioClip * self = calloc (1, sizeof (AudioClip));

  self->audio_region = region;
  self->buff = buff;
  self->buff_size = buff_size;
  self->channels = channels;
  self->filename = strdup (filename);
  self->end_frames = buff_size - 1;

  return self;
}
