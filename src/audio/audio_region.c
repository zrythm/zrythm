/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel.h"
#include "audio/audio_region.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/region.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/io.h"

#include "ext/audio_decoder/ad.h"

/**
 * Creates region (used when loading projects).
 */
AudioRegion *
audio_region_get_or_create_blank (int id)
{
  if (PROJECT->regions[id])
    {
      return (AudioRegion *) PROJECT->regions[id];
    }
  else
    {
      AudioRegion * audio_region = calloc (1, sizeof (AudioRegion));
      Region * region = (Region *) audio_region;

      region->id = id;
      PROJECT->regions[id] = region;
      PROJECT->num_regions++;

      g_message ("Creating blank audio region %d", id);

      return audio_region;
    }
}

AudioRegion *
audio_region_new (Track *    track,
                  char *     filename,
                  Position * start_pos)
{
  AudioRegion * self =
    calloc (1, sizeof (AudioRegion));

  /* open with ad */
  struct adinfo nfo;
  SRC_DATA src_data;
  float * out_buff;
  long out_buff_size;

  /* decode */
  audio_decode (
    &nfo, &src_data, &out_buff, &out_buff_size,
    filename);

  /* set end pos to sample end */
  position_set_to_pos (&self->end_pos,
                       &self->start_pos);
  position_add_frames (&self->end_pos,
                       src_data.output_frames_gen /
                       nfo.channels);

  /* init */
  region_init ((Region *) self,
               REGION_TYPE_AUDIO,
               track,
               start_pos,
               &self->end_pos);

  /* generate a copy of the given filename in the
   * project dir */
  g_warn_if_fail (
    io_file_exists (PROJECT->audio_dir));
  GFile * file =
    g_file_new_for_path (filename);
  char * basename =
    g_file_get_basename (file);
  char * new_path =
    g_build_filename (
      PROJECT->audio_dir,
      basename);
  char * tmp;
  int i = 0;
  while (io_file_exists (new_path))
    {
      g_free (new_path);
      tmp =
        g_strdup_printf (
          "%s(%d)",
          basename, i++);
      new_path =
        g_build_filename (
          PROJECT->audio_dir,
          tmp);
      g_free (tmp);
    }
  audio_write_raw_file (
    out_buff, src_data.output_frames_gen,
    AUDIO_ENGINE->sample_rate,
    nfo.channels, new_path);
  g_free (basename);
  g_free (file);

  /*self->buff = buff;*/
  /*self->buff_size = buff_size;*/
  /*self->channels = channels;*/
  /*self->filename = strdup (filename);*/

  return self;
}

/**
 * Frees members only but not the audio region itself.
 *
 * Regions should be free'd using region_free.
 */
void
audio_region_free_members (AudioRegion * self)
{
  free (self->buff);
  g_free (self->filename);
}
