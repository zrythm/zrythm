// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <inttypes.h>

#include "actions/arranger_selections.h"
#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/clip.h"
#include "dsp/control_port.h"
#include "dsp/engine.h"
#include "dsp/recording_event.h"
#include "dsp/recording_manager.h"
#include "dsp/track.h"
#include "dsp/transport.h"
#include "gui/backend/arranger_object.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#if 0
static int received = 0;
static int returned = 0;

#  define UP_RECEIVED(x) \
    received++; \
    /*g_message ("RECEIVED"); \*/
  /*recording_event_print (x)*/

#  define UP_RETURNED(x) \
    returned++; \
    /*g_message ("RETURNED"); \*/
  /*recording_event_print (x)*/
#endif

/**
 * Adds the region's identifier to the recorded
 * identifiers (to be used for creating the undoable
 * action when recording stops.
 */
NONNULL static void
add_recorded_id (RecordingManager * self, ZRegion * region)
{
  /*region_identifier_print (&region->id);*/
  region_identifier_copy (
    &self->recorded_ids[self->num_recorded_ids], &region->id);
  self->num_recorded_ids++;
}

static void
free_temp_selections (RecordingManager * self)
{
  if (self->selections_before_start)
    {
      object_free_w_func_and_null (
        arranger_selections_free, self->selections_before_start);
    }
}

static void
handle_stop_recording (RecordingManager * self, bool is_automation)
{
  g_return_if_fail (self->num_active_recordings > 0);

  /* skip if still recording */
  if (self->num_active_recordings > 1)
    {
      self->num_active_recordings--;
      return;
    }

  g_message (
    "%s%s", "----- stopped recording", is_automation ? " (automation)" : "");

  /* cache the current selections */
  ArrangerSelections * prev_selections =
    arranger_selections_clone ((ArrangerSelections *) TL_SELECTIONS);

  /* select all the recorded regions */
  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS, F_NO_FREE, F_PUBLISH_EVENTS);
  for (int i = 0; i < self->num_recorded_ids; i++)
    {
      RegionIdentifier * id = &self->recorded_ids[i];

      if (
        (is_automation && id->type != REGION_TYPE_AUTOMATION)
        || (!is_automation && id->type == REGION_TYPE_AUTOMATION))
        continue;

      /* do some sanity checks for lane regions */
      if (region_type_has_lane (id->type))
        {
          Track * track =
            tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash);
          g_return_if_fail (IS_TRACK_AND_NONNULL (track));

          g_return_if_fail (id->lane_pos < track->num_lanes);
          TrackLane * lane = track->lanes[id->lane_pos];
          g_return_if_fail (lane);

          g_return_if_fail (id->idx <= lane->num_regions);
        }

      /*region_identifier_print (id);*/
      ZRegion * region = region_find (id);
      g_return_if_fail (region);
      arranger_selections_add_object (
        (ArrangerSelections *) TL_SELECTIONS, (ArrangerObject *) region);

      if (is_automation)
        {
          region->last_recorded_ap = NULL;
        }
    }

  /* perform the create action */
  GError * err = NULL;
  bool     ret = arranger_selections_action_perform_record (
    self->selections_before_start, (ArrangerSelections *) TL_SELECTIONS, true,
    &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to create recorded regions"));
    }

  /* update frame caches and write audio clips to
   * pool */
  for (int i = 0; i < self->num_recorded_ids; i++)
    {
      ZRegion * r = region_find (&self->recorded_ids[i]);
      if (r->id.type == REGION_TYPE_AUDIO)
        {
          AudioClip * clip = audio_region_get_clip (r);
          bool        success =
            audio_clip_write_to_pool (clip, true, F_NOT_BACKUP, &err);
          if (!success)
            {
              HANDLE_ERROR (
                err, "%s", "Failed to write audio region clip to pool");
            }
        }
    }

  /* restore the selections */
  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS, F_NO_FREE, F_PUBLISH_EVENTS);
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (prev_selections, objs_arr);
  for (size_t i = 0; i < objs_arr->len; i++)
    {
      ArrangerObject * sel_obj =
        (ArrangerObject *) g_ptr_array_index (objs_arr, i);
      g_return_if_fail (sel_obj);
      ArrangerObject * obj = arranger_object_find (sel_obj);
      g_return_if_fail (obj);
      arranger_object_select (obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
    }
  g_ptr_array_unref (objs_arr);

  /* free the temporary selections */
  free_temp_selections (self);

  /* disarm transport record button */
  transport_set_recording (TRANSPORT, false, true, F_PUBLISH_EVENTS);

  self->num_active_recordings--;
  self->num_recorded_ids = 0;
  g_warn_if_fail (self->num_active_recordings == 0);
}

/**
 * Handles the recording logic inside the process
 * cycle.
 *
 * The MidiEvents are already dequeued at this
 * point.
 *
 * @param g_frames_start Global start frames.
 * @param nframes Number of frames to process. If
 *   this is zero, a pause will be added. See \ref
 *   RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING and
 *   RECORDING_EVENT_TYPE_PAUSE_AUTOMATION_RECORDING.
 */
void
recording_manager_handle_recording (
  RecordingManager *                  self,
  const TrackProcessor *              track_processor,
  const EngineProcessTimeInfo * const time_nfo)
{
#if 0
  g_message (
    "handling recording from %ld (%" PRIu32
    " frames)",
    g_start_frames + ev->local_offset, ev->nframes);
#endif

  /* whether to skip adding track recording
   * events */
  bool skip_adding_track_events = false;

  /* whether to skip adding automation recording
   * events */
  bool skip_adding_automation_events = false;

  /* whether we are inside the punch range in
   * punch mode or true if otherwise */
  bool inside_punch_range = false;

  z_return_if_fail_cmp (
    time_nfo->local_offset + time_nfo->nframes, <=, AUDIO_ENGINE->block_length);

  if (TRANSPORT->punch_mode)
    {
      Position tmp;
      position_from_frames (
        &tmp, (signed_frame_t) time_nfo->g_start_frame_w_offset);
      inside_punch_range =
        transport_position_is_inside_punch_range (TRANSPORT, &tmp);
    }
  else
    {
      inside_punch_range = true;
    }

  /* ---- handle start/stop/pause recording events
   * ---- */

  Track *               tr = track_processor_get_track (track_processor);
  AutomationTracklist * atl = track_get_automation_tracklist (tr);
  gint64                cur_time = g_get_monotonic_time ();

  /* if track type can't record do nothing */
  if (G_UNLIKELY (!track_type_can_record (tr->type)))
    {
    }
  /* else if not recording at all (recording stopped) */
  else if (
    !TRANSPORT->recording || !track_get_recording (tr) || !TRANSPORT_IS_ROLLING)
    {
      /* if track had previously recorded */
      if (G_UNLIKELY (tr->recording_region) && !tr->recording_stop_sent)
        {
          tr->recording_stop_sent = true;

          /* send stop recording event */
          RecordingEvent * re =
            (RecordingEvent *) object_pool_get (self->event_obj_pool);
          recording_event_init (re);
          re->type = RECORDING_EVENT_TYPE_STOP_TRACK_RECORDING;
          re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
          re->local_offset = time_nfo->local_offset;
          re->nframes = time_nfo->nframes;
          re->track_name_hash = tr->name_hash;
          /*UP_RECEIVED (re);*/
          recording_event_queue_push_back_event (self->event_queue, re);
        }
      skip_adding_track_events = true;
    }
  /* if pausing */
  else if (time_nfo->nframes == 0)
    {
      if (tr->recording_region || tr->recording_start_sent)
        {
          /* send pause event */
          RecordingEvent * re =
            (RecordingEvent *) object_pool_get (self->event_obj_pool);
          recording_event_init (re);
          re->type = RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING;
          re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
          re->local_offset = time_nfo->local_offset;
          re->nframes = time_nfo->nframes;
          re->track_name_hash = tr->name_hash;
          recording_event_queue_push_back_event (self->event_queue, re);

          skip_adding_track_events = true;
        }
    }
  /* if recording and inside punch range */
  else if (inside_punch_range)
    {
      /* if no recording started yet */
      if (!tr->recording_region && !tr->recording_start_sent)
        {
          tr->recording_start_sent = true;

          /* send start recording event */
          RecordingEvent * re =
            (RecordingEvent *) object_pool_get (self->event_obj_pool);
          recording_event_init (re);
          re->type = RECORDING_EVENT_TYPE_START_TRACK_RECORDING;
          re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
          re->local_offset = time_nfo->local_offset;
          re->nframes = time_nfo->nframes;
          re->track_name_hash = tr->name_hash;
          /*UP_RECEIVED (re);*/
          recording_event_queue_push_back_event (self->event_queue, re);
        }
    }
  else if (!inside_punch_range)
    {
      skip_adding_track_events = true;
    }

  for (
    int i = 0;
    /* use cache */
    i < atl->num_ats_in_record_mode; i++)
    {
      AutomationTrack * at = atl->ats_in_record_mode[i];

      bool at_should_be_recording =
        automation_track_should_be_recording (at, cur_time, false);

      /* if should stop automation recording */
      if (
        G_UNLIKELY (at->recording_started)
        && (!TRANSPORT_IS_ROLLING || !at_should_be_recording))
        {
          /* send stop automation recording event */
          RecordingEvent * re =
            (RecordingEvent *) object_pool_get (self->event_obj_pool);
          recording_event_init (re);
          re->type = RECORDING_EVENT_TYPE_STOP_AUTOMATION_RECORDING;
          re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
          re->local_offset = time_nfo->local_offset;
          re->nframes = time_nfo->nframes;
          re->automation_track_idx = at->index;
          re->track_name_hash = tr->name_hash;
          /*UP_RECEIVED (re);*/
          recording_event_queue_push_back_event (self->event_queue, re);

          skip_adding_automation_events = true;
        }
      /* if pausing (only at loop end) */
      else if (
        G_UNLIKELY (
          at->recording_start_sent
          && time_nfo->nframes == 0)
        &&
        (time_nfo->g_start_frame_w_offset
         ==
         (unsigned_frame_t)
         TRANSPORT->loop_end_pos.frames))
        {
          /* send pause event */
          RecordingEvent * re =
            (RecordingEvent *) object_pool_get (self->event_obj_pool);
          recording_event_init (re);
          re->type = RECORDING_EVENT_TYPE_PAUSE_AUTOMATION_RECORDING;
          re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
          re->local_offset = time_nfo->local_offset;
          re->nframes = time_nfo->nframes;
          re->automation_track_idx = at->index;
          re->track_name_hash = tr->name_hash;
          /*UP_RECEIVED (re);*/
          recording_event_queue_push_back_event (self->event_queue, re);

          skip_adding_automation_events = true;
        }

      /* if automatmion should be recording */
      if (TRANSPORT_IS_ROLLING && at_should_be_recording)
        {
          /* if recording hasn't started yet */
          if (!at->recording_started && !at->recording_start_sent)
            {
              at->recording_start_sent = true;

              /* send start recording event */
              RecordingEvent * re =
                (RecordingEvent *) object_pool_get (self->event_obj_pool);
              recording_event_init (re);
              re->type = RECORDING_EVENT_TYPE_START_AUTOMATION_RECORDING;
              re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
              re->local_offset = time_nfo->local_offset;
              re->nframes = time_nfo->nframes;
              re->automation_track_idx = at->index;
              re->track_name_hash = tr->name_hash;
              /*UP_RECEIVED (re);*/
              recording_event_queue_push_back_event (self->event_queue, re);
            }
        }
    }

  /* ---- end handling start/stop/pause recording
   * events ---- */

  if (!G_LIKELY (skip_adding_track_events))
    {
      /* add recorded track material to event queue */
      if (track_type_has_piano_roll (tr->type) || tr->type == TRACK_TYPE_CHORD)
        {
          MidiEvents * midi_events = track_processor->midi_in->midi_events;

          for (int i = 0; i < midi_events->num_events; i++)
            {
              MidiEvent * me = &midi_events->events[i];

              RecordingEvent * re =
                (RecordingEvent *) object_pool_get (self->event_obj_pool);
              recording_event_init (re);
              re->type = RECORDING_EVENT_TYPE_MIDI;
              re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
              re->local_offset = time_nfo->local_offset;
              re->nframes = time_nfo->nframes;
              re->has_midi_event = 1;
              midi_event_copy (&re->midi_event, me);
              re->track_name_hash = tr->name_hash;
              /*UP_RECEIVED (re);*/
              recording_event_queue_push_back_event (self->event_queue, re);
            }

          if (midi_events->num_events == 0)
            {
              RecordingEvent * re =
                (RecordingEvent *) object_pool_get (self->event_obj_pool);
              recording_event_init (re);
              re->type = RECORDING_EVENT_TYPE_MIDI;
              re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
              re->local_offset = time_nfo->local_offset;
              re->nframes = time_nfo->nframes;
              re->has_midi_event = 0;
              re->track_name_hash = tr->name_hash;
              /*UP_RECEIVED (re);*/
              recording_event_queue_push_back_event (self->event_queue, re);
            }
        }
      else if (tr->type == TRACK_TYPE_AUDIO)
        {
          RecordingEvent * re =
            (RecordingEvent *) object_pool_get (self->event_obj_pool);
          recording_event_init (re);
          re->type = RECORDING_EVENT_TYPE_AUDIO;
          re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
          re->local_offset = time_nfo->local_offset;
          re->nframes = time_nfo->nframes;
          dsp_copy (
            &re->lbuf[time_nfo->local_offset],
            &track_processor->stereo_in->l->buf[time_nfo->local_offset],
            time_nfo->nframes);
          Port * r =
            track_processor->mono
                && control_port_is_toggled (track_processor->mono)
              ? track_processor->stereo_in->l
              : track_processor->stereo_in->r;
          dsp_copy (
            &re->rbuf[time_nfo->local_offset], &r->buf[time_nfo->local_offset],
            time_nfo->nframes);
          re->track_name_hash = tr->name_hash;
          /*UP_RECEIVED (re);*/
          recording_event_queue_push_back_event (self->event_queue, re);
        }
    }

  if (skip_adding_automation_events)
    return;

  /* add automation events */

  if (!TRANSPORT_IS_ROLLING)
    return;

  /* FIXME optimize -- too many loops */
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];

      /* only proceed if automation should be recording */

      if (G_LIKELY (!at->recording_start_sent))
        continue;

      if (!automation_track_should_be_recording (at, cur_time, false))
        continue;

      /* send recording event */
      RecordingEvent * re =
        (RecordingEvent *) object_pool_get (self->event_obj_pool);
      recording_event_init (re);
      re->type = RECORDING_EVENT_TYPE_AUTOMATION;
      re->g_start_frame_w_offset = time_nfo->g_start_frame_w_offset;
      re->local_offset = time_nfo->local_offset;
      re->nframes = time_nfo->nframes;
      re->automation_track_idx = at->index;
      re->track_name_hash = tr->name_hash;
      /*UP_RECEIVED (re);*/
      recording_event_queue_push_back_event (self->event_queue, re);
    }
}

/**
 * Delete automation points since the last recorded
 * automation point and the current position (eg,
 * when in latch mode) if the position is after
 * the last recorded ap.
 *
 * @note Runs in GTK thread only.
 */
static void
delete_automation_points (
  RecordingManager * self,
  AutomationTrack *  at,
  ZRegion *          region,
  Position *         pos)
{
  automation_region_get_aps_since_last_recorded (region, pos, self->pending_aps);
  for (size_t i = 0; i < self->pending_aps->len; i++)
    {
      AutomationPoint * ap = g_ptr_array_index (self->pending_aps, i);
      automation_region_remove_ap (region, ap, false, true);
    }

  /* create a new automation point at the pos with the previous value */
  if (region->last_recorded_ap)
    {
      /* remove the last recorded AP if its
       * previous AP also has the same value */
      AutomationPoint * ap_before_recorded =
        automation_region_get_prev_ap (region, region->last_recorded_ap);
      float prev_fvalue = region->last_recorded_ap->fvalue;
      float prev_normalized_val = region->last_recorded_ap->normalized_val;
      if (
        ap_before_recorded
        && math_floats_equal (
          ap_before_recorded->fvalue, region->last_recorded_ap->fvalue))
        {
          automation_region_remove_ap (
            region, region->last_recorded_ap, false, true);
        }

      ArrangerObject * r_obj = (ArrangerObject *) region;
      Position         adj_pos;
      position_set_to_pos (&adj_pos, pos);
      position_add_ticks (&adj_pos, -r_obj->pos.ticks);
      AutomationPoint * ap =
        automation_point_new_float (prev_fvalue, prev_normalized_val, &adj_pos);
      automation_region_add_ap (region, ap, true);
      region->last_recorded_ap = ap;
    }
}

/**
 * Creates a new automation point and deletes anything between the last recorded
 * automation point and this point.
 *
 * @note Runs in GTK thread only.
 */
static AutomationPoint *
create_automation_point (
  RecordingManager * self,
  AutomationTrack *  at,
  ZRegion *          region,
  float              val,
  float              normalized_val,
  Position *         pos)
{
  automation_region_get_aps_since_last_recorded (region, pos, self->pending_aps);
  for (size_t i = 0; i < self->pending_aps->len; i++)
    {
      AutomationPoint * ap = g_ptr_array_index (self->pending_aps, i);
      automation_region_remove_ap (region, ap, false, true);
    }

  ArrangerObject * r_obj = (ArrangerObject *) region;
  Position         adj_pos;
  position_set_to_pos (&adj_pos, pos);
  position_add_ticks (&adj_pos, -r_obj->pos.ticks);
  if (
    region->last_recorded_ap
    && math_floats_equal (
      region->last_recorded_ap->normalized_val, normalized_val)
    && position_is_equal (&region->last_recorded_ap->base.pos, &adj_pos))
    {
      /* this block is used to avoid duplicate automation points */
      /* TODO this shouldn't happen and needs investigation */
      return NULL;
    }
  else
    {
      AutomationPoint * ap =
        automation_point_new_float (val, normalized_val, &adj_pos);
      ap->curve_opts.curviness = 1.0;
      ap->curve_opts.algo = CURVE_ALGORITHM_PULSE;
      automation_region_add_ap (region, ap, true);
      region->last_recorded_ap = ap;
      return ap;
    }

  return NULL;
}

/**
 * This is called when recording is paused (eg,
 * when playhead is not in recordable area).
 *
 * @note Runs in GTK thread only.
 */
static void
handle_pause_event (RecordingManager * self, RecordingEvent * ev)
{
  Track * tr =
    tracklist_find_track_by_name_hash (TRACKLIST, ev->track_name_hash);

  /* pausition to pause at */
  Position pause_pos;
  position_from_frames (&pause_pos, (signed_frame_t) ev->g_start_frame_w_offset);

#if 0
  g_debug ("track %s pause start frames %" PRIuFAST64 ", nframes %u", tr->name, pause_pos.frames, ev->nframes);
#endif

  if (ev->type == RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING)
    {
      tr->recording_paused = true;

      /* get the recording region */
      ZRegion * region = tr->recording_region;
      g_return_if_fail (region);

      /* remember lane index */
      tr->last_lane_idx = region->id.lane_pos;

      if (tr->in_signal_type == TYPE_EVENT)
        {
          /* add midi note offs at the end */
          MidiNote * mn;
          while ((mn = midi_region_pop_unended_note (region, -1)))
            {
              ArrangerObject * mn_obj = (ArrangerObject *) mn;
              arranger_object_end_pos_setter (mn_obj, &pause_pos);
            }
        }
    }
  else if (ev->type == RECORDING_EVENT_TYPE_PAUSE_AUTOMATION_RECORDING)
    {
      AutomationTrack * at =
        tr->automation_tracklist.ats[ev->automation_track_idx];
      at->recording_paused = true;
    }
}

/**
 * Handles cases where recording events are first
 * received after pausing recording.
 *
 * This should be called on every
 * \ref RECORDING_EVENT_TYPE_MIDI,
 * \ref RECORDING_EVENT_TYPE_AUDIO and
 * \ref RECORDING_EVENT_TYPE_AUTOMATION event
 * and it will handle resume logic automatically
 * if needed.
 *
 * Adds new regions if necessary, etc.
 *
 * @return Whether pause was handled.
 *
 * @note Runs in GTK thread only.
 */
static bool
handle_resume_event (RecordingManager * self, RecordingEvent * ev)
{
  Track * tr =
    tracklist_find_track_by_name_hash (TRACKLIST, ev->track_name_hash);
  gint64 cur_time = g_get_monotonic_time ();

  /* position to resume from */
  Position resume_pos;
  position_from_frames (
    &resume_pos, (signed_frame_t) ev->g_start_frame_w_offset);

#if 0
  g_debug ("track %s resume start frames %" PRIuFAST64 ", nframes %u", tr->name, resume_pos.frames, ev->nframes);
#endif

  /* position 1 frame afterwards */
  Position end_pos = resume_pos;
  position_add_frames (&end_pos, 1);

  if (
    ev->type == RECORDING_EVENT_TYPE_MIDI
    || ev->type == RECORDING_EVENT_TYPE_AUDIO)
    {
      /* not paused, nothing to do */
      if (!tr->recording_paused)
        {
          return false;
        }

      tr->recording_paused = false;

      if (
        TRANSPORT->recording_mode == RECORDING_MODE_TAKES
        || TRANSPORT->recording_mode == RECORDING_MODE_TAKES_MUTED
        || ev->type == RECORDING_EVENT_TYPE_AUDIO)
        {
          /* mute the previous region */
          if (
            (TRANSPORT->recording_mode == RECORDING_MODE_TAKES_MUTED
             || (TRANSPORT->recording_mode == RECORDING_MODE_OVERWRITE_EVENTS && ev->type == RECORDING_EVENT_TYPE_AUDIO))
            && tr->recording_region)
            {
              arranger_object_set_muted (
                (ArrangerObject *) tr->recording_region, F_MUTE,
                F_PUBLISH_EVENTS);
            }

          /* start new region in new lane */
          int new_lane_pos = tr->last_lane_idx + 1;
          int idx_inside_lane =
            tr->num_lanes > new_lane_pos
              ? tr->lanes[new_lane_pos]->num_regions
              : 0;
          ZRegion * new_region = NULL;
          if (tr->type == TRACK_TYPE_CHORD)
            {
              new_region =
                chord_region_new (&resume_pos, &end_pos, tr->num_chord_regions);
            }
          else if (tr->in_signal_type == TYPE_EVENT)
            {
              new_region = midi_region_new (
                &resume_pos, &end_pos, track_get_name_hash (tr), new_lane_pos,
                idx_inside_lane);
            }
          else if (tr->in_signal_type == TYPE_AUDIO)
            {
              char * name = audio_pool_gen_name_for_recording_clip (
                AUDIO_POOL, tr, new_lane_pos);
              new_region = audio_region_new (
                -1, NULL, true, NULL, 1, name, 2, BIT_DEPTH_32, &resume_pos,
                track_get_name_hash (tr), new_lane_pos, idx_inside_lane, NULL);
            }
          g_return_val_if_fail (new_region, false);
          GError * err = NULL;
          bool     success = track_add_region (
            tr, new_region, NULL, new_lane_pos, F_GEN_NAME, F_PUBLISH_EVENTS,
            &err);
          if (!success)
            {
              HANDLE_ERROR_LITERAL (err, "Failed to add region to track");
              return false;
            }

          /* remember region */
          add_recorded_id (self, new_region);
          tr->recording_region = new_region;
        }
      /* if MIDI and overwriting or merging
       * events */
      else if (tr->recording_region)
        {
          /* extend the previous region */
          ArrangerObject * r_obj = (ArrangerObject *) tr->recording_region;
          if (position_is_before (&resume_pos, &r_obj->pos))
            {
              double ticks_delta = r_obj->pos.ticks - resume_pos.ticks;
              arranger_object_set_start_pos_full_size (r_obj, &resume_pos);
              arranger_object_add_ticks_to_children (r_obj, ticks_delta);
            }
          if (position_is_after (&end_pos, &r_obj->end_pos))
            {
              arranger_object_set_end_pos_full_size (r_obj, &end_pos);
            }
        }
    }
  else if (ev->type == RECORDING_EVENT_TYPE_AUTOMATION)
    {
      AutomationTrack * at =
        tr->automation_tracklist.ats[ev->automation_track_idx];
      g_return_val_if_fail (at, false);

      /* not paused, nothing to do */
      if (!at->recording_paused)
        return false;

      Port * port = port_find_from_identifier (&at->port_id);
      float  value = port_get_control_value (port, false);
      float  normalized_value = port_get_control_value (port, true);

      /* get or start new region at resume pos */
      ZRegion * new_region = automation_track_get_region_before_pos (
        at, &resume_pos, true, Z_F_NO_USE_SNAPSHOTS);
      if (
        !new_region
        && automation_track_should_be_recording (at, cur_time, false))
        {
          /* create region */
          new_region = automation_region_new (
            &resume_pos, &end_pos, track_get_name_hash (tr), at->index,
            at->num_regions);
          g_return_val_if_fail (new_region, false);
          GError * err = NULL;
          bool     success = track_add_region (
            tr, new_region, at, -1, F_GEN_NAME, F_PUBLISH_EVENTS, &err);
          if (!success)
            {
              HANDLE_ERROR_LITERAL (err, "Failed to add region to track");
              return false;
            }
        }
      g_return_val_if_fail (new_region, false);
      add_recorded_id (self, new_region);

      if (automation_track_should_be_recording (at, cur_time, true))
        {
          while (
            new_region->num_aps > 0
            && position_is_equal (
              &((ArrangerObject *) new_region->aps[0])->pos, &resume_pos))
            {
              automation_region_remove_ap (
                new_region, new_region->aps[0], false, true);
            }

          /* create/replace ap at loop start */
          create_automation_point (
            self, at, new_region, value, normalized_value, &resume_pos);
        }
    }

  return true;
}

/**
 * @note Runs in GTK thread only.
 */
static void
handle_audio_event (RecordingManager * self, RecordingEvent * ev)
{
  bool handled_resume = handle_resume_event (self, ev);
  (void) handled_resume;
  /*g_debug ("handled resume %d", handled_resume);*/

  Track * tr =
    tracklist_find_track_by_name_hash (TRACKLIST, ev->track_name_hash);

  /* get end position */
  unsigned_frame_t start_frames = ev->g_start_frame_w_offset;
  unsigned_frame_t end_frames = start_frames + (long) ev->nframes;

  Position start_pos, end_pos;
  position_from_frames (&start_pos, (signed_frame_t) start_frames);
  position_from_frames (&end_pos, (signed_frame_t) end_frames);

  /* get the recording region */
  ZRegion * region = tr->recording_region;
  g_return_if_fail (region);
  ArrangerObject * r_obj = (ArrangerObject *) region;

  /* the clip */
  AudioClip * clip = NULL;

  clip = audio_region_get_clip (region);

  /* set region end pos */
  arranger_object_set_end_pos_full_size (r_obj, &end_pos);
  /*r_obj->end_pos.frames = end_pos.frames;*/

  signed_frame_t r_obj_len_frames = (r_obj->end_pos.frames - r_obj->pos.frames);
  z_return_if_fail_cmp (r_obj_len_frames, >=, 0);
  clip->num_frames = (unsigned_frame_t) r_obj_len_frames;
  clip->frames = (sample_t *) realloc (
    clip->frames,
    (size_t) (clip->num_frames * (long) clip->channels) * sizeof (sample_t));
#if 0
  region->frames =
    (sample_t *) realloc (
      region->frames,
      (size_t)
      (clip->num_frames *
         (long) clip->channels) *
      sizeof (sample_t));
  region->num_frames = (size_t) clip->num_frames;
  dsp_copy (
    &region->frames[0], &clip->frames[0],
    (size_t) clip->num_frames * clip->channels);
#endif

  position_from_frames (
    &r_obj->loop_end_pos, r_obj->end_pos.frames - r_obj->pos.frames);

  r_obj->fade_out_pos = r_obj->loop_end_pos;

  /* handle the samples normally */
  nframes_t cur_local_offset = 0;
  for (
    signed_frame_t i = (signed_frame_t) start_frames - r_obj->pos.frames;
    i < (signed_frame_t) end_frames - r_obj->pos.frames; i++)
    {
      z_return_if_fail_cmp (i, >=, 0);
      z_return_if_fail_cmp (i, <, (signed_frame_t) clip->num_frames);
      g_warn_if_fail (
        cur_local_offset >= ev->local_offset
        && cur_local_offset < ev->local_offset + ev->nframes);

      /* set clip frames */
      clip->frames[i * clip->channels] = ev->lbuf[cur_local_offset];
      clip->frames[i * clip->channels + 1] = ev->rbuf[cur_local_offset];

      cur_local_offset++;
    }

  audio_clip_update_channel_caches (clip, (size_t) clip->frames_written);

  /* write to pool if 2 seconds passed since last
   * write */
  gint64 cur_time = g_get_monotonic_time ();
  gint64 nano_sec_to_wait = 2 * 1000 * 1000;
  if (ZRYTHM_TESTING)
    {
      nano_sec_to_wait = 20 * 1000;
    }
  if ((cur_time - clip->last_write) > nano_sec_to_wait)
    {
      GError * err = NULL;
      bool success = audio_clip_write_to_pool (clip, true, F_NOT_BACKUP, &err);
      if (!success)
        {
          HANDLE_ERROR (err, "%s", "Failed to write audio clip to pool");
        }
    }

#if 0
  g_message (
    "%s wrote from %ld to %ld", __func__,
    start_frames, end_frames);
#endif
}

/**
 * @note Runs in GTK thread only.
 */
static void
handle_midi_event (RecordingManager * self, RecordingEvent * ev)
{
  handle_resume_event (self, ev);

  Track * tr =
    tracklist_find_track_by_name_hash (TRACKLIST, ev->track_name_hash);

  g_return_if_fail (tr->recording_region);

  Position start_pos, end_pos;
  position_from_frames (&start_pos, (signed_frame_t) ev->g_start_frame_w_offset);
  position_from_frames (
    &end_pos,
    (signed_frame_t) ev->g_start_frame_w_offset + (signed_frame_t) ev->nframes);

  /* get the recording region */
  ZRegion *        region = tr->recording_region;
  ArrangerObject * r_obj = (ArrangerObject *) region;

  /* set region end pos */
  bool set_end_pos = false;
  switch (TRANSPORT->recording_mode)
    {
    case RECORDING_MODE_OVERWRITE_EVENTS:
    case RECORDING_MODE_MERGE_EVENTS:
      if (position_is_before (&r_obj->end_pos, &end_pos))
        {
          set_end_pos = true;
        }
      break;
    case RECORDING_MODE_TAKES:
    case RECORDING_MODE_TAKES_MUTED:
      set_end_pos = true;
      break;
    }
  if (set_end_pos)
    {
      arranger_object_set_end_pos_full_size (r_obj, &end_pos);
    }

  tr->recording_region = region;

  /* get local positions */
  Position local_pos, local_end_pos;
  position_set_to_pos (&local_pos, &start_pos);
  position_set_to_pos (&local_end_pos, &end_pos);
  position_add_ticks (&local_pos, -r_obj->pos.ticks);
  position_add_ticks (&local_end_pos, -r_obj->pos.ticks);

  /* if overwrite mode, clear any notes inside the range */
  if (TRANSPORT->recording_mode == RECORDING_MODE_OVERWRITE_EVENTS)
    {
      for (int i = region->num_midi_notes - 1; i >= 0; i--)
        {
          MidiNote *       mn = region->midi_notes[i];
          ArrangerObject * mn_obj = (ArrangerObject *) mn;

          if (position_is_between_excl_start (
                &mn_obj->pos, &local_pos,
                &local_end_pos) ||
              position_is_between_excl_start (
                &mn_obj->end_pos, &local_pos,
                &local_end_pos) ||
              (position_is_before (
                 &mn_obj->pos, &local_pos) &&
               position_is_after_or_equal (
                 &mn_obj->end_pos, &local_end_pos)))
            {
              midi_region_remove_midi_note (
                region, mn, F_FREE, F_NO_PUBLISH_EVENTS);
            }
        }
    }

  if (!ev->has_midi_event)
    return;

  /* convert MIDI data to midi notes */
  MidiNote *       mn;
  ArrangerObject * mn_obj;
  MidiEvent *      mev = &ev->midi_event;
  midi_byte_t *    buf = mev->raw_buffer;

  if (tr->type == TRACK_TYPE_CHORD)
    {
      if (midi_is_note_on (buf))
        {
          midi_byte_t             note_number = midi_get_note_number (buf);
          const ChordDescriptor * descr =
            chord_editor_get_chord_from_note_number (CHORD_EDITOR, note_number);
          g_return_if_fail (descr);
          int chord_idx = chord_editor_get_chord_index (CHORD_EDITOR, descr);
          ChordObject * co = chord_object_new (
            &region->id, chord_idx, region->num_chord_objects);
          chord_region_add_chord_object (region, co, F_PUBLISH_EVENTS);
          arranger_object_set_position (
            (ArrangerObject *) co, &local_pos,
            ARRANGER_OBJECT_POSITION_TYPE_START, F_NO_VALIDATE);
        }
    }
  /* else if not chord track */
  else
    {
      if (midi_is_note_on (buf))
        {
          g_return_if_fail (region);
          midi_region_start_unended_note (
            region, &local_pos, &local_end_pos, midi_get_note_number (buf),
            midi_get_velocity (buf), F_PUBLISH_EVENTS);
        }
      else if (midi_is_note_off (buf))
        {
          g_return_if_fail (region);
          mn = midi_region_pop_unended_note (region, midi_get_note_number (buf));
          if (mn)
            {
              mn_obj = (ArrangerObject *) mn;
              arranger_object_end_pos_setter (mn_obj, &local_end_pos);
            }
        }
      else
        {
          /* TODO */
        }
    }
}

/**
 * @note Runs in GTK thread only.
 */
static void
handle_automation_event (RecordingManager * self, RecordingEvent * ev)
{
  handle_resume_event (self, ev);

  Track * tr =
    tracklist_find_track_by_name_hash (TRACKLIST, ev->track_name_hash);
  AutomationTrack * at = tr->automation_tracklist.ats[ev->automation_track_idx];
  Port *            port = port_find_from_identifier (&at->port_id);
  float             value = port_get_control_value (port, false);
  float             normalized_value = port_get_control_value (port, true);
  if (ZRYTHM_TESTING)
    {
      math_assert_nonnann (value);
      math_assert_nonnann (normalized_value);
    }
  bool automation_value_changed =
    !port->value_changed_from_reading
    && !math_floats_equal (value, at->last_recorded_value);
  gint64 cur_time = g_get_monotonic_time ();

  /* get end position */
  unsigned_frame_t start_frames = ev->g_start_frame_w_offset;
  unsigned_frame_t end_frames = start_frames + ev->nframes;

  Position start_pos, end_pos;
  position_from_frames (&start_pos, (signed_frame_t) start_frames);
  position_from_frames (&end_pos, (signed_frame_t) end_frames);

  bool new_region_created = false;

  /* get the recording region */
  ZRegion * region = automation_track_get_region_before_pos (
    at, &start_pos, true, Z_F_NO_USE_SNAPSHOTS);
#if 0
  position_print (&start_pos);
  position_print (&end_pos);
  if (region)
    {
      arranger_object_print (
        (ArrangerObject *) region);
    }
  else
    {
      g_message ("no region");
    }
#endif

  ZRegion * region_at_end = automation_track_get_region_before_pos (
    at, &end_pos, true, Z_F_NO_USE_SNAPSHOTS);
  if (!region && automation_value_changed)
    {
      g_debug ("creating new automation region (automation value changed)");
      /* create region */
      Position pos_to_end_new_r;
      if (region_at_end)
        {
          ArrangerObject * r_at_end_obj = (ArrangerObject *) region_at_end;
          position_set_to_pos (&pos_to_end_new_r, &r_at_end_obj->pos);
        }
      else
        {
          position_set_to_pos (&pos_to_end_new_r, &end_pos);
        }
      region = automation_region_new (
        &start_pos, &pos_to_end_new_r, track_get_name_hash (tr), at->index,
        at->num_regions);
      new_region_created = true;
      g_return_if_fail (region);
      GError * err = NULL;
      bool     success = track_add_region (
        tr, region, at, -1, F_GEN_NAME, F_PUBLISH_EVENTS, &err);
      if (!success)
        {
          HANDLE_ERROR_LITERAL (err, "Failed to add region");
          return;
        }

      add_recorded_id (self, region);
    }

  at->recording_region = region;
  ArrangerObject * r_obj = (ArrangerObject *) region;

  if (
    new_region_created
    || (r_obj && position_is_before (&r_obj->end_pos, &end_pos)))
    {
      /* set region end pos */
      arranger_object_set_end_pos_full_size (r_obj, &end_pos);
    }

  at->recording_region = region;

  /* handle the samples normally */
  if (automation_value_changed)
    {
      create_automation_point (
        self, at, region, value, normalized_value, &start_pos);
      at->last_recorded_value = value;
    }
  else if (at->record_mode == AUTOMATION_RECORD_MODE_LATCH)
    {
      g_return_if_fail (region);
      delete_automation_points (self, at, region, &start_pos);
    }

  /* if we left touch mode, set last recorded ap
   * to NULL */
  if (
    at->record_mode == AUTOMATION_RECORD_MODE_TOUCH
    && !automation_track_should_be_recording (at, cur_time, true)
    && at->recording_region)
    {
      at->recording_region->last_recorded_ap = NULL;
    }
}

static void
handle_start_recording (
  RecordingManager * self,
  RecordingEvent *   ev,
  bool               is_automation)
{
  Track * tr =
    tracklist_find_track_by_name_hash (TRACKLIST, ev->track_name_hash);
  gint64            cur_time = g_get_monotonic_time ();
  AutomationTrack * at = NULL;
  if (is_automation)
    {
      at = tr->automation_tracklist.ats[ev->automation_track_idx];
    }

  if (self->num_active_recordings == 0)
    {
      self->selections_before_start =
        arranger_selections_clone ((ArrangerSelections *) TL_SELECTIONS);
    }

  /* this could be called multiple times, ignore
   * if already processed */
  if (tr->recording_region && !is_automation)
    {
      g_warning ("record start already processed");
      self->num_active_recordings++;
      return;
    }

  /* get end position */
  unsigned_frame_t start_frames = ev->g_start_frame_w_offset;
  unsigned_frame_t end_frames = start_frames + ev->nframes;

  g_message (
    "start %" UNSIGNED_FRAME_FORMAT
    ", "
    "end %" UNSIGNED_FRAME_FORMAT,
    start_frames, end_frames);

  /* this is not needed because the cycle is
   * already split */
#if 0
  /* adjust for transport loop end */
  if (transport_is_loop_point_met (
        TRANSPORT, start_frames, ev->nframes))
    {
      start_frames =
        TRANSPORT->loop_start_pos.frames;
      end_frames =
        (end_frames -
           TRANSPORT->loop_end_pos.frames) +
        start_frames;
    }
#endif
  g_return_if_fail (start_frames < end_frames);

  Position start_pos, end_pos;
  position_from_frames (&start_pos, (signed_frame_t) start_frames);
  position_from_frames (&end_pos, (signed_frame_t) end_frames);

  if (is_automation)
    {
      /* don't unset recording paused, this will be unset by handle_resume() */
      /*at->recording_paused = false;*/

      /* nothing, wait for event to start writing data */
      Port * port = port_find_from_identifier (&at->port_id);
      float  value = port_get_control_value (port, false);

      if (automation_track_should_be_recording (at, cur_time, true))
        {
          /* set recorded value to something else to force the recorder to start
           * writing */
          g_message ("SHOULD BE RECORDING");
          at->last_recorded_value = value + 2.f;
        }
      else
        {
          g_message ("SHOULD NOT BE RECORDING");
          /** set the current value so that
           * nothing is recorded until it changes */
          at->last_recorded_value = value;
        }
    }
  else
    {
      tr->recording_paused = false;

      if (track_type_has_piano_roll (tr->type))
        {
          /* create region */
          int       new_lane_pos = tr->num_lanes - 1;
          ZRegion * region = midi_region_new (
            &start_pos, &end_pos, track_get_name_hash (tr), new_lane_pos,
            tr->lanes[new_lane_pos]->num_regions);
          g_return_if_fail (region);
          GError * err = NULL;
          bool     success = track_add_region (
            tr, region, NULL, new_lane_pos, F_GEN_NAME, F_PUBLISH_EVENTS, &err);
          if (!success)
            {
              HANDLE_ERROR_LITERAL (err, "Failed to add region");
              return;
            }

          tr->recording_region = region;
          add_recorded_id (self, region);
        }
      else if (tr->type == TRACK_TYPE_CHORD)
        {
          ZRegion * region =
            chord_region_new (&start_pos, &end_pos, tr->num_chord_regions);
          g_return_if_fail (region);
          GError * err = NULL;
          bool     success = track_add_region (
            tr, region, NULL, -1, F_GEN_NAME, F_PUBLISH_EVENTS, &err);
          if (!success)
            {
              HANDLE_ERROR_LITERAL (err, "Failed to add region");
              return;
            }

          tr->recording_region = region;
          add_recorded_id (self, region);
        }
      else if (tr->type == TRACK_TYPE_AUDIO)
        {
          /* create region */
          int    new_lane_pos = tr->num_lanes - 1;
          char * name = audio_pool_gen_name_for_recording_clip (
            AUDIO_POOL, tr, new_lane_pos);
          ZRegion * region = audio_region_new (
            -1, NULL, true, NULL, ev->nframes, name, 2, BIT_DEPTH_32,
            &start_pos, track_get_name_hash (tr), new_lane_pos,
            tr->lanes[new_lane_pos]->num_regions, NULL);
          g_return_if_fail (region);
          GError * err = NULL;
          bool     success = track_add_region (
            tr, region, NULL, new_lane_pos, F_GEN_NAME, F_PUBLISH_EVENTS, &err);
          if (!success)
            {
              HANDLE_ERROR_LITERAL (err, "Failed to add region");
              return;
            }

          tr->recording_region = region;
          add_recorded_id (self, region);

#if 0
          g_message (
            "region start %ld, end %ld",
            region->base.pos.frames,
            region->base.end_pos.frames);
#endif
        }
    }

  self->num_active_recordings++;
}

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 *
 * It can also be called to process the events
 * immediately. Should only be called from the
 * GTK thread.
 */
int
recording_manager_process_events (RecordingManager * self)
{
  /*gint64 curr_time = g_get_monotonic_time ();*/
  /*g_message ("~~~~~~~~~~~~~~~~starting processing");*/
  zix_sem_wait (&self->processing_sem);
  /*int i = 0;*/
  g_return_val_if_fail (!self->currently_processing, G_SOURCE_REMOVE);
  self->currently_processing = true;
  RecordingEvent * ev;
  while (recording_event_queue_dequeue_event (self->event_queue, &ev))
    {
      if (self->freeing)
        {
          goto return_to_pool;
        }
      /*i++;*/

      /*g_message ("event type %d", ev->type);*/

      switch (ev->type)
        {
        case RECORDING_EVENT_TYPE_MIDI:
          /*g_message ("-------- RECORD MIDI");*/
          handle_midi_event (self, ev);
          break;
        case RECORDING_EVENT_TYPE_AUDIO:
          /*g_message ("-------- RECORD AUDIO");*/
          handle_audio_event (self, ev);
          break;
        case RECORDING_EVENT_TYPE_AUTOMATION:
          /*g_message ("-------- RECORD AUTOMATION");*/
          handle_automation_event (self, ev);
          break;
        case RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING:
          g_message ("-------- PAUSE TRACK RECORDING");
          handle_pause_event (self, ev);
          break;
        case RECORDING_EVENT_TYPE_PAUSE_AUTOMATION_RECORDING:
          g_message ("-------- PAUSE AUTOMATION RECORDING");
          handle_pause_event (self, ev);
          break;
        case RECORDING_EVENT_TYPE_STOP_TRACK_RECORDING:
          {
            Track * tr = tracklist_find_track_by_name_hash (
              TRACKLIST, ev->track_name_hash);
            g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr), G_SOURCE_CONTINUE);
            g_message ("-------- STOP TRACK RECORDING (%s)", tr->name);
            g_return_val_if_fail (tr, G_SOURCE_REMOVE);
            handle_stop_recording (self, false);
            tr->recording_region = NULL;
            tr->recording_start_sent = false;
            tr->recording_stop_sent = false;
          }
          g_message ("num active recordings: %d", self->num_active_recordings);
          break;
        case RECORDING_EVENT_TYPE_STOP_AUTOMATION_RECORDING:
          g_message ("-------- STOP AUTOMATION RECORDING");
          {
            Track * tr = tracklist_find_track_by_name_hash (
              TRACKLIST, ev->track_name_hash);
            AutomationTrack * at =
              tr->automation_tracklist.ats[ev->automation_track_idx];
            g_return_val_if_fail (at, G_SOURCE_REMOVE);
            if (at->recording_started)
              {
                handle_stop_recording (self, true);
              }
            at->recording_started = false;
            at->recording_start_sent = false;
            at->recording_region = NULL;
          }
          g_message ("num active recordings: %d", self->num_active_recordings);
          break;
        case RECORDING_EVENT_TYPE_START_TRACK_RECORDING:
          {
            Track * tr = tracklist_find_track_by_name_hash (
              TRACKLIST, ev->track_name_hash);
            g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr), G_SOURCE_CONTINUE);
            g_message ("-------- START TRACK RECORDING (%s)", tr->name);
            handle_start_recording (self, ev, false);
            g_message ("num active recordings: %d", self->num_active_recordings);
          }
          break;
        case RECORDING_EVENT_TYPE_START_AUTOMATION_RECORDING:
          g_message ("-------- START AUTOMATION RECORDING");
          {
            Track * tr = tracklist_find_track_by_name_hash (
              TRACKLIST, ev->track_name_hash);
            AutomationTrack * at =
              tr->automation_tracklist.ats[ev->automation_track_idx];
            g_return_val_if_fail (at, G_SOURCE_REMOVE);
            if (!at->recording_started)
              {
                handle_start_recording (self, ev, true);
              }
            at->recording_started = true;
          }
          g_message ("num active recordings: %d", self->num_active_recordings);
          break;
        default:
          g_warning ("recording event %d not implemented yet", ev->type);
          break;
        }

      /*UP_RETURNED (ev);*/
return_to_pool:
      object_pool_return (self->event_obj_pool, ev);
    }
  /*g_message ("~~~~~~~~~~~processed %d events", i);*/

  self->currently_processing = false;
  zix_sem_post (&self->processing_sem);

  return G_SOURCE_CONTINUE;
}

/**
 * Creates the event queue and starts the event loop.
 *
 * Must be called from a GTK thread.
 */
RecordingManager *
recording_manager_new (void)
{
  RecordingManager * self = object_new (RecordingManager);

  self->pending_aps = g_ptr_array_new ();

  const size_t max_events = 10000;
  self->event_obj_pool = object_pool_new (
    (ObjectCreatorFunc) recording_event_new,
    (ObjectFreeFunc) recording_event_free, (int) max_events);
  self->event_queue = mpmc_queue_new ();
  mpmc_queue_reserve (self->event_queue, max_events);

  zix_sem_init (&self->processing_sem, 1);
  self->source_id =
    g_timeout_add (12, (GSourceFunc) recording_manager_process_events, self);

  return self;
}

void
recording_manager_free (RecordingManager * self)
{
  g_message ("%s: Freeing...", __func__);

  self->freeing = true;

  /* stop source func */
  g_source_remove_and_zero (self->source_id);

  /* process pending events */
  recording_manager_process_events (self);

  /* free objects */
  object_free_w_func_and_null (mpmc_queue_free, self->event_queue);
  object_free_w_func_and_null (object_pool_free, self->event_obj_pool);

  free_temp_selections (self);

  object_free_w_func_and_null (g_ptr_array_unref, self->pending_aps);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
