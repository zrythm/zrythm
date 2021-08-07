/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/audio_region.h"
#include "audio/track.h"
#include "audio/track_lane.h"
#include "audio/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "midilib/src/midifile.h"
#include "midilib/src/midiinfo.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm_app.h"

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
      region->magic = REGION_MAGIC;
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
  TrackLane * self = object_new (TrackLane);

  self->name =
    g_strdup_printf (_("Lane %d"), pos + 1);
  self->pos = pos;
  self->track_pos = track->pos;

  self->regions_size = 1;
  self->regions =
    object_new_n (self->regions_size, ZRegion *);

  self->height = TRACK_DEF_HEIGHT;

  return self;
}

/**
 * Rename the lane.
 *
 * @param with_action Whether to make this an
 *   undoable action.
 */
void
track_lane_rename (
  TrackLane *  self,
  const char * new_name,
  bool         with_action)
{
  if (with_action)
    {
      GError * err = NULL;
      UndoableAction * ua =
        tracklist_selections_action_new_edit_rename_lane (
          self, new_name, &err);
      if (ua)
        {
          undo_manager_perform (UNDO_MANAGER, ua);
          EVENTS_PUSH (
            ET_TRACK_LANES_VISIBILITY_CHANGED, NULL);
        }
      else
        {
          HANDLE_ERROR (
            err, _("Failed to rename lane: %s"),
            err->message);
          return;
        }
    }
  else
    {
      char * prev_name = self->name;
      self->name = g_strdup (new_name);
      g_free (prev_name);
    }
}

/**
 * Wrapper over track_lane_rename().
 */
void
track_lane_rename_with_action (
  TrackLane *  self,
  const char * new_name)
{
  track_lane_rename (self, new_name, true);
}

const char *
track_lane_get_name (
  TrackLane * self)
{
  return self->name;
}

/**
 * Updates the frames of each position in each child
 * of the track lane recursively.
 */
void
track_lane_update_frames (
  TrackLane * self)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      ArrangerObject * r_obj =
        (ArrangerObject *) self->regions[i];

      /* project not ready yet */
      if (!PROJECT || !AUDIO_ENGINE->pre_setup)
        continue;

      g_return_if_fail (IS_REGION (r_obj));
      arranger_object_update_frames (r_obj);
    }
}

/**
 * Adds a ZRegion to the given TrackLane.
 */
void
track_lane_add_region (
  TrackLane * self,
  ZRegion *   region)
{
  track_lane_insert_region (
    self, region, self->num_regions);
}

/**
 * Inserts a ZRegion to the given TrackLane at the
 * given index.
 */
void
track_lane_insert_region (
  TrackLane * self,
  ZRegion *   region,
  int         idx)
{
  g_return_if_fail (
    self && IS_REGION (region) && idx >= 0 &&
    (region->id.type == REGION_TYPE_AUDIO ||
     region->id.type == REGION_TYPE_MIDI));

  region_set_lane (region, self);

  array_double_size_if_full (
    self->regions, self->num_regions,
    self->regions_size, ZRegion *);
  for (int i = self->num_regions; i > idx; i--)
    {
      self->regions[i] = self->regions[i - 1];
      self->regions[i]->id.idx = i;
      region_update_identifier (
        self->regions[i]);
    }
  self->num_regions++;
  self->regions[idx] = region;
  region->id.lane_pos = self->pos;
  region->id.idx = idx;
  region_update_identifier (region);

  if (region->id.type == REGION_TYPE_AUDIO)
    {
      AudioClip * clip =
        audio_region_get_clip (region);
      g_return_if_fail (clip);
    }
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
  g_message (
    "lane: %d, track pos: %d, num regions: %d",
    self->pos, pos, self->num_regions);

  self->track_pos = pos;

  for (int i = 0; i < self->num_regions; i++)
    {
      ZRegion * region = self->regions[i];
      region_set_track_pos (region, pos);
      region->id.lane_pos = self->pos;
      region_update_identifier (region);
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
  TrackLane * new_lane = object_new (TrackLane);

  new_lane->name =
    g_strdup (lane->name);
  new_lane->regions_size =
    (size_t) lane->num_regions;
  new_lane->regions =
    object_new_n (new_lane->regions_size, ZRegion *);
  new_lane->height =
    lane->height;
  new_lane->pos = lane->pos;
  new_lane->mute = lane->mute;
  new_lane->solo = lane->solo;
  new_lane->midi_ch = lane->midi_ch;

  ZRegion * region, * new_region;
  new_lane->num_regions = lane->num_regions;
  new_lane->regions =
    g_realloc (
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
          (ArrangerObject *) region);

      new_lane->regions[i] = new_region;
      region_set_lane (new_region, new_lane);

      region_gen_name (
        new_region, region->name, NULL, NULL);
    }

  return new_lane;
}

/**
 * Unselects all arranger objects.
 *
 * TODO replace with "select_all" and boolean param.
 */
void
track_lane_unselect_all (
  TrackLane * self)
{
  Track * track = track_lane_get_track (self);
  g_return_if_fail (track);
  for (int i = 0; i < self->num_regions; i++)
    {
      ZRegion * region = self->regions[i];
      arranger_object_select (
        (ArrangerObject *) region, false, false,
        F_NO_PUBLISH_EVENTS);
    }
}

/**
 * Removes all objects recursively from the track
 * lane.
 */
void
track_lane_clear (
  TrackLane * self)
{
  Track * track = track_lane_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  for (int i = self->num_regions - 1; i >= 0; i--)
    {
      ZRegion * region = self->regions[i];
      g_return_if_fail (
        IS_REGION (region) &&
        region->id.track_pos == track->pos &&
        region->id.lane_pos == self->pos);
      track_remove_region (
        track, region, 0, 1);
    }

  g_return_if_fail (self->num_regions == 0);
}

/**
 * Removes but does not free the region.
 */
void
track_lane_remove_region (
  TrackLane * self,
  ZRegion *   region)
{
  g_return_if_fail (IS_REGION (region));

  array_delete (
    self->regions, self->num_regions, region);

  for (int i = region->id.idx; i < self->num_regions;
       i++)
    {
      ZRegion * r = self->regions[i];
      r->id.idx = i;
      region_update_identifier (r);
    }
}

Tracklist *
track_lane_get_tracklist (
  TrackLane * self)
{
  if (self->is_auditioner)
    return SAMPLE_PROCESSOR->tracklist;
  else
    return TRACKLIST;
}

Track *
track_lane_get_track (
  TrackLane * self)
{
  Tracklist * tracklist =
    track_lane_get_tracklist (self);

  g_return_val_if_fail (
    self->track_pos < tracklist->num_tracks, NULL);

  Track * track =
    tracklist->tracks[self->track_pos];
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
  g_return_if_fail (track);

  /* add track name */
  midiTrackAddText (
    mf, self->track_pos, textTrackName,
    track->name);

  ZRegion * region;
  for (int i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];
      midi_region_write_to_midi_file (
        region, mf, 1, true, true);
    }
}

/**
 * Frees the TrackLane.
 */
void
track_lane_free (
  TrackLane * self)
{
  g_free_and_null (self->name);

  for (int i = 0; i < self->num_regions; i++)
    {
      arranger_object_free (
        (ArrangerObject *) self->regions[i]);
    }

  object_zero_and_free_if_nonnull (self->regions);

  object_zero_and_free (self);
}
