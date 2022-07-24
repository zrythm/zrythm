// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>

#include "audio/audio_region.h"
#include "audio/midi_event.h"
#include "audio/track.h"
#include "audio/track_lane.h"
#include "audio/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/custom_button.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "midilib/src/midifile.h"
#include "midilib/src/midiinfo.h"

/**
 * Inits the TrackLane after a project was loaded.
 */
void
track_lane_init_loaded (TrackLane * self, Track * track)
{
  self->track = track;
  self->regions_size = (size_t) self->num_regions;
  int       i;
  ZRegion * region;
  for (i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];
      region->magic = REGION_MAGIC;
      ArrangerObject * r_obj = (ArrangerObject *) region;
      region_set_lane (region, self);
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
track_lane_new (Track * track, int pos)
{
  TrackLane * self = object_new (TrackLane);
  self->schema_version = TRACK_LANE_SCHEMA_VERSION;
  self->pos = pos;
  self->track = track;

  self->name = g_strdup_printf (_ ("Lane %d"), pos + 1);

  self->regions_size = 1;
  self->regions = object_new_n (self->regions_size, ZRegion *);

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
      bool     ret =
        tracklist_selections_action_perform_edit_rename_lane (
          self, new_name, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Failed to rename lane"));
        }
      EVENTS_PUSH (ET_TRACK_LANES_VISIBILITY_CHANGED, NULL);
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

/**
 * Sets track lane soloed, updates UI and optionally
 * adds the action to the undo stack.
 *
 * @param trigger_undo Create and perform an
 *   undoable action.
 * @param fire_events Fire UI events.
 */
void
track_lane_set_soloed (
  TrackLane * self,
  bool        solo,
  bool        trigger_undo,
  bool        fire_events)
{
  if (trigger_undo)
    {
      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_edit_solo_lane (
          self, solo, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track lane soloed"));
          return;
        }
    }
  else
    {
      g_debug (
        "setting lane '%s' soloed to %d", self->name, solo);
      self->solo = solo;
    }

  if (fire_events)
    {
      /* TODO use more specific event */
      EVENTS_PUSH (ET_TRACK_LANES_VISIBILITY_CHANGED, NULL);
    }
}

bool
track_lane_get_soloed (const TrackLane * const self)
{
  return self->solo;
}

/**
 * Sets track lane muted, updates UI and optionally
 * adds the action to the undo stack.
 *
 * @param trigger_undo Create and perform an
 *   undoable action.
 * @param fire_events Fire UI events.
 */
void
track_lane_set_muted (
  TrackLane * self,
  bool        mute,
  bool        trigger_undo,
  bool        fire_events)
{
  if (trigger_undo)
    {
      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_edit_mute_lane (
          self, mute, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Cannot set track lane muted"));
          return;
        }
    }
  else
    {
      g_debug (
        "setting lane '%s' muted to %d", self->name, mute);
      self->mute = mute;
    }

  if (fire_events)
    {
      /* TODO use more specific event */
      EVENTS_PUSH (ET_TRACK_LANES_VISIBILITY_CHANGED, NULL);
    }
}

bool
track_lane_get_muted (const TrackLane * const self)
{
  return self->mute;
}

const char *
track_lane_get_name (TrackLane * self)
{
  return self->name;
}

/**
 * Updates the positions in each child recursively.
 *
 * @param from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 */
void
track_lane_update_positions (
  TrackLane * self,
  bool        from_ticks,
  bool        bpm_change)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      ArrangerObject * r_obj =
        (ArrangerObject *) self->regions[i];

      /* project not ready yet */
      if (!PROJECT || !AUDIO_ENGINE->pre_setup)
        continue;

      g_return_if_fail (IS_REGION_AND_NONNULL (r_obj));
      if (ZRYTHM_TESTING)
        {
          region_validate (
            (ZRegion *) r_obj,
            track_lane_is_in_active_project (self), 0);
        }
      arranger_object_update_positions (
        r_obj, from_ticks, bpm_change, NULL);
      if (ZRYTHM_TESTING)
        {
          region_validate (
            (ZRegion *) r_obj,
            track_lane_is_in_active_project (self), 0);
        }
    }
}

/**
 * Adds a ZRegion to the given TrackLane.
 */
void
track_lane_add_region (TrackLane * self, ZRegion * region)
{
  track_lane_insert_region (self, region, self->num_regions);
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
    self && IS_REGION (region) && idx >= 0
    && (region->id.type == REGION_TYPE_AUDIO || region->id.type == REGION_TYPE_MIDI));

  region_set_lane (region, self);

  array_double_size_if_full (
    self->regions, self->num_regions, self->regions_size,
    ZRegion *);
  for (int i = self->num_regions; i > idx; i--)
    {
      self->regions[i] = self->regions[i - 1];
      self->regions[i]->id.idx = i;
      region_update_identifier (self->regions[i]);
    }
  self->num_regions++;
  self->regions[idx] = region;
  region->id.lane_pos = self->pos;
  region->id.idx = idx;
  region_update_identifier (region);

  if (region->id.type == REGION_TYPE_AUDIO)
    {
      AudioClip * clip = audio_region_get_clip (region);
      g_return_if_fail (clip);
    }
}

/**
 * Sets the new track name hash to all the lane's
 * objects recursively.
 */
void
track_lane_update_track_name_hash (TrackLane * self)
{
  Track * track = self->track;
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  for (int i = 0; i < self->num_regions; i++)
    {
      ZRegion * region = self->regions[i];
      region->id.track_name_hash = track_get_name_hash (track);
      region->id.lane_pos = self->pos;
      region_update_identifier (region);
    }
}

/**
 * Clones the TrackLane.
 *
 * @param track New owner track, if any.
 */
TrackLane *
track_lane_clone (const TrackLane * src, Track * track)
{
  TrackLane * self = object_new (TrackLane);
  self->schema_version = TRACK_LANE_SCHEMA_VERSION;
  self->track = track;

  self->name = g_strdup (src->name);
  self->regions_size = (size_t) src->num_regions;
  self->regions = object_new_n (self->regions_size, ZRegion *);
  self->height = src->height;
  self->pos = src->pos;
  self->mute = src->mute;
  self->solo = src->solo;
  self->midi_ch = src->midi_ch;

  ZRegion *region, *new_region;
  self->num_regions = src->num_regions;
  self->regions = g_realloc (
    self->regions,
    sizeof (ZRegion *) * (size_t) src->num_regions);
  for (int i = 0; i < src->num_regions; i++)
    {
      /* clone region */
      region = src->regions[i];
      new_region = (ZRegion *) arranger_object_clone (
        (ArrangerObject *) region);

      self->regions[i] = new_region;
      region_set_lane (new_region, self);

      region_gen_name (new_region, region->name, NULL, NULL);
    }

  return self;
}

/**
 * Unselects all arranger objects.
 *
 * TODO replace with "select_all" and boolean param.
 */
void
track_lane_unselect_all (TrackLane * self)
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
track_lane_clear (TrackLane * self)
{
  Track * track = track_lane_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  g_message (
    "clearing track lane %d (%p) for track '%s' | "
    "num regions %d",
    self->pos, self, track->name, self->num_regions);

  for (int i = self->num_regions - 1; i >= 0; i--)
    {
      ZRegion * region = self->regions[i];
      g_return_if_fail (
        IS_REGION (region)
        && region->id.track_name_hash
             == track_get_name_hash (track)
        && region->id.lane_pos == self->pos);
      track_remove_region (track, region, 0, 1);
    }

  g_return_if_fail (self->num_regions == 0);
}

/**
 * Removes but does not free the region.
 */
void
track_lane_remove_region (TrackLane * self, ZRegion * region)
{
  g_return_if_fail (IS_REGION (region));

  if (
    track_lane_is_in_active_project (self)
    && !track_lane_is_auditioner (self))
    {
      /* if clip editor region index is greater
       * than this index, decrement it */
      ZRegion * clip_editor_r =
        clip_editor_get_region (CLIP_EDITOR);
      if (
        clip_editor_r
        && clip_editor_r->id.track_name_hash
             == region->id.track_name_hash
        && clip_editor_r->id.lane_pos == region->id.lane_pos
        && clip_editor_r->id.idx > region->id.idx)
        {
          CLIP_EDITOR->region_id.idx--;
        }
    }

  bool deleted = false;
  array_delete_confirm (
    self->regions, self->num_regions, region, deleted);
  g_return_if_fail (deleted);

  for (int i = region->id.idx; i < self->num_regions; i++)
    {
      ZRegion * r = self->regions[i];
      r->id.idx = i;
      region_update_identifier (r);
    }
}

Tracklist *
track_lane_get_tracklist (const TrackLane * self)
{
  if (track_lane_is_auditioner (self))
    return SAMPLE_PROCESSOR->tracklist;
  else
    return TRACKLIST;
}

Track *
track_lane_get_track (const TrackLane * self)
{
  g_return_val_if_fail (self->track, NULL);
  return self->track;
}

/**
 * Calculates a unique index for this lane.
 */
int
track_lane_calculate_lane_idx (const TrackLane * self)
{
  Track *     track = track_lane_get_track (self);
  Tracklist * tracklist = track_lane_get_tracklist (self);
  int         pos = 1;
  for (int i = 0; i < tracklist->num_tracks; i++)
    {
      Track * cur_track = tracklist->tracks[i];
      if (cur_track == track)
        {
          pos += self->pos;
          break;
        }
      else
        {
          pos += cur_track->num_lanes;
        }
    }

  return pos;
}

/**
 * Writes the lane to the given MIDI file.
 *
 * @param lanes_as_tracks Export lanes as separate
 *   MIDI tracks.
 * @param use_track_or_lane_pos Whether to use the
 *   track position (or lane position if @ref
 *   lanes_as_tracks is true) in the MIDI data.
 *   The MIDI track will be set to 1 if false.
 * @param events Track events, if not using lanes
 *   as tracks.
 */
void
track_lane_write_to_midi_file (
  TrackLane *  self,
  MIDI_FILE *  mf,
  MidiEvents * events,
  bool         lanes_as_tracks,
  bool         use_track_or_lane_pos)
{
  Track * track = track_lane_get_track (self);
  g_return_if_fail (track);
  int  midi_track_pos = track->pos;
  bool own_events = false;
  if (lanes_as_tracks)
    {
      g_return_if_fail (!events);
      midi_track_pos = track_lane_calculate_lane_idx (self);
      events = midi_events_new ();
      own_events = true;
    }
  else if (!use_track_or_lane_pos)
    {
      g_return_if_fail (events);
      midi_track_pos = 1;
    }
  /* else if using track positions */
  else
    {
      g_return_if_fail (events);
    }

  /* All data is written out to tracks not
   * channels. We therefore
   * set the current channel before writing
   * data out. Channel assignments
   * can change any number of times during the
   * file, and affect all
   * tracks messages until it is changed. */
  midiFileSetTracksDefaultChannel (
    mf, midi_track_pos, MIDI_CHANNEL_1);

  /* add track name */
  if (lanes_as_tracks && use_track_or_lane_pos)
    {
      char midi_track_name[1000];
      sprintf (
        midi_track_name, "%s - %s", track->name, self->name);
      midiTrackAddText (
        mf, midi_track_pos, textTrackName, midi_track_name);
    }

  for (int i = 0; i < self->num_regions; i++)
    {
      const ZRegion * region = self->regions[i];
      midi_region_add_events (region, events, true, true);
    }

  if (own_events)
    {
      midi_events_write_to_midi_file (
        events, mf, midi_track_pos);
      object_free_w_func_and_null (midi_events_free, events);
    }
}

/**
 * Frees the TrackLane.
 */
void
track_lane_free (TrackLane * self)
{
  g_free_and_null (self->name);

  for (int i = 0; i < self->num_regions; i++)
    {
      arranger_object_free (
        (ArrangerObject *) self->regions[i]);
    }

  object_zero_and_free_if_nonnull (self->regions);

  /* FIXME this is bad design - this object should
   * not care about widgets */
  for (int j = 0; j < self->num_buttons; j++)
    {
      if (self->buttons[j])
        custom_button_widget_free (self->buttons[j]);
    }

  object_zero_and_free (self);
}
