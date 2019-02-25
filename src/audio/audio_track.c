/*
 * audio/audio_track.c - audio track
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include <math.h>
#include <stdlib.h>

#include "audio/audio_track.h"
#include "audio/automation_tracklist.h"
#include "audio/engine.h"
#include "audio/port.h"
#include "project.h"
#include "utils/arrays.h"

AudioTrack *
audio_track_new (Channel * channel)
{
  AudioTrack * self =
    calloc (1, sizeof (AudioTrack));

  Track * track = (Track *) self;
  track->type = TRACK_TYPE_AUDIO;
  gdk_rgba_parse (&track->color, "#19664c");
  track_init (track);
  project_add_track (track);

  ChannelTrack * bt = (ChannelTrack *) self;
  bt->channel = channel;

  return self;

}

void
audio_track_setup (AudioTrack * self)
{
  ChannelTrack * bt = (ChannelTrack *) self;

  channel_track_setup (bt);
}

void
audio_track_add_region (AudioTrack *  track,
                        AudioRegion * region)
{
  array_append (track->regions,
                track->num_regions,
                region);
}

void
audio_track_remove_region (AudioTrack *  track,
                           AudioRegion * region)
{
  array_delete (track->regions,
                track->num_regions,
                region);
}

/**
 * Fills stereo in buffers with info from the current clip.
 */
void
audio_track_fill_stereo_in_buffers (
  AudioTrack *  self,
  StereoPorts * stereo_in)
{
  long cycle_start_frames =
    position_to_frames (&PLAYHEAD);
  long cycle_end_frames =
    cycle_start_frames + AUDIO_ENGINE->block_length;

  for (int i = 0; i < self->num_regions; i++)
    {
      AudioRegion * ar = self->regions[i];
      Region * r = (Region *) ar;
      long region_start_frames =
        position_to_frames (&r->start_pos);
      long region_end_frames =
        position_to_frames (&r->end_pos);
      if (region_start_frames <= cycle_end_frames &&
          region_end_frames >= cycle_start_frames)
        {
          long local_frames_start =
            cycle_start_frames - region_start_frames;
          int buff_index = 0;
          if (ar->channels == 1)
            {
              /*g_message ("1 channel");*/
              for (int i = 0;
                   i < AUDIO_ENGINE->block_length;
                   i++)
                {
                  buff_index = local_frames_start + i;
                  if (buff_index < 0 ||
                      buff_index >= ar->buff_size)
                    continue;
                  stereo_in->l->buf[i] =
                    ar->buff[buff_index];
                  stereo_in->r->buf[i] =
                    ar->buff[buff_index];
                }
            }
          else if (ar->channels == 2)
            {
              /*g_message ("2 channels");*/
              for (int i = 0;
                   i < AUDIO_ENGINE->block_length;
                   i += 2)
                {
                  buff_index = local_frames_start + i;
                  if (buff_index < 0 ||
                      buff_index + 1 >= ar->buff_size)
                    continue;
                  stereo_in->l->buf[i] =
                    ar->buff[buff_index];
                  stereo_in->r->buf[i] =
                    ar->buff[buff_index + 1];
                }
            }
        }
    }
}

/**
 * Frees the track.
 *
 * TODO
 */
void
audio_track_free (AudioTrack * track)
{

}
