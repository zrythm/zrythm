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

#include "gui/backend/arranger_object.h"
#include "audio/audio_region.h"
#include "audio/clip.h"
#include "audio/engine.h"
#include "audio/recording_event.h"
#include "audio/recording_manager.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/mpmc_queue.h"
#include "zrythm.h"

#include <gtk/gtk.h>

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
  Track * tr = track_processor->track;

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
            RECORDING_EVENT_TYPE_STOP_RECORDING;
          re->g_start_frames = g_start_frames;
          re->local_offset = local_offset;
          re->nframes = nframes;
          strcpy (re->track_name, tr->name);
          recording_event_queue_push_back_event (
            RECORDING_MANAGER->event_queue, re);
        }
      return;
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
            RECORDING_EVENT_TYPE_START_RECORDING;
          re->g_start_frames = g_start_frames;
          re->local_offset = local_offset;
          re->nframes = nframes;
          strcpy (re->track_name, tr->name);
          recording_event_queue_push_back_event (
            RECORDING_MANAGER->event_queue, re);
        }
    }

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
             clip->channels) *
          sizeof (sample_t));
      region->frames =
        (sample_t *) realloc (
          region->frames,
          (size_t)
          (clip->num_frames *
             clip->channels) *
          sizeof (sample_t));
      region->num_frames = (size_t) clip->num_frames;
      memcpy (
        &region->frames[0], &clip->frames[0],
        sizeof (float) * (size_t) clip->num_frames *
        clip->channels);

      arranger_object_loop_end_pos_setter (
        r_obj, &TRANSPORT->loop_end_pos);
      r_obj->loop_end_pos.frames =
        TRANSPORT->loop_end_pos.frames;

      /* start new region in new lane at
       * TRANSPORT loop start */
      ZRegion * new_region =
        audio_region_new (
          -1, NULL, NULL, nframes, 2,
          &TRANSPORT->loop_start_pos,
          1);
      track_add_region (
        tr, new_region, NULL,
        region->lane_pos + 1, F_GEN_NAME,
        F_PUBLISH_EVENTS);
      region = new_region;

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
           clip->channels) *
        sizeof (sample_t));
      region->frames =
        (sample_t *) realloc (
          region->frames,
          (size_t)
          (clip->num_frames *
             clip->channels) *
          sizeof (sample_t));
      region->num_frames = (size_t) clip->num_frames;
      memcpy (
        &region->frames[0], &clip->frames[0],
        sizeof (float) * (size_t) clip->num_frames *
        clip->channels);

      arranger_object_loop_end_pos_setter (
        r_obj, &end_pos);
      r_obj->loop_end_pos.frames =
        end_pos.frames;
    }

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
      ZRegion * new_region =
        midi_region_new (
          &TRANSPORT->loop_start_pos,
          &end_pos, 1);
      track_add_region (
        tr, new_region, NULL,
        region->lane_pos + 1, F_GEN_NAME,
        F_PUBLISH_EVENTS);
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
          arranger_object_end_pos_setter (
            mn_obj,
            &TRANSPORT->loop_end_pos);
        }
    }

  if (!ev->has_midi_event)
    return;

  /* convert MIDI data to midi notes */
  MidiEvent * mev = &ev->midi_event;
  switch (mev->type)
    {
      case MIDI_EVENT_TYPE_NOTE_ON:
        g_return_if_fail (region);
        mn =
          midi_note_new (
            region, &start_pos,
            &end_pos,
            mev->note_pitch,
            mev->velocity, 1);
        midi_region_add_midi_note (
          region, mn);

        /* add to unended notes */
        array_append (
          region->unended_notes,
          region->num_unended_notes, mn);
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
              mn_obj, &end_pos);
          }
        break;
      default:
        /* TODO */
        break;
    }
}

static void
handle_start_recording (
  RecordingEvent * ev)
{
  Track * tr = track_get_from_name (ev->track_name);

  /* this could be called multiple times, ignore
   * if already processed */
  if (tr->recording_region)
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

  if (track_has_piano_roll (tr))
    {
      /* create region */
      ZRegion * region =
        midi_region_new (
          &start_pos, &end_pos, 1);
      g_return_if_fail (region);
      track_add_region (
        tr, region, NULL, tr->num_lanes - 1,
        F_GEN_NAME, F_PUBLISH_EVENTS);

      tr->recording_region = region;
    }
  else if (tr->type == TRACK_TYPE_AUDIO)
    {
      /* create region */
      ZRegion * region =
        audio_region_new (
          -1, NULL, NULL, ev->nframes, 2,
          &start_pos, 1);
      g_return_if_fail (region);
      track_add_region (
        tr, region, NULL, tr->num_lanes - 1,
        F_GEN_NAME, F_PUBLISH_EVENTS);

      tr->recording_region = region;
    }
}

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 */
static int
events_process (void * data)
{
  MPMCQueue * q = (MPMCQueue *) data;
  RecordingManager * self = RECORDING_MANAGER;
  /*gint64 curr_time = g_get_monotonic_time ();*/
  if (q != self->event_queue)
    {
      return G_SOURCE_REMOVE;
    }

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
        case RECORDING_EVENT_TYPE_STOP_RECORDING:
          /*g_message ("-------- STOP RECORDING");*/
          {
            Track * track =
              track_get_from_name (ev->track_name);
            g_warn_if_fail (track);
            track->recording_region = NULL;
          }
          break;
        case RECORDING_EVENT_TYPE_START_RECORDING:
          /*g_message ("-------- START_RECORDING");*/
          handle_start_recording (ev);
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

static void *
create_event_obj (void)
{
  return calloc (1, sizeof (RecordingEvent));
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
    calloc (1, sizeof (RecordingManager));

  ObjectPool * obj_pool;
  MPMCQueue * queue;
  if (self->event_queue &&
      self->event_obj_pool)
    {
      obj_pool = self->event_obj_pool;
      queue = self->event_queue;
      self->event_obj_pool = NULL;
      self->event_queue = NULL;
      object_pool_free (obj_pool);
      mpmc_queue_free (queue);
    }

  obj_pool =
    object_pool_new (
      create_event_obj, 200);
  queue = mpmc_queue_new ();
  mpmc_queue_reserve (
    queue, (size_t) 200);

  self->event_queue = queue;
  self->event_obj_pool = obj_pool;

  g_timeout_add (12, events_process, queue);

  return self;
}
