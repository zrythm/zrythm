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

#include "actions/arranger_selections.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/clip.h"
#include "audio/control_port.h"
#include "audio/engine.h"
#include "audio/recording_event.h"
#include "audio/recording_manager.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/arranger_object.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <gtk/gtk.h>

/**
 * Adds the region's identifier to the recorded
 * identifiers (to be used for creating the undoable
 * action when recording stops.
 */
static void
add_recorded_id (
  RecordingManager * self,
  ZRegion *          region)
{
  /*region_identifier_print (&region->id);*/
  region_identifier_copy (
    &self->recorded_ids[self->num_recorded_ids],
    &region->id);
  self->num_recorded_ids++;
}

static void
free_temp_selections (
  RecordingManager * self)
{
  if (self->selections_before_start_track)
    {
      object_free_w_func_and_null (
        arranger_selections_free,
        self->selections_before_start_track);
    }
  if (self->selections_before_start_automation)
    {
      object_free_w_func_and_null (
        arranger_selections_free,
        self->selections_before_start_automation);
    }
}

static void
on_stop_recording (
  RecordingManager * self,
  bool               is_automation)
{
  g_message (
    "%s%s", "----- stopped recording",
    is_automation ? " (automation)" : "");

  /* cache the current selections */
  ArrangerSelections * prev_selections =
    arranger_selections_clone (
      (ArrangerSelections *) TL_SELECTIONS);

  /* select all the recorded regions */
  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS);
  for (int i = 0; i < self->num_recorded_ids; i++)
    {
      RegionIdentifier * id = &self->recorded_ids[i];

      if ((is_automation &&
           id->type != REGION_TYPE_AUTOMATION) ||
         (!is_automation &&
           id->type == REGION_TYPE_AUTOMATION))
        continue;

      /* do some sanity checks for lane regions */
      if (region_type_has_lane (id->type))
        {
          g_return_if_fail (
            id->track_pos < TRACKLIST->num_tracks);
          Track * track =
            TRACKLIST->tracks[id->track_pos];
          g_return_if_fail (track);

          g_return_if_fail (
            id->lane_pos < track->num_lanes);
          TrackLane * lane =
            track->lanes[id->lane_pos];
          g_return_if_fail (lane);

          g_return_if_fail (
            id->idx <= lane->num_regions);
        }

      /*region_identifier_print (id);*/
      ZRegion * region = region_find (id);
      g_return_if_fail (region);
      arranger_selections_add_object (
        (ArrangerSelections *) TL_SELECTIONS,
        (ArrangerObject *) region);

      if (is_automation)
        {
          region->last_recorded_ap = NULL;
        }
    }

  /* perform the create action */
  UndoableAction * action =
    arranger_selections_action_new_record (
      is_automation ?
        self->selections_before_start_automation :
        self->selections_before_start_track,
      (ArrangerSelections *) TL_SELECTIONS, true);
  undo_manager_perform (UNDO_MANAGER, action);

  /* update frame caches and write audio clips to
   * pool */
  for (int i = 0; i < self->num_recorded_ids; i++)
    {
      ZRegion * r =
        region_find (&self->recorded_ids[i]);
      if (r->id.type == REGION_TYPE_AUDIO)
        {
          AudioClip * clip =
            audio_region_get_clip (r);
          audio_region_init_frame_caches (
            r, clip);
          audio_clip_write_to_pool (clip);
        }
    }

  /* restore the selections */
  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS);
  int num_objs;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      prev_selections, &num_objs);
  for (int i = 0; i < num_objs; i++)
    {
      ArrangerObject * obj =
        arranger_object_find (objs[i]);
      g_return_if_fail (obj);
      arranger_object_select (
        obj, F_SELECT, F_APPEND);
    }

  /* free the temporary selections */
  free_temp_selections (self);
}

/**
 * Handles the recording logic inside the process
 * cycle.
 *
 * The MidiEvents are already dequeued at this
 * point.
 *
 * @param g_frames_start Global start frames.
 * @param nframes Number of frames to process.
 */
void
recording_manager_handle_recording (
  RecordingManager * self,
  TrackProcessor * track_processor,
  const long       g_start_frames,
  const nframes_t  local_offset,
  const nframes_t  nframes)
{
  Track * tr =
    track_processor_get_track (track_processor);
  AutomationTracklist * atl =
    track_get_automation_tracklist (tr);
  gint64 cur_time = g_get_monotonic_time ();
  bool stop_recording = false;

  if (!TRANSPORT->recording ||
      !tr->recording ||
      !TRANSPORT_IS_ROLLING)
    {
      if (track_type_can_record (tr->type) &&
          tr->recording_region)
        {
          /* send stop recording event */
          RecordingEvent * re =
            (RecordingEvent *)
            object_pool_get (
              RECORDING_MANAGER->event_obj_pool);
          re->type =
            RECORDING_EVENT_TYPE_STOP_TRACK_RECORDING;
          re->g_start_frames = g_start_frames;
          re->local_offset = local_offset;
          re->nframes = nframes;
          strcpy (re->track_name, tr->name);
          recording_event_queue_push_back_event (
            RECORDING_MANAGER->event_queue, re);
        }
      stop_recording = true;
    }
  else
    {
      if (track_type_can_record (tr->type) &&
          !tr->recording_region)
        {
          /* send start recording event */
          RecordingEvent * re =
            (RecordingEvent *)
            object_pool_get (
              RECORDING_MANAGER->event_obj_pool);
          re->type =
            RECORDING_EVENT_TYPE_START_TRACK_RECORDING;
          re->g_start_frames = g_start_frames;
          re->local_offset = local_offset;
          re->nframes = nframes;
          strcpy (re->track_name, tr->name);
          recording_event_queue_push_back_event (
            RECORDING_MANAGER->event_queue, re);
        }
    }

  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];
      if ((!TRANSPORT_IS_ROLLING ||
           !automation_track_should_be_recording (
             at, cur_time, false)) &&
           at->recording_started)
        {
          /* send stop automation recording event */
          RecordingEvent * re =
            (RecordingEvent *)
            object_pool_get (
              RECORDING_MANAGER->event_obj_pool);
          re->type =
            RECORDING_EVENT_TYPE_STOP_AUTOMATION_RECORDING;
          re->g_start_frames = g_start_frames;
          re->local_offset = local_offset;
          re->nframes = nframes;
          port_identifier_copy (
            &re->port_id, &at->port_id);
          strcpy (re->track_name, tr->name);
          recording_event_queue_push_back_event (
            RECORDING_MANAGER->event_queue, re);
        }
      if (TRANSPORT_IS_ROLLING &&
          automation_track_should_be_recording (
            at, cur_time, false))
        {
          if (!at->recording_started)
            {
              /* send start recording event */
              RecordingEvent * re =
                (RecordingEvent *)
                object_pool_get (
                  RECORDING_MANAGER->event_obj_pool);
              re->type =
                RECORDING_EVENT_TYPE_START_AUTOMATION_RECORDING;
              re->g_start_frames = g_start_frames;
              re->local_offset = local_offset;
              re->nframes = nframes;
              port_identifier_copy (
                &re->port_id, &at->port_id);
              strcpy (re->track_name, tr->name);
              recording_event_queue_push_back_event (
                RECORDING_MANAGER->event_queue, re);
            }

          /* add recorded automation material to
           * event queue */
          RecordingEvent * re =
            (RecordingEvent *)
            object_pool_get (
              RECORDING_MANAGER->event_obj_pool);
          re->type =
            RECORDING_EVENT_TYPE_AUTOMATION;
          re->g_start_frames = g_start_frames;
          re->local_offset = local_offset;
          re->nframes = nframes;
          strcpy (re->track_name, tr->name);
          port_identifier_copy (
            &re->port_id, &at->port_id);
          recording_event_queue_push_back_event (
            RECORDING_MANAGER->event_queue, re);
        }
    }

  if (stop_recording)
    return;

  /* add recorded track material to event queue */

  if (track_has_piano_roll (tr))
    {
      MidiEvents * midi_events =
        track_processor->midi_in->midi_events;

      for (int i = 0;
           i < midi_events->num_events; i++)
        {
          MidiEvent * me =
            &midi_events->events[i];

          RecordingEvent * re =
            (RecordingEvent *)
            object_pool_get (
              RECORDING_MANAGER->event_obj_pool);
          re->type = RECORDING_EVENT_TYPE_MIDI;
          re->g_start_frames = g_start_frames;
          re->local_offset = local_offset;
          re->nframes = nframes;
          re->has_midi_event = 1;
          midi_event_copy (me, &re->midi_event);
          strcpy (re->track_name, tr->name);
          recording_event_queue_push_back_event (
            RECORDING_MANAGER->event_queue, re);
        }

      if (midi_events->num_events == 0)
        {
          RecordingEvent * re =
            (RecordingEvent *)
            object_pool_get (
              RECORDING_MANAGER->event_obj_pool);
          re->type = RECORDING_EVENT_TYPE_MIDI;
          re->g_start_frames = g_start_frames;
          re->local_offset = local_offset;
          re->nframes = nframes;
          re->has_midi_event = 0;
          strcpy (re->track_name, tr->name);
          recording_event_queue_push_back_event (
            RECORDING_MANAGER->event_queue, re);
        }
    }
  else if (tr->type == TRACK_TYPE_AUDIO)
    {
      RecordingEvent * re =
        (RecordingEvent *)
        object_pool_get (
          RECORDING_MANAGER->event_obj_pool);
      re->type = RECORDING_EVENT_TYPE_AUDIO;
      re->g_start_frames = g_start_frames;
      re->local_offset = local_offset;
      re->nframes = nframes;
      memcpy (
        re->lbuf,
        track_processor->stereo_in->l->buf,
        sizeof (float) *
          (size_t) AUDIO_ENGINE->block_length);
      memcpy (
        re->rbuf,
        track_processor->stereo_in->r->buf,
        sizeof (float) *
          (size_t) AUDIO_ENGINE->block_length);
      strcpy (re->track_name, tr->name);
      recording_event_queue_push_back_event (
        RECORDING_MANAGER->event_queue, re);
    }
}

static void
handle_audio_event (
  RecordingEvent * ev)
{
  long g_start_frames = ev->g_start_frames;
  nframes_t nframes = ev->nframes;
  nframes_t local_offset = ev->local_offset;
  Track * tr = track_get_from_name (ev->track_name);

  /* get end position */
  long start_frames = g_start_frames;
  long end_frames =
    g_start_frames + (long) nframes;

  /* adjust for transport loop end */
  int loop_met = 0;
  nframes_t frames_till_loop = 0;
  if ((frames_till_loop =
         transport_is_loop_point_met (
           TRANSPORT, g_start_frames, nframes)))
    {
      loop_met = 1;
      start_frames =
        TRANSPORT->loop_start_pos.frames;
      end_frames =
        (end_frames -
           TRANSPORT->loop_end_pos.frames) +
        start_frames;
    }

  Position start_pos, end_pos;
  position_from_frames (
    &start_pos, start_frames);
  position_from_frames (
    &end_pos, end_frames);

  /* get the recording region */
  ZRegion * region = tr->recording_region;
  g_return_if_fail (region);
  ArrangerObject * r_obj =
    (ArrangerObject *) region;

  /* the region before the loop point, if
   * loop point is met */
  ZRegion * region_before_loop_end = NULL;
  ArrangerObject * r_obj_before_loop_end;

  /* the clip */
  AudioClip * clip = NULL;
  AudioClip * clip_before_loop_end = NULL;

  clip = audio_region_get_clip (region);

  if (loop_met)
    {
      region_before_loop_end = region;
      r_obj_before_loop_end =
        (ArrangerObject *)
        region;
      clip_before_loop_end = clip;

      /* set current region end pos  to
       * transport loop end */
      arranger_object_end_pos_setter (
        r_obj, &TRANSPORT->loop_end_pos);
      r_obj->end_pos.frames =
        TRANSPORT->loop_end_pos.frames;

      clip->num_frames =
        r_obj->end_pos.frames -
        r_obj->pos.frames;
      clip->frames =
        (sample_t *) realloc (
          clip->frames,
          (size_t)
          (clip->num_frames *
             (long) clip->channels) *
          sizeof (sample_t));
      region->frames =
        (sample_t *) realloc (
          region->frames,
          (size_t)
          (clip->num_frames *
             (long) clip->channels) *
          sizeof (sample_t));
      region->num_frames = (size_t) clip->num_frames;
      memcpy (
        &region->frames[0], &clip->frames[0],
        sizeof (float) * (size_t) clip->num_frames *
        clip->channels);

      position_from_frames (
        &r_obj->loop_end_pos,
        r_obj->end_pos.frames -
          r_obj->pos.frames);

      /* start new region in new lane at
       * TRANSPORT loop start */
      int new_lane_pos = region->id.lane_pos + 1;
      ZRegion * new_region =
        audio_region_new (
          -1, NULL, NULL, (long) nframes, 2,
          &TRANSPORT->loop_start_pos, tr->pos,
          new_lane_pos,
          tr->num_lanes > new_lane_pos ?
            tr->lanes[new_lane_pos]->num_regions :
            0);
      track_add_region (
        tr, new_region, NULL,
        new_lane_pos, F_GEN_NAME,
        F_PUBLISH_EVENTS);
      region = new_region;
      add_recorded_id (
        RECORDING_MANAGER, new_region);

      clip =
        audio_region_get_clip (region);
    }
  else /* loop not met */
    {
      /* set region end pos */
      arranger_object_end_pos_setter (
        r_obj, &end_pos);
      r_obj->end_pos.frames =
        end_pos.frames;

      clip->num_frames =
        r_obj->end_pos.frames -
        r_obj->pos.frames;
      clip->frames =
        (sample_t *) realloc (
        clip->frames,
        (size_t)
        (clip->num_frames *
           (long) clip->channels) *
        sizeof (sample_t));
      region->frames =
        (sample_t *) realloc (
          region->frames,
          (size_t)
          (clip->num_frames *
             (long) clip->channels) *
          sizeof (sample_t));
      region->num_frames = (size_t) clip->num_frames;
      memcpy (
        &region->frames[0], &clip->frames[0],
        sizeof (float) * (size_t) clip->num_frames *
        clip->channels);

      position_from_frames (
        &r_obj->loop_end_pos,
        r_obj->end_pos.frames -
          r_obj->pos.frames);
    }
  r_obj->fade_out_pos = r_obj->loop_end_pos;

  tr->recording_region = region;

  if (loop_met)
    {
      /* handle the samples until loop end */
      if (region_before_loop_end)
        {
          long clip_offset_before_loop =
            g_start_frames -
            r_obj_before_loop_end->
              pos.frames;
          for (
            nframes_t i =
              local_offset;
            i <
              local_offset +
                frames_till_loop;
            i++)
            {
              g_warn_if_fail (
                clip_offset_before_loop >= 0 &&
                clip_offset_before_loop <
                  clip_before_loop_end->
                    num_frames);
              g_warn_if_fail (
                i >= local_offset &&
                i < local_offset + nframes);
              clip_before_loop_end->frames[
                clip_before_loop_end->channels *
                  clip_offset_before_loop] =
                    ev->lbuf[i];
              clip_before_loop_end->frames[
                clip_before_loop_end->channels *
                  (clip_offset_before_loop++)] =
                    ev->rbuf[i];
            }
        }

      /* handle samples after loop start */
      long clip_offset = 0;
      for (
        nframes_t i =
          nframes -
            (local_offset +
              frames_till_loop);
        i < nframes;
        i++)
        {
          g_warn_if_fail (
            clip_offset >= 0 &&
            clip_offset <
              clip->num_frames);
          g_warn_if_fail (
            i >= local_offset &&
            i < local_offset + nframes);
          clip->frames[
            clip->channels * clip_offset] =
              ev->lbuf[i];
          clip->frames[
            clip->channels * (clip_offset++)] =
              ev->rbuf[i];
        }
    }

  /* handle the samples normally */
  nframes_t cur_local_offset =
    local_offset;
  g_return_if_fail (region);
  r_obj =
    (ArrangerObject *) region;
  for (long i =
         start_frames -
           r_obj->pos.frames;
       i <
         end_frames -
           r_obj->pos.frames; i++)
    {
      g_warn_if_fail (
        i >= 0 &&
        i < clip->num_frames);
      g_warn_if_fail (
        cur_local_offset >= local_offset &&
        cur_local_offset <
          local_offset + nframes);
      clip->frames[
        i * clip->channels] =
          ev->lbuf[cur_local_offset];
      clip->frames[
        i * clip->channels + 1] =
          ev->rbuf[cur_local_offset++];
    }
}

static void
handle_midi_event (
  RecordingEvent * ev)
{
  long g_start_frames = ev->g_start_frames;
  nframes_t nframes = ev->nframes;
  Track * tr = track_get_from_name (ev->track_name);

  /* get end position */
  long start_frames = g_start_frames;
  long end_frames =
    g_start_frames + (long) nframes;

  /* adjust for transport loop end */
  int loop_met = 0;
  nframes_t frames_till_loop = 0;
  if ((frames_till_loop =
         transport_is_loop_point_met (
           TRANSPORT, g_start_frames, nframes)))
    {
      loop_met = 1;
      start_frames =
        TRANSPORT->loop_start_pos.frames;
      end_frames =
        (end_frames -
           TRANSPORT->loop_end_pos.frames) +
        start_frames;
    }

  Position start_pos, end_pos;
  position_from_frames (
    &start_pos, start_frames);
  position_from_frames (
    &end_pos, end_frames);

  /* get the recording region */
  ZRegion * region = tr->recording_region;
  g_return_if_fail (region);
  ArrangerObject * r_obj =
    (ArrangerObject *) region;

  /* the region before the loop point, if
   * loop point is met */
  ZRegion * region_before_loop_end = NULL;

  if (loop_met)
    {
      region_before_loop_end = region;

      /* set current region end pos  to
       * transport loop end */
      arranger_object_end_pos_setter (
        r_obj, &TRANSPORT->loop_end_pos);
      r_obj->end_pos.frames =
        TRANSPORT->loop_end_pos.frames;
      arranger_object_loop_end_pos_setter (
        r_obj, &TRANSPORT->loop_end_pos);
      r_obj->loop_end_pos.frames =
        TRANSPORT->loop_end_pos.frames;

      /* start new region in new lane at
       * TRANSPORT loop start */
      int new_lane_pos = region->id.lane_pos + 1;
      ZRegion * new_region =
        midi_region_new (
          &TRANSPORT->loop_start_pos,
          &end_pos, tr->pos, new_lane_pos,
          tr->num_lanes > new_lane_pos ?
            tr->lanes[new_lane_pos]->num_regions :
            0);
      track_add_region (
        tr, new_region, NULL,
        new_lane_pos, F_GEN_NAME,
        F_PUBLISH_EVENTS);
      add_recorded_id (
        RECORDING_MANAGER, new_region);
      region = new_region;
    }
  else /* loop not met */
    {
      /* set region end pos */
      arranger_object_end_pos_setter (
        r_obj, &end_pos);
      r_obj->end_pos.frames =
        end_pos.frames;
      arranger_object_loop_end_pos_setter (
        r_obj, &end_pos);
      r_obj->loop_end_pos.frames =
        end_pos.frames;
    }

  tr->recording_region = region;

  MidiNote * mn;
  ArrangerObject * mn_obj;

  /* add midi note off if loop met */
  if (loop_met)
    {
      while (
        (mn =
          midi_region_pop_unended_note (
            region_before_loop_end, -1)))
        {
          mn_obj =
            (ArrangerObject *) mn;
          Position local_end_pos;
          position_set_to_pos (
            &local_end_pos,
            &TRANSPORT->loop_end_pos);
          position_add_ticks (
            &local_end_pos,
            - ((ArrangerObject *)
            region_before_loop_end)->pos.
              total_ticks);
          arranger_object_end_pos_setter (
            mn_obj, &local_end_pos);
        }
    }

  if (!ev->has_midi_event)
    return;

  /* get local positions */
  Position local_pos, local_end_pos;
  position_set_to_pos (
    &local_pos, &start_pos);
  position_set_to_pos (
    &local_end_pos, &end_pos);
  position_add_ticks (
    &local_pos,
    - ((ArrangerObject *) region)->pos.
      total_ticks);
  position_add_ticks (
    &local_end_pos,
    - ((ArrangerObject *) region)->pos.
      total_ticks);

  /* convert MIDI data to midi notes */
  MidiEvent * mev = &ev->midi_event;
  switch (mev->type)
    {
      case MIDI_EVENT_TYPE_NOTE_ON:
        g_return_if_fail (region);
        midi_region_start_unended_note (
          region, &local_pos, &local_end_pos,
          mev->note_pitch, mev->velocity, 1);
        break;
      case MIDI_EVENT_TYPE_NOTE_OFF:
        g_return_if_fail (region);
        mn =
          midi_region_pop_unended_note (
            region, mev->note_pitch);
        if (mn)
          {
            mn_obj =
              (ArrangerObject *) mn;
            arranger_object_end_pos_setter (
              mn_obj, &local_end_pos);
          }
        break;
      default:
        /* TODO */
        break;
    }
}

/**
 * Delete automation points since the last recorded
 * automation point and the current position (eg,
 * when in latch mode) if the position is after
 * the last recorded ap.
 */
static void
delete_automation_points (
  AutomationTrack * at,
  ZRegion *         region,
  Position *        pos)
{
  AutomationPoint * aps[100];
  int               num_aps = 0;
  automation_region_get_aps_since_last_recorded (
    region, pos, aps, &num_aps);
  for (int i = 0; i < num_aps; i++)
    {
      AutomationPoint * ap = aps[i];
      automation_region_remove_ap (
        region, ap, true);
    }

  /* create a new automation point at the pos with
   * the previous value */
  if (region->last_recorded_ap)
    {
      /* remove the last recorded AP if its
       * previous AP also has the same value */
      AutomationPoint * ap_before_recorded =
        automation_region_get_prev_ap (
          region, region->last_recorded_ap);
      if (ap_before_recorded &&
          math_floats_equal (
            ap_before_recorded->fvalue,
            region->last_recorded_ap->fvalue))
        {
          automation_region_remove_ap (
            region, region->last_recorded_ap, true);
        }

      ArrangerObject * r_obj =
        (ArrangerObject *) region;
      Position adj_pos;
      position_set_to_pos (&adj_pos, pos);
      position_add_ticks (
        &adj_pos, - r_obj->pos.total_ticks);
      AutomationPoint * ap =
        automation_point_new_float (
          region->last_recorded_ap->fvalue,
          region->last_recorded_ap->normalized_val,
          &adj_pos);
      automation_region_add_ap (
        region, ap, true);
      region->last_recorded_ap = ap;
    }
}

/**
 * Creates a new automation point and deletes
 * anything between the last recorded automation
 * point and this point.
 */
static AutomationPoint *
create_automation_point (
  AutomationTrack * at,
  ZRegion *         region,
  float             val,
  float             normalized_val,
  Position *        pos)
{
  AutomationPoint * aps[100];
  int               num_aps = 0;
  automation_region_get_aps_since_last_recorded (
    region, pos, aps, &num_aps);
  for (int i = 0; i < num_aps; i++)
    {
      AutomationPoint * ap = aps[i];
      automation_region_remove_ap (
        region, ap, true);
    }

  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  Position adj_pos;
  position_set_to_pos (&adj_pos, pos);
  position_add_ticks (
    &adj_pos, - r_obj->pos.total_ticks);
  AutomationPoint * ap =
    automation_point_new_float (
      val, normalized_val, &adj_pos);
  automation_region_add_ap (
    region, ap, true);
  region->last_recorded_ap = ap;

  return ap;
}

static void
handle_automation_event (
  RecordingEvent * ev)
{
  long g_start_frames = ev->g_start_frames;
  nframes_t nframes = ev->nframes;
  /*nframes_t local_offset = ev->local_offset;*/
  Track * tr = track_get_from_name (ev->track_name);
  AutomationTrack * at =
    automation_track_find_from_port_id (
      &ev->port_id, false);
  Port * port =
    automation_track_get_port (at);
  float value =
    port_get_control_value (port, false);
  float normalized_value =
    port_get_control_value (port, true);
  bool automation_value_changed =
    !port->value_changed_from_reading &&
    !math_floats_equal (
      value, at->last_recorded_value);
  gint64 cur_time = g_get_monotonic_time ();

  /* get end position */
  long start_frames = g_start_frames;
  long end_frames =
    g_start_frames + (long) nframes;

  /* adjust for transport loop end */
  int loop_met = 0;
  nframes_t frames_till_loop = 0;
  if ((frames_till_loop =
         transport_is_loop_point_met (
           TRANSPORT, g_start_frames, nframes)))
    {
      loop_met = 1;
      start_frames =
        TRANSPORT->loop_start_pos.frames;
      end_frames =
        (end_frames -
           TRANSPORT->loop_end_pos.frames) +
        start_frames;
    }

  Position start_pos, end_pos;
  position_from_frames (
    &start_pos, start_frames);
  position_from_frames (
    &end_pos, end_frames);

  bool new_region_created = false;

  /* get the recording region */
  ZRegion * region =
    automation_track_get_region_before_pos (
      at, &start_pos);
  ZRegion * region_at_end = NULL;
  if (!loop_met)
    region_at_end =
      automation_track_get_region_before_pos (
        at, &end_pos);
  if (!region && automation_value_changed)
    {
      /* create region */
      Position pos_to_end_new_r;
      if (region_at_end)
        {
          ArrangerObject * r_at_end_obj =
            (ArrangerObject *) region_at_end;
          position_set_to_pos (
            &pos_to_end_new_r, &r_at_end_obj->pos);
        }
      else if (loop_met)
        {
          position_set_to_pos (
            &pos_to_end_new_r,
            &TRANSPORT->loop_end_pos);
        }
      else
        {
          position_set_to_pos (
            &pos_to_end_new_r, &end_pos);
        }
      region =
        automation_region_new (
          &start_pos, &pos_to_end_new_r, tr->pos,
          at->index, at->num_regions);
      new_region_created = true;
      g_return_if_fail (region);
      track_add_region (
        tr, region, at, -1,
        F_GEN_NAME, F_PUBLISH_EVENTS);

      add_recorded_id (
        RECORDING_MANAGER, region);
    }

  at->recording_region = region;
  ArrangerObject * r_obj =
    (ArrangerObject *) region;

  /* the region before the loop point, if
   * loop point is met */
  ZRegion * region_before_loop_end = NULL;

  if (loop_met)
    {
      region_before_loop_end = region;

      if (region)
        {
          /* set current region end pos to
           * transport loop end */
          arranger_object_end_pos_setter (
            r_obj, &TRANSPORT->loop_end_pos);
          r_obj->end_pos.frames =
            TRANSPORT->loop_end_pos.frames;

          position_from_frames (
            &r_obj->loop_end_pos,
            r_obj->end_pos.frames -
              r_obj->pos.frames);
        }

      /* get or start new region at
       * TRANSPORT loop start */
      ZRegion * new_region =
        automation_track_get_region_before_pos (
          at, &TRANSPORT->loop_start_pos);
      region_at_end =
        automation_track_get_region_before_pos (
          at, &end_pos);
      if (!new_region &&
          automation_track_should_be_recording (
            at, cur_time, false))
        {
          /* create region */
          Position pos_to_end_new_r;
          if (region_at_end)
            {
              ArrangerObject * r_at_end_obj =
                (ArrangerObject *) region_at_end;
              position_set_to_pos (
                &pos_to_end_new_r,
                &r_at_end_obj->pos);
            }
          else
            {
              position_set_to_pos (
                &pos_to_end_new_r, &end_pos);
            }
          new_region =
            automation_region_new (
              &TRANSPORT->loop_start_pos,
              &end_pos, tr->pos,
              at->index, at->num_regions);
          g_return_if_fail (new_region);
          track_add_region (
            tr, new_region, at, -1,
            F_GEN_NAME, F_PUBLISH_EVENTS);
        }
      region = new_region;
      if (region)
        {
          add_recorded_id (
            RECORDING_MANAGER, new_region);
        }
    }
  else /* loop not met */
    {
      if (new_region_created ||
          (r_obj &&
           position_is_before (
             &r_obj->end_pos, &end_pos)))
        {
          /* set region end pos */
          arranger_object_end_pos_setter (
            r_obj, &end_pos);
          r_obj->end_pos.frames =
            end_pos.frames;

          position_from_frames (
            &r_obj->loop_end_pos,
            r_obj->end_pos.frames -
              r_obj->pos.frames);
        }
    }

  at->recording_region = region;

  if (loop_met)
    {
      /* handle the samples until loop end */
      if (region_before_loop_end)
        {
          if (automation_value_changed)
            {
              create_automation_point (
                at, region_before_loop_end,
                value, normalized_value,
                &start_pos);
              at->last_recorded_value = value;
            }
        }

      if (automation_track_should_be_recording (
            at, cur_time, true))
        {
          while (position_is_equal (
                   &((ArrangerObject *)
                      region->aps[0])->pos,
                   &TRANSPORT->loop_start_pos))
            {
              automation_region_remove_ap (
                region, region->aps[0], true);
            }

          /* create/replace ap at loop start */
          create_automation_point (
            at, region, value, normalized_value,
            &TRANSPORT->loop_start_pos);
        }
    }
  else
    {
      /* handle the samples normally */
      if (automation_value_changed)
        {
          create_automation_point (
            at, region,
            value, normalized_value,
            &start_pos);
          at->last_recorded_value = value;
        }
      else if (at->record_mode ==
                 AUTOMATION_RECORD_MODE_LATCH)
        {
          delete_automation_points (
            at, region, &start_pos);
        }
    }

  /* if we left touch mode, set last recorded ap
   * to NULL */
  if (at->record_mode ==
        AUTOMATION_RECORD_MODE_TOUCH &&
      !automation_track_should_be_recording (
        at, cur_time, true))
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
  Track * tr = track_get_from_name (ev->track_name);
  gint64 cur_time = g_get_monotonic_time ();
  AutomationTrack * at = NULL;
  if (is_automation)
    {
      at =
        automation_track_find_from_port_id (
          &ev->port_id, false);
      self->selections_before_start_automation =
          arranger_selections_clone (
            (ArrangerSelections *) TL_SELECTIONS);
    }
  else
    {
      self->selections_before_start_track =
          arranger_selections_clone (
            (ArrangerSelections *) TL_SELECTIONS);
    }

  /* this could be called multiple times, ignore
   * if already processed */
  if (tr->recording_region && !is_automation)
    {
      g_message ("record start already processed");
      return;
    }

  /* get end position */
  long start_frames = ev->g_start_frames;
  long end_frames =
    ev->g_start_frames + (long) ev->nframes;

  /* adjust for transport loop end */
  if (transport_is_loop_point_met (
        TRANSPORT, ev->g_start_frames,
        ev->nframes))
    {
      start_frames =
        TRANSPORT->loop_start_pos.frames;
      end_frames =
        (end_frames -
           TRANSPORT->loop_end_pos.frames) +
        start_frames;
    }

  Position start_pos, end_pos;
  position_from_frames (
    &start_pos, start_frames);
  position_from_frames (
    &end_pos, end_frames);

  if (is_automation)
    {
      /* nothing, wait for event to start
       * writing data */
      Port * port =
        automation_track_get_port (at);
      float value =
        port_get_control_value (port, false);

      if (automation_track_should_be_recording (
            at, cur_time, true))
        {
          /* set recorded value to something else
           * to force the recorder to start
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
      if (track_has_piano_roll (tr))
        {
          /* create region */
          int new_lane_pos = tr->num_lanes - 1;
          ZRegion * region =
            midi_region_new (
              &start_pos, &end_pos, tr->pos,
              new_lane_pos,
              tr->lanes[new_lane_pos]->num_regions);
          g_return_if_fail (region);
          track_add_region (
            tr, region, NULL, new_lane_pos,
            F_GEN_NAME, F_PUBLISH_EVENTS);

          tr->recording_region = region;
          add_recorded_id (RECORDING_MANAGER, region);
        }
      else if (tr->type == TRACK_TYPE_AUDIO)
        {
          /* create region */
          int new_lane_pos = tr->num_lanes - 1;
          ZRegion * region =
            audio_region_new (
              -1, NULL, NULL, ev->nframes, 2,
              &start_pos, tr->pos, new_lane_pos,
              tr->lanes[new_lane_pos]->num_regions);
          g_return_if_fail (region);
          track_add_region (
            tr, region, NULL, new_lane_pos,
            F_GEN_NAME, F_PUBLISH_EVENTS);

          tr->recording_region = region;
          add_recorded_id (RECORDING_MANAGER, region);
        }
    }
}

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 */
static int
events_process (
  RecordingManager * self)
{
  /*gint64 curr_time = g_get_monotonic_time ();*/
  /*g_message ("starting processing");*/
  RecordingEvent * ev;
  while (recording_event_queue_dequeue_event (
           self->event_queue, &ev))
    {
      if (ev->type < 0)
        {
          g_warn_if_reached ();
          continue;
        }

      /*g_message ("event type %d", ev->type);*/

      switch (ev->type)
        {
        case RECORDING_EVENT_TYPE_MIDI:
          /*g_message ("-------- RECORD MIDI");*/
          handle_midi_event (ev);
          break;
        case RECORDING_EVENT_TYPE_AUDIO:
          /*g_message ("-------- RECORD AUDIO");*/
          handle_audio_event (ev);
          break;
        case RECORDING_EVENT_TYPE_AUTOMATION:
          g_message ("-------- RECORD AUTOMATION");
          handle_automation_event (ev);
          break;
        case RECORDING_EVENT_TYPE_STOP_TRACK_RECORDING:
          g_message ("-------- STOP TRACK RECORDING");
          {
            Track * track =
              track_get_from_name (ev->track_name);
            g_warn_if_fail (track);
            if (self->is_recording)
              {
                on_stop_recording (self, false);
              }
            self->is_recording = 0;
            track->recording_region = NULL;
          }
          break;
        case RECORDING_EVENT_TYPE_STOP_AUTOMATION_RECORDING:
          g_message ("-------- STOP AUTOMATION RECORDING");
          {
            AutomationTrack * at =
              automation_track_find_from_port_id (
                &ev->port_id, false);
            g_warn_if_fail (at);
            if (at->recording_started)
              {
                on_stop_recording (self, true);
              }
            at->recording_started = false;
            at->recording_region = NULL;
          }
          break;
        case RECORDING_EVENT_TYPE_START_TRACK_RECORDING:
          g_message ("-------- START TRACK RECORDING");
          if (!self->is_recording)
            {
              self->num_recorded_ids = 0;
            }
          self->is_recording = 1;
          handle_start_recording (self, ev, false);
          break;
        case RECORDING_EVENT_TYPE_START_AUTOMATION_RECORDING:
          g_message ("-------- START AUTOMATION RECORDING");
          {
            AutomationTrack * at =
              automation_track_find_from_port_id (
                &ev->port_id, false);
            g_warn_if_fail (at);
            if (!at->recording_started)
              {
                handle_start_recording (
                  self, ev, true);
              }
            at->recording_started = true;
          }
          break;
        default:
          g_warning (
            "recording event %d not implemented yet",
            ev->type);
          break;
        }

      object_pool_return (
        self->event_obj_pool, ev);
    }
  /*g_message ("processed %d events", i);*/

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
  RecordingManager * self =
    object_new (RecordingManager);

  self->event_obj_pool =
    object_pool_new (
      (ObjectCreatorFunc) recording_event_new,
      (ObjectFreeFunc) recording_event_free,
      200);
  self->event_queue = mpmc_queue_new ();
  mpmc_queue_reserve (
    self->event_queue, (size_t) 200);

  self->source_id =
    g_timeout_add (
      12, (GSourceFunc) events_process, self);

  return self;
}

void
recording_manager_free (
  RecordingManager * self)
{
  g_message ("%s: Freeing...", __func__);

  /* stop source func */
  g_source_remove_and_zero (self->source_id);

  /* process pending events */
  events_process (self);

  /* free objects */
  object_free_w_func_and_null (
    mpmc_queue_free, self->event_queue);
  object_free_w_func_and_null (
    object_pool_free, self->event_obj_pool);

  free_temp_selections (self);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
