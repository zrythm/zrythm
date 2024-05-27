// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_track.h"
#include "dsp/engine.h"
#include "dsp/marker_track.h"
#include "dsp/midi_event.h"
#include "dsp/position.h"
#include "dsp/tempo_track.h"
#include "dsp/track.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/timeline_selections.h"
#include "io/midi_file.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/debug.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/yaml.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"
#include <limits.h>

/**
 * Creates a new TimelineSelections instance for
 * the given range.
 *
 * @bool clone_objs True to clone each object,
 *   false to use pointers to project objects.
 */
TimelineSelections *
timeline_selections_new_for_range (
  Position * start_pos,
  Position * end_pos,
  bool       clone_objs)
{
  TimelineSelections * self = (TimelineSelections *) arranger_selections_new (
    ArrangerSelectionsType::ARRANGER_SELECTIONS_TYPE_TIMELINE);

#define ADD_OBJ(obj) \
  if (arranger_object_is_hit ((ArrangerObject *) (obj), start_pos, end_pos)) \
    { \
      ArrangerObject * _obj = (ArrangerObject *) (obj); \
      if (clone_objs) \
        { \
          _obj = arranger_object_clone (_obj); \
        } \
      arranger_selections_add_object ((ArrangerSelections *) self, _obj); \
    }

  /* regions */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track *               track = TRACKLIST->tracks[i];
      AutomationTracklist * atl = track_get_automation_tracklist (track);
      for (int j = 0; j < track->num_lanes; j++)
        {
          TrackLane * lane = track->lanes[j];
          for (int k = 0; k < lane->num_regions; k++)
            {
              ADD_OBJ (lane->regions[k]);
            }
        }
      for (int j = 0; j < track->num_chord_regions; j++)
        {
          ADD_OBJ (track->chord_regions[j]);
        }
      if (atl)
        {
          for (int j = 0; j < atl->num_ats; j++)
            {
              AutomationTrack * at = atl->ats[j];

              for (int k = 0; k < at->num_regions; k++)
                {
                  ADD_OBJ (at->regions[k]);
                }
            }
        }
      for (int j = 0; j < track->num_scales; j++)
        {
          ADD_OBJ (track->scales[j]);
        }
      for (int j = 0; j < track->num_markers; j++)
        {
          ADD_OBJ (track->markers[j]);
        }
    }

#undef ADD_OBJ

  return self;
}

/**
 * Gets highest track in the selections.
 */
Track *
timeline_selections_get_last_track (TimelineSelections * ts)
{
  int     track_pos = -1;
  Track * track = NULL;
  int     tmp_pos;
  Track * tmp_track;

#define CHECK_POS(_track) \
  tmp_track = _track; \
  tmp_pos = tracklist_get_track_pos (TRACKLIST, tmp_track); \
  if (tmp_pos > track_pos) \
    { \
      track_pos = tmp_pos; \
      track = tmp_track; \
    }

  Region * region;
  for (int i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      ArrangerObject * r_obj = (ArrangerObject *) region;
      Track *          _track = arranger_object_get_track (r_obj);
      CHECK_POS (_track);
    }
  CHECK_POS (P_CHORD_TRACK);

  return track;
#undef CHECK_POS
}

/**
 * Gets lowest track in the selections.
 */
Track *
timeline_selections_get_first_track (TimelineSelections * ts)
{
  int     track_pos = INT_MAX;
  Track * track = NULL;
  int     tmp_pos;
  Track * tmp_track;

#define CHECK_POS(_track) \
  tmp_track = _track; \
  tmp_pos = tracklist_get_track_pos (TRACKLIST, tmp_track); \
  if (tmp_pos < track_pos) \
    { \
      track_pos = tmp_pos; \
      track = tmp_track; \
    }

  Region * region;
  for (int i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      ArrangerObject * r_obj = (ArrangerObject *) region;
      Track *          _track = arranger_object_get_track (r_obj);
      CHECK_POS (_track);
    }
  if (ts->num_scale_objects > 0)
    {
      CHECK_POS (P_CHORD_TRACK);
    }
  if (ts->num_markers > 0)
    {
      CHECK_POS (P_MARKER_TRACK);
    }

  return track;
#undef CHECK_POS
}

#if 0
/**
 * Gets index of the lowest track in the selections.
 *
 * Used during pasting.
 */
static int
get_lowest_track_pos (TimelineSelections * ts)
{
  int track_pos = INT_MAX;

#  define CHECK_POS(id) \
    { \
    }

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * r = ts->regions[i];
      Track *   tr = tracklist_find_track_by_name_hash (
          TRACKLIST, r->id.track_name_hash);
      if (tr->pos < track_pos)
        {
          track_pos = tr->pos;
        }
    }

  if (ts->num_scale_objects > 0)
    {
      if (ts->chord_track_vis_index < track_pos)
        track_pos = ts->chord_track_vis_index;
    }
  if (ts->num_markers > 0)
    {
      if (ts->marker_track_vis_index < track_pos)
        track_pos = ts->marker_track_vis_index;
    }

  return track_pos;
}
#endif

/**
 * Replaces the track positions in each object with
 * visible track indices starting from 0.
 *
 * Used during copying.
 */
void
timeline_selections_set_vis_track_indices (TimelineSelections * ts)
{
  int     i;
  Track * highest_tr = timeline_selections_get_first_track (ts);

  for (i = 0; i < ts->num_regions; i++)
    {
      Region * r = ts->regions[i];
      Track *  region_track = arranger_object_get_track ((ArrangerObject *) r);
      ts->region_track_vis_index =
        tracklist_get_visible_track_diff (TRACKLIST, highest_tr, region_track);
    }
  if (ts->num_scale_objects > 0)
    ts->chord_track_vis_index =
      tracklist_get_visible_track_diff (TRACKLIST, highest_tr, P_CHORD_TRACK);
  if (ts->num_markers > 0)
    ts->marker_track_vis_index =
      tracklist_get_visible_track_diff (TRACKLIST, highest_tr, P_MARKER_TRACK);
}

/**
 * Returns whether the selections can be pasted.
 *
 * Zrythm only supports pasting all the selections into a
 * single destination track.
 *
 * @param pos Position to paste to.
 * @param idx Track index to start pasting to.
 */
int
timeline_selections_can_be_pasted (
  TimelineSelections * ts,
  Position *           pos,
  const int            idx)
{
  Track * tr = TRACKLIST_SELECTIONS->tracks[0];
  if (!tr)
    return false;

  for (int j = 0; j < ts->num_regions; j++)
    {
      Region * r = ts->regions[j];

      /* automation regions can't be copy-pasted this way */
      if (r->id.type == RegionType::REGION_TYPE_AUTOMATION)
        return false;

      /* check if this track can host this region */
      if (!track_type_can_host_region_type (tr->type, r->id.type))
        {
          g_message (
            "track %s can't host region type %s", tr->name,
            ENUM_NAME (r->id.type));
          return false;
        }
    }

  if (ts->num_scale_objects > 0 && tr->type != TrackType::TRACK_TYPE_CHORD)
    return false;

  if (ts->num_markers > 0 && tr->type != TrackType::TRACK_TYPE_MARKER)
    return false;

  return true;
}

#if 0
void
timeline_selections_paste_to_pos (
  TimelineSelections * ts,
  Position *           pos)
{
  Track * track =
    TRACKLIST_SELECTIONS->tracks[0];

  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS, F_NO_FREE);

  double pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  arranger_selections_get_start_pos (
    (ArrangerSelections *) ts, &start_pos,
    F_GLOBAL);
  double start_pos_ticks =
    position_to_ticks (&start_pos);

  double curr_ticks, diff;
  int i;
  for (i = 0; i < ts->num_regions; i++)
    {
      Region * region = ts->regions[i];
      ArrangerObject * r_obj =
        (ArrangerObject *) region;
      Track * region_track =
        tracklist_get_visible_track_after_delta (
          TRACKLIST, track, region->id.track_pos);
      g_return_if_fail (region_track);

      /* update positions */
      curr_ticks =
        position_to_ticks (&r_obj->pos);
      diff = curr_ticks - start_pos_ticks;
      position_from_ticks (
        &r_obj->pos, pos_ticks + diff);
      curr_ticks =
        position_to_ticks (&r_obj->end_pos);
      diff = curr_ticks - start_pos_ticks;
      position_from_ticks (
        &r_obj->end_pos, pos_ticks + diff);
      /* TODO */

      /* clone and add to track */
      Region * cp =
        (Region *)
        arranger_object_clone (
          r_obj,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);

      switch (region->id.type)
        {
        case RegionType::REGION_TYPE_MIDI:
        case RegionType::REGION_TYPE_AUDIO:
          track_add_region (
            region_track, cp, NULL,
            region->id.lane_pos,
            F_GEN_NAME, F_PUBLISH_EVENTS);
          break;
        case RegionType::REGION_TYPE_AUTOMATION:
          {
            AutomationTrack * at =
              region_track->automation_tracklist.
                ats[region->id.at_idx];
            g_return_if_fail (at);
            track_add_region (
              region_track, cp, at, -1,
              F_GEN_NAME, F_PUBLISH_EVENTS);
          }
          break;
        case RegionType::REGION_TYPE_CHORD:
          track_add_region (
            region_track, cp, NULL, -1,
            F_GEN_NAME, F_PUBLISH_EVENTS);
          break;
        }

      /* select it */
      arranger_object_select (
        (ArrangerObject *) cp, F_SELECT,
        F_APPEND);
    }
  for (i = 0; i < ts->num_scale_objects; i++)
    {
      ScaleObject * scale = ts->scale_objects[i];
      ArrangerObject * s_obj =
        (ArrangerObject *) scale;

      curr_ticks =
        position_to_ticks (&s_obj->pos);
      diff = curr_ticks - start_pos_ticks;
      position_from_ticks (
        &s_obj->pos, pos_ticks + diff);

      /* clone and add to track */
      ScaleObject * clone =
        (ScaleObject *)
        arranger_object_clone (
          s_obj,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
      chord_track_add_scale (
        P_CHORD_TRACK, clone);

      /* select it */
      arranger_object_select (
        (ArrangerObject *) clone, F_SELECT,
        F_APPEND);
    }
  for (i = 0; i < ts->num_markers; i++)
    {
      Marker * m = ts->markers[i];
      ArrangerObject * m_obj =
        (ArrangerObject *) m;

      curr_ticks =
        position_to_ticks (&m_obj->pos);
      diff = curr_ticks - start_pos_ticks;
      position_from_ticks (
        &m_obj->pos, pos_ticks + diff);

      /* clone and add to track */
      Marker * clone =
        (Marker *)
        arranger_object_clone (
          m_obj,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
      marker_track_add_marker (
        P_MARKER_TRACK, clone);

      /* select it */
      arranger_object_select (
        (ArrangerObject *) clone, F_SELECT,
        F_APPEND);
    }
#  undef DIFF
}
#endif

/**
 * @param with_parents Also mark all the track's
 *   parents recursively.
 */
void
timeline_selections_mark_for_bounce (TimelineSelections * ts, bool with_parents)
{
  engine_reset_bounce_mode (AUDIO_ENGINE);

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * r = ts->regions[i];
      Track *  track = arranger_object_get_track ((ArrangerObject *) r);
      g_return_if_fail (track);

      if (!with_parents)
        {
          track->bounce_to_master = true;
        }
      track_mark_for_bounce (
        track, F_BOUNCE, F_NO_MARK_REGIONS, F_MARK_CHILDREN, with_parents);
      r->bounce = 1;
    }
}

static bool
move_regions_to_new_lanes_or_tracks_or_ats (
  TimelineSelections * self,
  const int            vis_track_diff,
  const int            lane_diff,
  const int            vis_at_diff)
{
  /* if nothing to do return */
  if (vis_track_diff == 0 && lane_diff == 0 && vis_at_diff == 0)
    return false;

  /* only 1 operation supported at once */
  if (vis_track_diff != 0)
    {
      g_return_val_if_fail (lane_diff == 0 && vis_at_diff == 0, false);
    }
  if (lane_diff != 0)
    {
      g_return_val_if_fail (vis_track_diff == 0 && vis_at_diff == 0, false);
    }
  if (vis_at_diff != 0)
    {
      g_return_val_if_fail (lane_diff == 0 && vis_track_diff == 0, false);
    }

  /* if there are objects other than regions, moving is not
   * supported */
  int num_objs =
    arranger_selections_get_num_objects ((ArrangerSelections *) self);
  if (num_objs != self->num_regions)
    {
      g_debug (
        "selection contains non-regions - skipping "
        "moving to another track/lane");
      return false;
    }

  arranger_selections_sort_by_indices ((ArrangerSelections *) self, false);

  /* store selected regions because they will be
   * deselected during moving */
  z_return_val_if_fail_cmp (self->num_regions, >=, 0, false);
  GPtrArray * regions_arr = g_ptr_array_sized_new ((guint) self->num_regions);
  for (int i = 0; i < self->num_regions; i++)
    {
      g_ptr_array_add (regions_arr, self->regions[i]);
    }

  /*
   * for tracks, check that:
   * - all regions can be moved to a compatible track
   * for lanes, check that:
   * - all regions are in the same track
   * - only lane regions are selected
   * - the lane bounds are not exceeded
   */
  bool compatible = true;
  for (size_t i = 0; i < regions_arr->len; i++)
    {
      Region *         region = (Region *) g_ptr_array_index (regions_arr, i);
      ArrangerObject * r_obj = (ArrangerObject *) region;
      Track *          track = arranger_object_get_track (r_obj);
      track->block_auto_creation_and_deletion = true;
      if (vis_track_diff != 0)
        {
          Track * visible = tracklist_get_visible_track_after_delta (
            TRACKLIST, track, vis_track_diff);
          if (
            !visible
            || !track_type_is_compatible_for_moving (track->type, visible->type)
            ||
            /* do not allow moving automation tracks
             * to other tracks for now */
            region->id.type == RegionType::REGION_TYPE_AUTOMATION)
            {
              compatible = false;
              break;
            }
        }
      else if (lane_diff != 0)
        {
          if (region->id.lane_pos + lane_diff < 0)
            {
              compatible = false;
              break;
            }

          /* don't create more than 1 extra lanes */
          TrackLane * lane = region_get_lane (region);
          g_return_val_if_fail (region && lane, -1);
          int new_lane_pos = lane->pos + lane_diff;
          g_return_val_if_fail (new_lane_pos >= 0, -1);
          if (new_lane_pos >= track->num_lanes)
            {
              g_debug (
                "new lane position %d is >= the number of lanes in the track (%d)",
                new_lane_pos, track->num_lanes);
              compatible = false;
              break;
            }
          if (
            new_lane_pos > track->last_lane_created
            && track->last_lane_created > 0 && lane_diff > 0)
            {
              g_debug (
                "already created a new lane at %d, skipping new lane for %d",
                track->last_lane_created, new_lane_pos);
              compatible = false;
              break;
            }

          if (region->id.type == RegionType::REGION_TYPE_AUTOMATION)
            {
              compatible = false;
              break;
            }
        }
      else if (vis_at_diff != 0)
        {
          if (region->id.type != RegionType::REGION_TYPE_AUTOMATION)
            {
              compatible = false;
              break;
            }

          /* don't allow moving automation regions -- too
           * error prone */
          compatible = false;
          break;
        }
    }
  if (!compatible)
    {
      g_ptr_array_free (regions_arr, true);
      return false;
    }

  /* new positions are all compatible, move the
   * regions */
  for (size_t i = 0; i < regions_arr->len; i++)
    {
      Region *         region = (Region *) g_ptr_array_index (regions_arr, i);
      ArrangerObject * r_obj = (ArrangerObject *) region;
      if (vis_track_diff != 0)
        {
          Track * region_track = arranger_object_get_track (r_obj);
          g_warn_if_fail (region && region_track);
          Track * track_to_move_to = tracklist_get_visible_track_after_delta (
            TRACKLIST, region_track, vis_track_diff);
          g_warn_if_fail (track_to_move_to);

          region_move_to_track (region, track_to_move_to, -1, -1);
        }
      else if (lane_diff != 0)
        {
          TrackLane * lane = region_get_lane (region);
          g_return_val_if_fail (region && lane, -1);

          int new_lane_pos = lane->pos + lane_diff;
          g_return_val_if_fail (new_lane_pos >= 0, -1);
          Track * track = track_lane_get_track (lane);
          bool    new_lanes_created =
            track_create_missing_lanes (track, new_lane_pos);
          if (new_lanes_created)
            {
              track->last_lane_created = new_lane_pos;
            }

          region_move_to_track (region, track, new_lane_pos, -1);
        }
      else if (vis_at_diff != 0)
        {
          AutomationTrack * at = region_get_automation_track (region);
          g_return_val_if_fail (region && at, -1);
          AutomationTracklist * atl =
            automation_track_get_automation_tracklist (at);
          AutomationTrack * new_at =
            automation_tracklist_get_visible_at_after_delta (
              atl, at, vis_at_diff);

          if (at != new_at)
            {
              /* TODO */
              g_warning ("!MOVING!");
              /*automation_track_remove_region (at, region);*/
              /*automation_track_add_region (new_at, region);*/
            }
        }
    }

  EVENTS_PUSH (EventType::ET_TRACK_LANES_VISIBILITY_CHANGED, NULL);

  g_ptr_array_free (regions_arr, true);

  return true;
}

/**
 * Move the selected regions to new automation tracks.
 *
 * @return True if moved.
 */
bool
timeline_selections_move_regions_to_new_ats (
  TimelineSelections * self,
  const int            vis_at_diff)
{
  return move_regions_to_new_lanes_or_tracks_or_ats (self, 0, 0, vis_at_diff);
}

/**
 * Move the selected Regions to new lanes.
 *
 * @param diff The delta to move the tracks.
 *
 * @return True if moved.
 */
bool
timeline_selections_move_regions_to_new_lanes (
  TimelineSelections * self,
  const int            diff)
{
  return move_regions_to_new_lanes_or_tracks_or_ats (self, 0, diff, 0);
}

/**
 * Move the selected Regions to the new Track.
 *
 * @param new_track_is_before 1 if the Region's should move to
 *   their previous tracks, 0 for their next tracks.
 *
 * @return True if moved.
 */
bool
timeline_selections_move_regions_to_new_tracks (
  TimelineSelections * self,
  const int            vis_track_diff)
{
  return move_regions_to_new_lanes_or_tracks_or_ats (self, vis_track_diff, 0, 0);
}

/**
 * Sets the regions'
 * \ref Region.index_in_prev_lane.
 */
void
timeline_selections_set_index_in_prev_lane (TimelineSelections * self)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      Region *         r = self->regions[i];
      ArrangerObject * r_obj = (ArrangerObject *) r;
      r_obj->index_in_prev_lane = r->id.idx;
    }
}

bool
timeline_selections_contains_only_regions (const TimelineSelections * self)
{
  return self->num_regions > 0 && self->num_scale_objects == 0
         && self->num_markers == 0;
}

bool
timeline_selections_contains_only_region_types (
  const TimelineSelections * self,
  RegionType                 types)
{
  if (!timeline_selections_contains_only_regions (self))
    return false;

  for (int i = 0; i < self->num_regions; i++)
    {
      Region * r = self->regions[i];
      if (!ENUM_BITSET_TEST (RegionType, types, r->id.type))
        return false;
    }
  return true;
}

/**
 * Exports the selections to the given MIDI file.
 */
bool
timeline_selections_export_to_midi_file (
  const TimelineSelections * self,
  const char *               full_path,
  int                        midi_version,
  const bool                 export_full_regions,
  const bool                 lanes_as_tracks)
{
  MIDI_FILE * mf;

  if ((mf = midiFileCreate (full_path, TRUE)))
    {
      /* Write tempo information out to track 1 */
      midiSongAddTempo (
        mf, 1, (int) tempo_track_get_current_bpm (P_TEMPO_TRACK));

      /* All data is written out to tracks not
       * channels. We therefore set the current
       * channel before writing data out. Channel
       * assignments can change any number of times
       * during the file, and affect all tracks
       * messages until it is changed. */
      midiFileSetTracksDefaultChannel (mf, 1, MIDI_CHANNEL_1);

      midiFileSetPPQN (mf, TICKS_PER_QUARTER_NOTE);

      midiFileSetVersion (mf, midi_version);

      /* common time: 4 crochet beats, per bar */
      int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
      midiSongAddSimpleTimeSig (
        mf, 1, beats_per_bar,
        math_round_double_to_signed_32 (TRANSPORT->ticks_per_beat));

      TimelineSelections * sel_clone = (TimelineSelections *)
        arranger_selections_clone ((ArrangerSelections *) self);
      arranger_selections_sort_by_indices (
        (ArrangerSelections *) sel_clone, false);

      int          last_midi_track_pos = -1;
      MidiEvents * events = NULL;
      for (int i = 0; i < sel_clone->num_regions; i++)
        {
          const Region * r = sel_clone->regions[i];

          int midi_track_pos = 1;
          if (midi_version > 0)
            {
              /* add track name */
              Track * track =
                arranger_object_get_track ((const ArrangerObject *) r);
              g_return_val_if_fail (track, false);
              char midi_track_name[1000];
              if (lanes_as_tracks)
                {
                  TrackLane * lane = region_get_lane (r);
                  g_return_val_if_fail (lane, false);
                  sprintf (midi_track_name, "%s - %s", track->name, lane->name);
                  midi_track_pos = track_lane_calculate_lane_idx (lane);
                }
              else
                {
                  strcpy (midi_track_name, track->name);
                  midi_track_pos = track->pos;
                }
              midiTrackAddText (
                mf, midi_track_pos, textTrackName, midi_track_name);
            }

          if (last_midi_track_pos == midi_track_pos)
            {
              g_return_val_if_fail (events, false);
            }
          else
            {
              /* finish prev events if any */
              if (events)
                {
                  midi_events_write_to_midi_file (
                    events, mf, last_midi_track_pos);
                  object_free_w_func_and_null (midi_events_free, events);
                }

              /* start new events */
              events = midi_events_new ();
            }

          /* append to the current events */
          midi_region_add_events (
            r, events, NULL, NULL, true, export_full_regions);
          last_midi_track_pos = midi_track_pos;
        }

      /* finish prev events if any again */
      if (events)
        {
          midi_events_write_to_midi_file (events, mf, last_midi_track_pos);
          object_free_w_func_and_null (midi_events_free, events);
        }

      arranger_selections_free_full ((ArrangerSelections *) sel_clone);

      midiFileClose (mf);
    }

  return true;
}
