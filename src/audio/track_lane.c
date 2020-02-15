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

#include "audio/track.h"
#include "audio/track_lane.h"
#include "audio/tracklist.h"
#include "utils/arrays.h"
#include "midilib/src/midifile.h"
#include "midilib/src/midiinfo.h"
#include "project.h"

#include <glib/gi18n.h>

/**
 * Inits the TrackLane after a project was loaded.
 */
void
track_lane_init_loaded (
  TrackLane * lane)
{
  lane->regions_size =
    (size_t) lane->num_regions;
  int i;
  ZRegion * region;
  for (i = 0; i < lane->num_regions; i++)
    {
      region = lane->regions[i];
      ArrangerObject * r_obj =
        (ArrangerObject *) region;
      region_set_lane (region, lane);
      arranger_object_init_loaded (r_obj);
    }
}

/**
 * Creates a new TrackLane at the given pos in the
 * given Track.
 *
 * @param track The Track to create the TrackLane for.
 * @param pos The position (index) in the Track that
 *   this lane will be placed in.
 */
TrackLane *
track_lane_new (
  Track * track,
  int     pos)
{
  TrackLane * self = calloc (1, sizeof (TrackLane));

  self->name = g_strdup_printf (_("Lane %d"), pos);
  self->pos = pos;
  self->track_pos = track->pos;

  self->regions_size = 1;
  self->regions =
    malloc (self->regions_size *
            sizeof (ZRegion *));

  self->height = TRACK_DEF_HEIGHT;

  return self;
}

/**
 * Updates the frames of each position in each child
 * of the track lane recursively.
 */
void
track_lane_update_frames (
  TrackLane * self)
{
  ZRegion * r;
  for (int i = 0; i < self->num_regions; i++)
    {
      r = self->regions[i];
      ArrangerObject * r_obj =
        (ArrangerObject *) r;
      arranger_object_update_frames (r_obj);
    }
}

/**
 * Adds a ZRegion to the given TrackLane.
 */
void
track_lane_add_region (
  TrackLane * self,
  ZRegion *    region)
{
  g_return_if_fail (
    region->id.type == REGION_TYPE_AUDIO ||
    region->id.type == REGION_TYPE_MIDI);

  region_set_lane (region, self);

  array_double_size_if_full (
    self->regions, self->num_regions,
    self->regions_size, ZRegion *);
  array_append (self->regions,
                self->num_regions,
                region);
}

/**
 * Sets the track position to the lane and all its
 * members recursively.
 *
 * @param set_pointers Sets the Track pointers as
 *   well.
 */
void
track_lane_set_track_pos (
  TrackLane * self,
  const int   pos)
{
  self->track_pos = pos;

  for (int i = 0; i < self->num_regions; i++)
    {
      region_set_track_pos (
        self->regions[i], pos);
    }
}

/**
 * Clones the TrackLane.
 *
 * Mainly used when cloning Track's.
 */
TrackLane *
track_lane_clone (
  TrackLane * lane)
{
  TrackLane * new_lane =
    calloc (1, sizeof (TrackLane));

  new_lane->name =
    g_strdup (lane->name);
  new_lane->regions_size =
    (size_t) lane->num_regions;
  new_lane->regions =
    malloc (new_lane->regions_size *
            sizeof (ZRegion *));
  new_lane->height =
    lane->height;
  new_lane->pos = lane->pos;
  new_lane->mute = lane->mute;
  new_lane->solo = lane->solo;
  new_lane->midi_ch = lane->midi_ch;

  ZRegion * region, * new_region;
  new_lane->num_regions = lane->num_regions;
  new_lane->regions =
    realloc (
      new_lane->regions,
      sizeof (ZRegion *) *
        (size_t) lane->num_regions);
  for (int i = 0; i < lane->num_regions; i++)
    {
      /* clone region */
      region = lane->regions[i];
      new_region =
        (ZRegion *)
        arranger_object_clone (
          (ArrangerObject *) region,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);

      new_lane->regions[i] = new_region;
      region_set_lane (new_region, new_lane);

      region_gen_name (
        new_region, region->name, NULL, NULL);
    }

  return new_lane;
}

Track *
track_lane_get_track (
  TrackLane * self)
{
  g_return_val_if_fail (self, NULL);

  if (self->track_pos >= TRACKLIST->num_tracks)
    g_return_val_if_reached (NULL);

  Track * track = TRACKLIST->tracks[self->track_pos];
  g_return_val_if_fail (track, NULL);

  return track;
}

/**
 * Writes the lane to the given MIDI file.
 */
void
track_lane_write_to_midi_file (
  TrackLane * self,
  MIDI_FILE * mf)
{
  /* All data is written out to _tracks_ not
   * channels. We therefore
  ** set the current channel before writing
  data out. Channel assignments
  ** can change any number of times during the
  file, and affect all
  ** tracks messages until it is changed. */
  midiFileSetTracksDefaultChannel (
    mf, self->track_pos, MIDI_CHANNEL_1);

  Track * track = track_lane_get_track (self);

  /* add track name */
  midiTrackAddText (
    mf, self->track_pos, textTrackName,
    track->name);

  ZRegion * region;
  for (int i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];
      midi_region_write_to_midi_file (
        region, mf, 1, 1);
    }
}

/**
 * Frees the TrackLane.
 */
void
track_lane_free (
  TrackLane * self)
{
  if (self->widget)
    {
      if (GTK_IS_WIDGET (self->widget))
        g_object_unref (
          G_OBJECT (self->widget));
    }

  if (self->name)
    g_free (self->name);

  for (int i = 0; i < self->num_regions; i++)
    arranger_object_free (
      (ArrangerObject *) self->regions[i]);

  free (self);
}
