// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/channel.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/clip.h"
#include "dsp/control_port.h"
#include "dsp/instrument_track.h"
#include "dsp/midi_note.h"
#include "dsp/midi_region.h"
#include "dsp/pool.h"
#include "dsp/recording_manager.h"
#include "dsp/region.h"
#include "dsp/region_link_group_manager.h"
#include "dsp/router.h"
#include "dsp/stretcher.h"
#include "dsp/track.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Only to be used by implementing structs.
 */
void
region_init (
  ZRegion *        self,
  const Position * start_pos,
  const Position * end_pos,
  unsigned int     track_name_hash,
  int              lane_pos_or_at_idx,
  int              idx_inside_lane_or_at)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  obj->type = ARRANGER_OBJECT_TYPE_REGION;
  arranger_object_init (obj);

  self->id.track_name_hash = track_name_hash;
  self->id.lane_pos = lane_pos_or_at_idx;
  self->id.at_idx = lane_pos_or_at_idx;
  self->id.idx = idx_inside_lane_or_at;
  self->id.link_group = -1;

  position_init (&obj->pos);
  position_set_to_pos (&obj->pos, start_pos);
  obj->pos.frames = start_pos->frames;
  position_init (&obj->end_pos);
  position_set_to_pos (&obj->end_pos, end_pos);
  obj->end_pos.frames = end_pos->frames;
  position_init (&obj->clip_start_pos);
  long length = arranger_object_get_length_in_frames (obj);
  g_warn_if_fail (length > 0);
  position_init (&obj->loop_start_pos);
  position_init (&obj->loop_end_pos);
  position_from_frames (&obj->loop_end_pos, length);
  obj->loop_end_pos.frames = length;

  /* set fade positions to start/end */
  position_init (&obj->fade_in_pos);
  position_init (&obj->fade_out_pos);
  position_from_frames (&obj->fade_out_pos, length);

  self->magic = REGION_MAGIC;

  region_validate (self, false, 0);
}

/**
 * Generates a name for the ZRegion, either using
 * the given AutomationTrack or Track, or appending
 * to the given base name.
 */
void
region_gen_name (
  ZRegion *         self,
  const char *      base_name,
  AutomationTrack * at,
  Track *           track)
{
  g_return_if_fail (IS_REGION (self));

  /* Name to try to assign */
  char * orig_name = NULL;
  if (base_name)
    orig_name = g_strdup (base_name);
  else if (at)
    orig_name = g_strdup_printf ("%s - %s", track->name, at->port_id.label);
  else
    orig_name = g_strdup (track->name);

  arranger_object_set_name (
    (ArrangerObject *) self, orig_name, F_NO_PUBLISH_EVENTS);
  g_free (orig_name);
}

/**
 * Sets the track lane.
 */
void
region_set_lane (ZRegion * self, const TrackLane * const lane)
{
  g_return_if_fail (IS_REGION (self));
  g_return_if_fail (IS_TRACK_AND_NONNULL (lane->track));

  if (track_lane_is_auditioner (lane))
    self->base.is_auditioner = true;

  self->id.lane_pos = lane->pos;
  self->id.track_name_hash = track_get_name_hash (lane->track);
}

/**
 * Moves the ZRegion to the given Track, maintaining the
 * selection status of the ZRegion.
 *
 * Assumes that the ZRegion is already in a TrackLane or
 * AutomationTrack.
 *
 * @param lane_or_at_index If MIDI or audio, lane position.
 *   If automation, automation track index in the automation
 *   tracklist. If -1, the track lane or automation track
 *   index will be inferred from the region.
 * @param index If MIDI or audio, index in lane in the new
 *   track to insert the region to, or -1 to append. If
 *   automation, index in the automation track.
 */
void
region_move_to_track (
  ZRegion * region,
  Track *   track,
  int       lane_or_at_index,
  int       index)
{
  g_return_if_fail (IS_REGION (region) && track);

  Track * region_track = arranger_object_get_track ((ArrangerObject *) region);
  /*bool same_track = region_track == track;*/

  g_message ("moving region %s to track %s", region->name, track->name);
  size_t sz = 2000;
  char   buf[sz];
  region_print_to_str (region, buf, sz);
  g_debug ("before: %s", buf);

  RegionLinkGroup * link_group = NULL;
  if (region_has_link_group (region))
    {
      link_group = region_get_link_group (region);
      g_return_if_fail (link_group);
      region_link_group_remove_region (link_group, region, false, true);
    }

  bool      selected = region_is_selected (region);
  ZRegion * clip_editor_region = clip_editor_get_region (CLIP_EDITOR);

  if (region_track)
    {
      /* remove the region from its old track */
      track_remove_region (region_track, region, F_NO_PUBLISH_EVENTS, F_NO_FREE);
    }

  int lane_pos = lane_or_at_index >= 0 ? lane_or_at_index : region->id.lane_pos;
  int at_pos = lane_or_at_index >= 0 ? lane_or_at_index : region->id.at_idx;

  if (region->id.type == REGION_TYPE_AUTOMATION)
    {
      AutomationTrack * at = track->automation_tracklist.ats[at_pos];

      /* convert the automation points to match the new
       * automatable */
      Port * port = port_find_from_identifier (&at->port_id);
      g_return_if_fail (IS_PORT_AND_NONNULL (port));
      for (int i = 0; i < region->num_aps; i++)
        {
          AutomationPoint * ap = region->aps[i];
          ap->fvalue =
            control_port_normalized_val_to_real (port, ap->normalized_val);
        }

      /* add the region to its new track */
      bool     success;
      GError * err = NULL;
      if (index >= 0)
        {
          success = track_insert_region (
            track, region, at, -1, index, F_NO_GEN_NAME, F_NO_PUBLISH_EVENTS,
            &err);
        }
      else
        {
          success = track_add_region (
            track, region, at, -1, F_NO_GEN_NAME, F_NO_PUBLISH_EVENTS, &err);
        }
      if (!success)
        {
          HANDLE_ERROR_LITERAL (err, "Failed to add region to track");
          return;
        }

      g_warn_if_fail (region->id.at_idx == at->index);
      region_set_automation_track (region, at);
    }
  else
    {
      /* create lanes if they don't exist */
      track_create_missing_lanes (track, lane_pos);

      /* add the region to its new track */
      bool     success;
      GError * err = NULL;
      if (index >= 0)
        {
          success = track_insert_region (
            track, region, NULL, lane_pos, index, F_NO_GEN_NAME,
            F_NO_PUBLISH_EVENTS, &err);
        }
      else
        {
          success = track_add_region (
            track, region, NULL, lane_pos, F_NO_GEN_NAME, F_NO_PUBLISH_EVENTS,
            &err);
        }
      if (!success)
        {
          HANDLE_ERROR_LITERAL (err, "Failed to add region to track");
          return;
        }

      g_warn_if_fail (region->id.lane_pos == lane_pos);
      g_warn_if_fail (
        track->lanes[lane_pos]->num_regions > 0
        && track->lanes[lane_pos]->regions[region->id.idx] == region);
      region_set_lane (region, track->lanes[lane_pos]);

      track_create_missing_lanes (track, lane_pos);

      if (region_track)
        {
          /* remove empty lanes if the region was the last on
           * its track lane */
          track_remove_empty_last_lanes (region_track);
        }

      if (link_group)
        {
          region_link_group_add_region (link_group, region);
        }
    }

  /* reset the clip editor region because
   * track_remove_region clears it */
  if (region == clip_editor_region)
    {
      clip_editor_set_region (CLIP_EDITOR, region, true);
    }

  /* reselect if necessary */
  arranger_object_select (
    (ArrangerObject *) region, selected, F_APPEND, F_NO_PUBLISH_EVENTS);

  region_print_to_str (region, buf, sz);
  g_debug ("after: %s", buf);

  if (ZRYTHM_TESTING)
    {
      region_link_group_manager_validate (REGION_LINK_GROUP_MANAGER);
    }
}

/**
 * Stretch the region's contents.
 *
 * This should be called right after changing the
 * region's size.
 *
 * @param ratio The ratio to stretch by.
 *
 * @return Whether successful.
 */
bool
region_stretch (ZRegion * self, double ratio, GError ** error)
{
  g_return_val_if_fail (IS_REGION (self), false);

  g_debug ("stretching region %p (ratio %f)", self, ratio);

  self->stretching = true;
  ArrangerObject * obj = (ArrangerObject *) self;

  switch (self->id.type)
    {
    case REGION_TYPE_MIDI:
      for (int i = 0; i < self->num_midi_notes; i++)
        {
          MidiNote *       mn = self->midi_notes[i];
          ArrangerObject * mn_obj = (ArrangerObject *) mn;

          /* set start pos */
          double   before_ticks = mn_obj->pos.ticks;
          double   new_ticks = before_ticks * ratio;
          Position tmp;
          position_from_ticks (&tmp, new_ticks);
          arranger_object_pos_setter (mn_obj, &tmp);

          /* set end pos */
          before_ticks = mn_obj->end_pos.ticks;
          new_ticks = before_ticks * ratio;
          position_from_ticks (&tmp, new_ticks);
          arranger_object_end_pos_setter (mn_obj, &tmp);
        }
      break;
    case REGION_TYPE_AUTOMATION:
      for (int i = 0; i < self->num_aps; i++)
        {
          AutomationPoint * ap = self->aps[i];
          ArrangerObject *  ap_obj = (ArrangerObject *) ap;

          /* set pos */
          double   before_ticks = ap_obj->pos.ticks;
          double   new_ticks = before_ticks * ratio;
          Position tmp;
          position_from_ticks (&tmp, new_ticks);
          arranger_object_pos_setter (ap_obj, &tmp);
        }
      break;
    case REGION_TYPE_CHORD:
      for (int i = 0; i < self->num_chord_objects; i++)
        {
          ChordObject *    co = self->chord_objects[i];
          ArrangerObject * co_obj = (ArrangerObject *) co;

          /* set pos */
          double   before_ticks = co_obj->pos.ticks;
          double   new_ticks = before_ticks * ratio;
          Position tmp;
          position_from_ticks (&tmp, new_ticks);
          arranger_object_pos_setter (co_obj, &tmp);
        }
      break;
    case REGION_TYPE_AUDIO:
      {
        AudioClip * clip = audio_region_get_clip (self);
        GError *    err = NULL;
        int         new_clip_id = audio_pool_duplicate_clip (
          AUDIO_POOL, clip->pool_id, F_NO_WRITE_FILE, &err);
        if (new_clip_id < 0)
          {
            PROPAGATE_PREFIXED_ERROR_LITERAL (
              error, err, "Failed to duplicate clip");
            return false;
          }
        AudioClip * new_clip = audio_pool_get_clip (AUDIO_POOL, new_clip_id);
        audio_region_set_clip_id (self, new_clip->pool_id);
        Stretcher * stretcher = stretcher_new_rubberband (
          AUDIO_ENGINE->sample_rate, new_clip->channels, ratio, 1.0, false);
        ssize_t returned_frames = stretcher_stretch_interleaved (
          stretcher, new_clip->frames, (size_t) new_clip->num_frames,
          &new_clip->frames);
        z_return_val_if_fail_cmp (returned_frames, >, 0, false);
        new_clip->num_frames = (unsigned_frame_t) returned_frames;
        bool success =
          audio_clip_write_to_pool (new_clip, F_NO_PARTS, F_NOT_BACKUP, &err);
        if (!success)
          {
            PROPAGATE_PREFIXED_ERROR_LITERAL (
              error, err, "Failed to write clip to pool");
            return false;
          }
        (void) obj;
        /* readjust end position to match the
         * number of frames exactly */
        Position new_end_pos;
        position_from_frames (&new_end_pos, returned_frames);
        arranger_object_set_position (
          obj, &new_end_pos, ARRANGER_OBJECT_POSITION_TYPE_LOOP_END,
          F_NO_VALIDATE);
        position_add_frames (&new_end_pos, obj->pos.frames);
        arranger_object_set_position (
          obj, &new_end_pos, ARRANGER_OBJECT_POSITION_TYPE_END, F_NO_VALIDATE);
        stretcher_free (stretcher);
      }
      break;
    default:
      g_critical ("unimplemented");
      break;
    }

  obj->use_cache = false;
  self->stretching = false;

  return true;
}

/**
 * Sets the automation track.
 */
void
region_set_automation_track (ZRegion * self, AutomationTrack * at)
{
  g_return_if_fail (IS_REGION (self) && at);

  g_debug (
    "setting region automation track to %d %s", at->index, at->port_id.label);

  /* if clip editor region or region selected,
   * unselect it */
  if (region_identifier_is_equal (&self->id, &CLIP_EDITOR->region_id))
    {
      clip_editor_set_region (CLIP_EDITOR, NULL, true);
    }
  bool was_selected = false;
  if (region_is_selected (self))
    {
      was_selected = true;
      arranger_object_select (
        (ArrangerObject *) self, F_NO_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
    }
  self->id.at_idx = at->index;
  Track * track = automation_track_get_track (at);
  self->id.track_name_hash = track_get_name_hash (track);

  region_update_identifier (self);

  /* reselect it if was selected */
  if (was_selected)
    {
      arranger_object_select (
        (ArrangerObject *) self, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
    }
}

void
region_get_type_as_string (RegionType type, char * buf)
{
  g_return_if_fail (type >= 0 && type <= REGION_TYPE_CHORD);
  switch (type)
    {
    case REGION_TYPE_MIDI:
      strcpy (buf, _ ("MIDI"));
      break;
    case REGION_TYPE_AUDIO:
      strcpy (buf, _ ("Audio"));
      break;
    case REGION_TYPE_AUTOMATION:
      strcpy (buf, _ ("Automation"));
      break;
    case REGION_TYPE_CHORD:
      strcpy (buf, _ ("Chord"));
      break;
    }
}

/**
 * Sanity checking.
 *
 * @param frames_per_tick Frames per tick used when
 * validating audio regions. Passing 0 will use the value
 * from the current engine.
 */
bool
region_validate (ZRegion * self, bool is_project, double frames_per_tick)
{
  g_return_val_if_fail (IS_REGION (self), false);

  if (!region_identifier_validate (&self->id))
    {
      return false;
    }

  if (is_project)
    {
      ZRegion * found = region_find (&self->id);
      if (found != self)
        {
          return false;
        }
    }

  ArrangerObject * r_obj = (ArrangerObject *) self;
  g_return_val_if_fail (
    position_is_before (&r_obj->loop_start_pos, &r_obj->loop_end_pos), false);

  switch (self->id.type)
    {
    case REGION_TYPE_CHORD:
      chord_region_validate (self);
      break;
    case REGION_TYPE_AUTOMATION:
      automation_region_validate (self);
      break;
    case REGION_TYPE_AUDIO:
      audio_region_validate (self, frames_per_tick);
      break;
    default:
      break;
    }

  return true;
}

TrackLane *
region_get_lane (const ZRegion * self)
{
  g_return_val_if_fail (IS_REGION (self), NULL);
  g_return_val_if_fail (
    self->id.type == REGION_TYPE_MIDI || self->id.type == REGION_TYPE_AUDIO,
    NULL);

  Track * track = arranger_object_get_track ((ArrangerObject *) self);
  g_return_val_if_fail (IS_TRACK (track), NULL);
  if (self->id.lane_pos < track->num_lanes)
    {
      TrackLane * lane = track->lanes[self->id.lane_pos];
      g_return_val_if_fail (lane, NULL);
      return lane;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Returns the region's link group.
 */
RegionLinkGroup *
region_get_link_group (ZRegion * self)
{
  g_return_val_if_fail (
    self && self->id.link_group >= 0
      && REGION_LINK_GROUP_MANAGER->num_groups > self->id.link_group,
    NULL);
  RegionLinkGroup * group = region_link_group_manager_get_group (
    REGION_LINK_GROUP_MANAGER, self->id.link_group);
  return group;
}

/**
 * Sets the link group to the region.
 *
 * @param group_idx If -1, the region will be
 *   removed from its current link group, if any.
 */
void
region_set_link_group (ZRegion * region, int group_idx, bool update_identifier)
{
  ArrangerObject * obj = (ArrangerObject *) region;
  if (obj->flags & ARRANGER_OBJECT_FLAG_NON_PROJECT)
    {
      region->id.link_group = group_idx;
      return;
    }

  if (region->id.link_group >= 0 && region->id.link_group != group_idx)
    {
      RegionLinkGroup * link_group = region_get_link_group (region);
      g_return_if_fail (link_group);
      region_link_group_remove_region (
        link_group, region, true, update_identifier);
    }
  if (group_idx >= 0)
    {
      RegionLinkGroup * group = region_link_group_manager_get_group (
        REGION_LINK_GROUP_MANAGER, group_idx);
      g_return_if_fail (group);
      region_link_group_add_region (group, region);
    }

  g_return_if_fail (group_idx == region->id.link_group);

  if (update_identifier)
    region_update_identifier (region);
}

void
region_create_link_group_if_none (ZRegion * region)
{
  ArrangerObject * obj = (ArrangerObject *) region;
  if (obj->flags & ARRANGER_OBJECT_FLAG_NON_PROJECT)
    return;

  if (region->id.link_group < 0)
    {
      size_t sz = 2000;
      char   buf[sz];
      region_print_to_str (region, buf, sz);
      g_debug ("creating link group for region: %s", buf);
      int new_group =
        region_link_group_manager_add_group (REGION_LINK_GROUP_MANAGER);
      region_set_link_group (region, new_group, true);

      region_print_to_str (region, buf, sz);
      g_debug ("after link group (%d): %s", new_group, buf);
    }
}

bool
region_has_link_group (ZRegion * region)
{
  g_return_val_if_fail (IS_REGION (region), false);
  return region->id.link_group >= 0;
}

/**
 * Removes the link group from the region, if any.
 */
void
region_unlink (ZRegion * region)
{
  ArrangerObject * obj = (ArrangerObject *) region;
  if (obj->flags & ARRANGER_OBJECT_FLAG_NON_PROJECT)
    {
      region->id.link_group = -1;
    }
  else if (region->id.link_group >= 0)
    {
      RegionLinkGroup * group = region_link_group_manager_get_group (
        REGION_LINK_GROUP_MANAGER, region->id.link_group);
      region_link_group_remove_region (group, region, true, true);
    }
  else
    {
      g_warn_if_reached ();
    }

  g_warn_if_fail (region->id.link_group == -1);

  region_update_identifier (region);
}

/**
 * Looks for the ZRegion matching the identifier.
 */
ZRegion *
region_find (const RegionIdentifier * const id)
{
  Track *           track = NULL;
  TrackLane *       lane = NULL;
  AutomationTrack * at = NULL;
  if (id->type == REGION_TYPE_MIDI || id->type == REGION_TYPE_AUDIO)
    {
      track = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash);
      g_return_val_if_fail (track, NULL);

      if (id->lane_pos >= track->num_lanes)
        {
          g_critical (
            "%s: given lane pos %d is greater than "
            "track '%s' (%d) number of lanes %d",
            __func__, id->lane_pos, track->name, track->pos, track->num_lanes);
          return NULL;
        }
      lane = track->lanes[id->lane_pos];
      g_return_val_if_fail (lane, NULL);

      z_return_val_if_fail_cmp (id->idx, >=, 0, NULL);
      z_return_val_if_fail_cmp (id->idx, <, lane->num_regions, NULL);

      ZRegion * region = lane->regions[id->idx];
      g_return_val_if_fail (IS_REGION (region), NULL);

      return region;
    }
  else if (id->type == REGION_TYPE_AUTOMATION)
    {
      track = tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash);
      g_return_val_if_fail (track, NULL);

      AutomationTracklist * atl = &track->automation_tracklist;
      g_return_val_if_fail (id->at_idx < atl->num_ats, NULL);
      at = atl->ats[id->at_idx];
      g_return_val_if_fail (at, NULL);

      if (id->idx >= at->num_regions)
        {
          automation_tracklist_print_regions (atl);
          g_critical (
            "Automation track for %s has less regions (%d) "
            "than the given index %d",
            at->port_id.label, at->num_regions, id->idx);
          return NULL;
        }
      ZRegion * region = at->regions[id->idx];
      g_return_val_if_fail (IS_REGION (region), NULL);

      return region;
    }
  else if (id->type == REGION_TYPE_CHORD)
    {
      track = P_CHORD_TRACK;
      g_return_val_if_fail (track, NULL);

      if (id->idx >= track->num_chord_regions)
        g_return_val_if_reached (NULL);
      ZRegion * region = track->chord_regions[id->idx];
      g_return_val_if_fail (IS_REGION (region), NULL);

      return region;
    }

  g_return_val_if_reached (NULL);
}

/**
 * To be called every time the identifier changes
 * to update the region's children.
 */
void
region_update_identifier (ZRegion * self)
{
  g_return_if_fail (IS_REGION (self));

  /* reset link group */
  region_set_link_group (self, self->id.link_group, false);

  switch (self->id.type)
    {
    case REGION_TYPE_AUDIO:
      break;
    case REGION_TYPE_MIDI:
      for (int i = 0; i < self->num_midi_notes; i++)
        {
          MidiNote * mn = self->midi_notes[i];
          midi_note_set_region_and_index (mn, self, i);
        }
      break;
    case REGION_TYPE_AUTOMATION:
      for (int i = 0; i < self->num_aps; i++)
        {
          AutomationPoint * ap = self->aps[i];
          automation_point_set_region_and_index (ap, self, i);
        }
      break;
    case REGION_TYPE_CHORD:
      for (int i = 0; i < self->num_chord_objects; i++)
        {
          ChordObject * co = self->chord_objects[i];
          chord_object_set_region_and_index (co, self, i);
        }
      break;
    default:
      break;
    }
}

/**
 * Updates all other regions in the region link
 * group, if any.
 */
void
region_update_link_group (ZRegion * self)
{
  g_message ("updating link group %d", self->id.link_group);
  if (self->id.link_group >= 0)
    {
      RegionLinkGroup * group = region_link_group_manager_get_group (
        REGION_LINK_GROUP_MANAGER, self->id.link_group);
      region_link_group_update (group, self);
    }
}

/**
 * Removes all children objects from the region.
 */
void
region_remove_all_children (ZRegion * region)
{
  g_message ("removing all children from %d %s", region->id.idx, region->name);
  switch (region->id.type)
    {
    case REGION_TYPE_MIDI:
      {
        g_message ("%d midi notes", region->num_midi_notes);
        for (int i = region->num_midi_notes - 1; i >= 0; i--)
          {
            MidiNote * mn = region->midi_notes[i];
            midi_region_remove_midi_note (
              region, mn, F_FREE, F_NO_PUBLISH_EVENTS);
          }
        g_warn_if_fail (region->num_midi_notes == 0);
      }
      break;
    case REGION_TYPE_AUDIO:
      break;
    case REGION_TYPE_AUTOMATION:
      {
        for (int i = region->num_aps - 1; i >= 0; i--)
          {
            AutomationPoint * ap = region->aps[i];
            automation_region_remove_ap (region, ap, false, F_FREE);
          }
      }
      break;
    case REGION_TYPE_CHORD:
      {
        for (int i = region->num_chord_objects - 1; i >= 0; i--)
          {
            ChordObject * co = region->chord_objects[i];
            chord_region_remove_chord_object (
              region, co, F_FREE, F_NO_PUBLISH_EVENTS);
          }
      }
      break;
    }
}

/**
 * Clones and copies all children from \ref src to
 * \ref dest.
 */
void
region_copy_children (ZRegion * dest, ZRegion * src)
{
  g_return_if_fail (dest->id.type == src->id.type);

  g_message (
    "copying children from %d %s to %d %s", src->id.idx, src->name,
    dest->id.idx, dest->name);

  switch (src->id.type)
    {
    case REGION_TYPE_MIDI:
      {
        g_warn_if_fail (dest->num_midi_notes == 0);
        g_message ("%d midi notes", src->num_midi_notes);
        for (int i = 0; i < src->num_midi_notes; i++)
          {
            MidiNote *       orig_mn = src->midi_notes[i];
            ArrangerObject * orig_mn_obj = (ArrangerObject *) orig_mn;

            MidiNote * mn = (MidiNote *) arranger_object_clone (orig_mn_obj);

            midi_region_add_midi_note (dest, mn, F_NO_PUBLISH_EVENTS);
          }
      }
      break;
    case REGION_TYPE_AUDIO:
      break;
    case REGION_TYPE_AUTOMATION:
      {
        /* add automation points */
        AutomationPoint *src_ap, *dest_ap;
        for (int j = 0; j < src->num_aps; j++)
          {
            src_ap = src->aps[j];
            ArrangerObject * src_ap_obj = (ArrangerObject *) src_ap;

            dest_ap = automation_point_new_float (
              src_ap->fvalue, src_ap->normalized_val, &src_ap_obj->pos);
            automation_region_add_ap (dest, dest_ap, F_NO_PUBLISH_EVENTS);
          }
      }
      break;
    case REGION_TYPE_CHORD:
      {
        ChordObject *src_co, *dest_co;
        for (int i = 0; i < src->num_chord_objects; i++)
          {
            src_co = src->chord_objects[i];

            dest_co =
              (ChordObject *) arranger_object_clone ((ArrangerObject *) src_co);

            chord_region_add_chord_object (dest, dest_co, F_NO_PUBLISH_EVENTS);
          }
      }
      break;
    }
}

bool
region_is_looped (const ZRegion * const self)
{
  const ArrangerObject * const obj = (const ArrangerObject * const) self;
  return
    obj->loop_start_pos.ticks > 0
    || obj->clip_start_pos.ticks > 0
    ||
    (obj->end_pos.ticks - obj->pos.ticks) >
       (obj->loop_end_pos.ticks +
         /* add some buffer because these are not
          * accurate */
          0.1)
    ||
    (obj->end_pos.ticks - obj->pos.ticks) <
       (obj->loop_end_pos.ticks -
         /* add some buffer because these are not
          * accurate */
          0.1);
}

/**
 * Returns the MidiNote matching the properties of
 * the given MidiNote.
 *
 * Used to find the actual MidiNote in the region
 * from a cloned MidiNote (e.g. when doing/undoing).
 */
MidiNote *
region_find_midi_note (ZRegion * r, MidiNote * clone)
{
  MidiNote * mn;
  for (int i = 0; i < r->num_midi_notes; i++)
    {
      mn = r->midi_notes[i];

      if (midi_note_is_equal (clone, mn))
        return mn;
    }

  return NULL;
}

/**
 * Gets the AutomationTrack using the saved index.
 */
AutomationTrack *
region_get_automation_track (const ZRegion * const region)
{
  Track * track =
    arranger_object_get_track ((const ArrangerObject * const) region);
  g_return_val_if_fail (
    IS_TRACK (track) && track->automation_tracklist.num_ats > region->id.at_idx,
    NULL);

  return track->automation_tracklist.ats[region->id.at_idx];
}

void
region_print_to_str (const ZRegion * self, char * buf, const size_t buf_size)
{
  char from_pos_str[100], to_pos_str[100], loop_end_pos_str[100];
  position_to_string (&self->base.pos, from_pos_str);
  position_to_string (&self->base.end_pos, to_pos_str);
  position_to_string (&self->base.loop_end_pos, loop_end_pos_str);
  snprintf (
    buf, buf_size,
    "%s [%s] - track name hash %u - lane pos %d - "
    "idx %d - address %p - <%s> to <%s> (%ld frames, %f ticks) - "
    "loop end <%s> - link group %d",
    self->name, region_identifier_get_region_type_name (self->id.type),
    self->id.track_name_hash, self->id.lane_pos, self->id.idx, self,
    from_pos_str, to_pos_str, self->base.end_pos.frames - self->base.pos.frames,
    self->base.end_pos.ticks - self->base.pos.ticks, loop_end_pos_str,
    self->id.link_group);
}

/**
 * Print region info for debugging.
 */
void
region_print (const ZRegion * self)
{
  size_t sz = 2000;
  char   buf[sz];
  region_print_to_str (self, buf, sz);
  g_message ("%s", buf);
}

/**
 * Returns the region at the given position in the
 * given Track.
 *
 * @param at The automation track to look in.
 * @param track The track to look in, if at is
 *   NULL.
 * @param pos The position.
 */
ZRegion *
region_at_position (
  const Track *           track,
  const AutomationTrack * at,
  const Position *        pos)
{
  ZRegion * region;
  if (track)
    {
      TrackLane * lane;
      for (int i = 0; i < track->num_lanes; i++)
        {
          lane = track->lanes[i];
          for (int j = 0; j < lane->num_regions; j++)
            {
              region = lane->regions[j];
              ArrangerObject * r_obj = (ArrangerObject *) region;
              if (
                position_is_after_or_equal (pos, &r_obj->pos)
                && position_is_before_or_equal (pos, &r_obj->end_pos))
                {
                  return region;
                }
            }
        }
    }
  else if (at)
    {
      for (int i = 0; i < at->num_regions; i++)
        {
          region = at->regions[i];
          ArrangerObject * r_obj = (ArrangerObject *) region;
          if (
            position_is_after_or_equal (pos, &r_obj->pos)
            && position_is_before_or_equal (pos, &r_obj->end_pos))
            {
              return region;
            }
        }
    }
  return NULL;
}

/**
 * Returns if the position is inside the region
 * or not.
 *
 * @param gframes Global position in frames.
 * @param inclusive Whether the last frame should
 *   be counted as part of the region.
 */
int
region_is_hit (
  const ZRegion *      region,
  const signed_frame_t gframes,
  const bool           inclusive)
{
  const ArrangerObject * r_obj = (const ArrangerObject *) region;
  return
    r_obj->pos.frames <= gframes &&
    ((inclusive &&
      r_obj->end_pos.frames >=
        gframes) ||
     (!inclusive &&
      r_obj->end_pos.frames >
        gframes));
}

/**
 * Returns the ArrangerSelections based on the
 * given region type.
 */
ArrangerSelections *
region_get_arranger_selections (ZRegion * self)
{
  ArrangerSelections * sel = NULL;
  switch (self->id.type)
    {
    case REGION_TYPE_MIDI:
      sel = (ArrangerSelections *) MA_SELECTIONS;
      break;
    case REGION_TYPE_AUTOMATION:
      sel = (ArrangerSelections *) AUTOMATION_SELECTIONS;
      break;
    case REGION_TYPE_CHORD:
      sel = (ArrangerSelections *) CHORD_SELECTIONS;
      break;
    case REGION_TYPE_AUDIO:
      sel = (ArrangerSelections *) AUDIO_SELECTIONS;
      break;
    default:
      break;
    }

  return sel;
}

/**
 * Returns the arranger for editing the region's children.
 */
ArrangerWidget *
region_get_arranger_for_children (ZRegion * self)
{
  ArrangerWidget * arranger = NULL;
  switch (self->id.type)
    {
    case REGION_TYPE_MIDI:
      arranger = (ArrangerWidget *) MW_MIDI_ARRANGER;
      break;
    case REGION_TYPE_AUTOMATION:
      arranger = (ArrangerWidget *) MW_AUTOMATION_ARRANGER;
      break;
    case REGION_TYPE_CHORD:
      arranger = (ArrangerWidget *) MW_CHORD_ARRANGER;
      break;
    case REGION_TYPE_AUDIO:
      arranger = (ArrangerWidget *) MW_AUDIO_ARRANGER;
      break;
    default:
      break;
    }

  return arranger;
}

/**
 * Returns whether the region is effectively in
 * musical mode.
 *
 * @note Only applicable to audio regions.
 */
bool
region_get_musical_mode (ZRegion * self)
{
#if ZRYTHM_TARGET_VER_MAJ == 1
  /* off for v1 */
  return false;
#endif

  switch (self->musical_mode)
    {
    case REGION_MUSICAL_MODE_INHERIT:
      return g_settings_get_boolean (S_UI, "musical-mode");
    case REGION_MUSICAL_MODE_OFF:
      return false;
    case REGION_MUSICAL_MODE_ON:
      return true;
    }
  g_return_val_if_reached (false);
}

/**
 * Returns if any part of the ZRegion is inside the
 * given range, inclusive.
 */
int
region_is_hit_by_range (
  const ZRegion *      region,
  const signed_frame_t gframes_start,
  const signed_frame_t gframes_end,
  const bool           end_inclusive)
{
  const ArrangerObject * obj = (const ArrangerObject *) region;
  /* 4 cases:
   * - region start is inside range
   * - region end is inside range
   * - start is inside region
   * - end is inside region
   */
  if (end_inclusive)
    {
      return
        (gframes_start <=
           obj->pos.frames &&
         gframes_end >=
           obj->pos.frames) ||
        (gframes_start <=
           obj->end_pos.frames &&
         gframes_end >=
           obj->end_pos.frames) ||
        region_is_hit (region, gframes_start, 1) ||
        region_is_hit (region, gframes_end, 1);
    }
  else
    {
      return
        (gframes_start <=
           obj->pos.frames &&
         gframes_end >
           obj->pos.frames) ||
        (gframes_start <
           obj->end_pos.frames &&
         gframes_end >
           obj->end_pos.frames) ||
        region_is_hit (region, gframes_start, 0) ||
        region_is_hit (region, gframes_end, 0);
    }
}

/**
 * Copies the data from src to dest.
 *
 * Used when doing/undoing changes in name,
 * clip start point, loop start point, etc.
 */
void
region_copy (ZRegion * src, ZRegion * dest)
{
  g_free (dest->name);
  dest->name = g_strdup (src->name);

  ArrangerObject * src_obj = (ArrangerObject *) src;
  ArrangerObject * dest_obj = (ArrangerObject *) dest;

  dest_obj->clip_start_pos = src_obj->clip_start_pos;
  dest_obj->loop_start_pos = src_obj->loop_start_pos;
  dest_obj->loop_end_pos = src_obj->loop_end_pos;
  dest_obj->fade_in_pos = src_obj->fade_in_pos;
  dest_obj->fade_out_pos = src_obj->fade_out_pos;
}

/**
 * Wrapper for adding an arranger object.
 */
void
region_add_arranger_object (ZRegion * self, ArrangerObject * obj, bool fire_events)
{
  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      chord_region_add_chord_object (self, (ChordObject *) obj, fire_events);
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      midi_region_add_midi_note (self, (MidiNote *) obj, fire_events);
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      automation_region_add_ap (self, (AutomationPoint *) obj, fire_events);
      break;
    default:
      g_return_if_reached ();
      break;
    }
}

/**
 * Converts frames on the timeline (global)
 * to local frames (in the clip).
 *
 * If normalize is 1 it will only return a position
 * from 0 to loop_end (it will traverse the
 * loops to find the appropriate position),
 * otherwise it may exceed loop_end.
 *
 * @param timeline_frames Timeline position in
 *   frames.
 *
 * @return The local frames.
 */
signed_frame_t
region_timeline_frames_to_local (
  const ZRegion * const self,
  const signed_frame_t  timeline_frames,
  const bool            normalize)
{
  g_return_val_if_fail (IS_REGION (self), 0);

  const ArrangerObject * const r_obj = (const ArrangerObject * const) self;

  if (normalize)
    {
      signed_frame_t diff_frames = timeline_frames - r_obj->pos.frames;

      /* special case: timeline frames is exactly at the end of the region */
      if (timeline_frames == r_obj->end_pos.frames)
        return diff_frames;

      const signed_frame_t loop_end_frames = r_obj->loop_end_pos.frames;
      const signed_frame_t clip_start_frames = r_obj->clip_start_pos.frames;
      const signed_frame_t loop_size =
        arranger_object_get_loop_length_in_frames (r_obj);
      z_return_val_if_fail_cmp (loop_size, >, 0, 0);

      diff_frames += clip_start_frames;

      while (diff_frames >= loop_end_frames)
        {
          diff_frames -= loop_size;
        }

      return diff_frames;
    }
  else
    {
      return timeline_frames - r_obj->pos.frames;
    }
}

/**
 * Returns the number of frames until the next
 * loop end point or the end of the region.
 *
 * @param[in] timeline_frames Global frames at
 *   start.
 * @param[out] ret_frames Return frames.
 * @param[out] is_loop Whether the return frames
 *   are for a loop (if false, the return frames
 *   are for the region's end).
 */
void
region_get_frames_till_next_loop_or_end (
  const ZRegion * const self,
  const signed_frame_t  timeline_frames,
  signed_frame_t *      ret_frames,
  bool *                is_loop)
{
  g_return_if_fail (IS_REGION (self));

  ArrangerObject * r_obj = (ArrangerObject *) self;

  signed_frame_t loop_size = arranger_object_get_loop_length_in_frames (r_obj);
  z_return_if_fail_cmp (loop_size, >, 0);

  signed_frame_t local_frames = timeline_frames - r_obj->pos.frames;

  local_frames += r_obj->clip_start_pos.frames;

  while (local_frames >= r_obj->loop_end_pos.frames)
    {
      local_frames -= loop_size;
    }

  signed_frame_t frames_till_next_loop =
    r_obj->loop_end_pos.frames - local_frames;

  signed_frame_t frames_till_end = r_obj->end_pos.frames - timeline_frames;

  *is_loop = frames_till_next_loop < frames_till_end;
  *ret_frames = MIN (frames_till_end, frames_till_next_loop);
}

/**
 * Generates the filename for this region.
 *
 * MUST be free'd.
 *
 * FIXME logic needs changing
 */
char *
region_generate_filename (ZRegion * region)
{
  Track * track = arranger_object_get_track ((ArrangerObject *) region);
  return g_strdup_printf (REGION_PRINTF_FILENAME, track->name, region->name);
}

/**
 * Returns if this region is currently being
 * recorded onto.
 */
bool
region_is_recording (ZRegion * self)
{
  if (RECORDING_MANAGER->num_active_recordings == 0)
    {
      return false;
    }

  for (int i = 0; i < RECORDING_MANAGER->num_recorded_ids; i++)
    {
      if (region_identifier_is_equal (
            &self->id, &RECORDING_MANAGER->recorded_ids[i]))
        {
          return true;
        }
    }

  return false;
}

/**
 * Disconnects the region and anything using it.
 *
 * Does not free the ZRegion or its children's
 * resources.
 */
void
region_disconnect (ZRegion * self)
{
  ZRegion * clip_editor_region = clip_editor_get_region (CLIP_EDITOR);
  if (clip_editor_region == self)
    {
      clip_editor_set_region (CLIP_EDITOR, NULL, true);
    }
  if (TL_SELECTIONS)
    {
      arranger_selections_remove_object (
        (ArrangerSelections *) TL_SELECTIONS, (ArrangerObject *) self);
    }
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * mn = self->midi_notes[i];
      arranger_selections_remove_object (
        (ArrangerSelections *) MA_SELECTIONS, (ArrangerObject *) mn);
    }
  for (int i = 0; i < self->num_chord_objects; i++)
    {
      ChordObject * c = self->chord_objects[i];
      arranger_selections_remove_object (
        (ArrangerSelections *) CHORD_SELECTIONS, (ArrangerObject *) c);
    }
  for (int i = 0; i < self->num_aps; i++)
    {
      AutomationPoint * ap = self->aps[i];
      arranger_selections_remove_object (
        (ArrangerSelections *) AUTOMATION_SELECTIONS, (ArrangerObject *) ap);
    }
  if (ZRYTHM_HAVE_UI)
    {
      /*ARRANGER_WIDGET_GET_PRIVATE (MW_TIMELINE);*/
      /*if (ar_prv->start_object ==*/
      /*(ArrangerObject *) self)*/
      /*ar_prv->start_object = NULL;*/
    }
}
