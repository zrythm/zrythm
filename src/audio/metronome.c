/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"
#include "audio/encoder.h"
#include "audio/metronome.h"
#include "utils/audio.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <gtk/gtk.h>

/**
 * Initializes the Metronome by loading the samples
 * into memory.
 */
Metronome *
metronome_new (void)
{
  Metronome * self = object_new (Metronome);

  if (ZRYTHM_TESTING)
    {
      const char * src_root =
        getenv ("G_TEST_SRC_ROOT_DIR");
      g_warn_if_fail (src_root);
      self->emphasis_path =
        g_build_filename (
          src_root, "data", "samples", "klick",
          "square_emphasis.wav", NULL);
      self->normal_path =
        g_build_filename (
          src_root, "data", "samples", "klick",
          "/square_normal.wav", NULL);
    }
  else
    {
      char * samplesdir =
        zrythm_get_dir (
          ZRYTHM_DIR_SYSTEM_SAMPLESDIR);
      self->emphasis_path =
        g_build_filename (
          samplesdir, "square_emphasis.wav", NULL);
      self->normal_path =
        g_build_filename (
          samplesdir, "square_normal.wav", NULL);

      /* decode */
      AudioEncoder * enc =
        audio_encoder_new_from_file (
          self->emphasis_path);
      if (!enc)
        {
          g_critical (
            "Failed to load samples for metronome "
            "from %s",
            self->emphasis_path);
          return NULL;
        }
      audio_encoder_decode (enc, 0);
      self->emphasis =
        calloc (
          (size_t)
            (enc->num_out_frames * enc->channels),
          sizeof (float));
      self->emphasis_size = enc->num_out_frames;
      self->emphasis_channels = enc->channels;
      g_return_val_if_fail (
        enc->channels > 0, NULL);
      for (int i = 0;
           i < enc->num_out_frames * enc->channels;
           i++)
        {
          self->emphasis[i] = enc->out_frames[i];
        }
      audio_encoder_free (enc);

      enc =
        audio_encoder_new_from_file (
          self->normal_path);
      audio_encoder_decode (
        enc, 0);
      self->normal =
        calloc (
          (size_t)
            (enc->num_out_frames * enc->channels),
          sizeof (float));
      self->normal_size = enc->num_out_frames;
      self->normal_channels = enc->channels;
      g_return_val_if_fail (enc->channels > 0, NULL);
      for (int i = 0;
           i < enc->num_out_frames * enc->channels;
           i++)
        {
          self->normal[i] = enc->out_frames[i];
        }
      audio_encoder_free (enc);
    }

  return self;
}

void
metronome_free (
  Metronome * self)
{
  g_free_and_null (self->emphasis_path);
  g_free_and_null (self->normal_path);
  object_zero_and_free (self->emphasis);
  object_zero_and_free (self->normal);

  object_zero_and_free (self);
}
