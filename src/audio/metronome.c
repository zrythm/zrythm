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

#include <stdlib.h>

#include "zrythm-config.h"
#include "audio/encoder.h"
#include "audio/engine.h"
#include "audio/metronome.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <gtk/gtk.h>

/**
 * Initializes the Metronome by loading the samples
 * into memory.
 */
Metronome *
metronome_new (void)
{
  Metronome * self = object_new (Metronome);

  if (ZRYTHM_TESTING)
    {
      const char * src_root =
        getenv ("G_TEST_SRC_ROOT_DIR");
      g_warn_if_fail (src_root);
      self->emphasis_path =
        g_build_filename (
          src_root, "data", "samples", "klick",
          "square_emphasis.wav", NULL);
      self->normal_path =
        g_build_filename (
          src_root, "data", "samples", "klick",
          "/square_normal.wav", NULL);
    }
  else
    {
      char * samplesdir =
        zrythm_get_dir (
          ZRYTHM_DIR_SYSTEM_SAMPLESDIR);
      self->emphasis_path =
        g_build_filename (
          samplesdir, "square_emphasis.wav", NULL);
      self->normal_path =
        g_build_filename (
          samplesdir, "square_normal.wav", NULL);
    }

  /* decode */
  AudioEncoder * enc =
    audio_encoder_new_from_file (
      self->emphasis_path);
  if (!enc)
    {
      g_critical (
        "Failed to load samples for metronome "
        "from %s",
        self->emphasis_path);
      return NULL;
    }
  audio_encoder_decode (enc, 0);
  self->emphasis =
    calloc (
      (size_t)
        (enc->num_out_frames * enc->channels),
      sizeof (float));
  self->emphasis_size = enc->num_out_frames;
  self->emphasis_channels = enc->channels;
  g_return_val_if_fail (
    enc->channels > 0, NULL);
  for (int i = 0;
       i < enc->num_out_frames * enc->channels;
       i++)
    {
      self->emphasis[i] = enc->out_frames[i];
    }
  audio_encoder_free (enc);

  enc =
    audio_encoder_new_from_file (
      self->normal_path);
  audio_encoder_decode (
    enc, 0);
  self->normal =
    calloc (
      (size_t)
        (enc->num_out_frames * enc->channels),
      sizeof (float));
  self->normal_size = enc->num_out_frames;
  self->normal_channels = enc->channels;
  g_return_val_if_fail (enc->channels > 0, NULL);
  for (int i = 0;
       i < enc->num_out_frames * enc->channels;
       i++)
    {
      self->normal[i] = enc->out_frames[i];
    }
  audio_encoder_free (enc);

  return self;
}

/**
 * Finds all metronome events (beat and bar changes)
 * within the given range and adds them to the
 * queue of the sample processor.
 *
 * @param end_pos End position, exclusive.
 * @param loffset Local offset.
 */
static void
find_and_queue_metronome (
  const Position * start_pos,
  const Position * end_pos,
  const nframes_t  loffset)
{
  /* find each bar / beat change from start
   * to finish */
  int num_bars_before =
    position_get_total_bars (start_pos);
  int num_bars_after =
    position_get_total_bars (end_pos);

#if 0
  /* ignore bars right at the edge of the end
   * pos (probably not needed) */
  if (end_pos->beats == 1 &&
      end_pos->sixteenths == 1 &&
      end_pos->ticks == 0)
    {
      num_bars_after--;
    }
#endif

  for (int i = num_bars_before + 1;
       i <= num_bars_after;
       i++)
    {
      Position bar_pos;
      position_set_to_bar (
        &bar_pos, i + 1);
      long bar_offset_long =
        bar_pos.frames - start_pos->frames;
      g_return_if_fail (bar_offset_long >= 0);
      long metronome_offset_long =
        bar_offset_long + loffset;
      g_return_if_fail (bar_offset_long >= 0);
      nframes_t metronome_offset =
        (nframes_t) metronome_offset_long;
      g_return_if_fail (
        metronome_offset <
          AUDIO_ENGINE->block_length);
      sample_processor_queue_metronome (
        SAMPLE_PROCESSOR,
        METRONOME_TYPE_EMPHASIS,
        metronome_offset);
    }

  int num_beats_before =
    position_get_total_beats (start_pos);
  int num_beats_after =
    position_get_total_beats (end_pos);

#if 0
  if (end_pos->sixteenths == 1 &&
      end_pos->ticks == 0)
    num_beats_after--;
#endif

  for (int i = num_beats_before + 1;
       i <= num_beats_after;
       i++)
    {
      Position beat_pos;
      position_set_to_bar (
        &beat_pos,
        i / TRANSPORT->beats_per_bar);
      position_set_beat (
        &beat_pos,
        i % TRANSPORT->beats_per_bar + 1);
      if (beat_pos.beats != 1)
        {
          /* adjust position because even though
           * the start and beat pos have the same
           * ticks, their frames differ (the beat
           * position might be before the start
           * position in frames) */
          if (beat_pos.frames < start_pos->frames)
            beat_pos.frames = start_pos->frames;

          long beat_offset_long =
            beat_pos.frames - start_pos->frames;
          g_return_if_fail (beat_offset_long >= 0);
          long metronome_offset_long =
            beat_offset_long + loffset;
          g_return_if_fail (
            metronome_offset_long >= 0);
          nframes_t metronome_offset =
            (nframes_t) metronome_offset_long;
          g_return_if_fail (
            metronome_offset <
              AUDIO_ENGINE->block_length);
          sample_processor_queue_metronome (
            SAMPLE_PROCESSOR,
            METRONOME_TYPE_NORMAL,
            metronome_offset);
        }
    }
}

/**
 * Queues metronome events (if any) within the
 * current processing cycle.
 *
 * @param loffset Local offset in this cycle.
 */
void
metronome_queue_events (
 AudioEngine *   self,
 const nframes_t loffset,
 const nframes_t nframes)
{
  Position playhead_pos, bar_pos, beat_pos,
           unlooped_playhead;
  position_init (&bar_pos);
  position_init (&beat_pos);
  position_set_to_pos (&playhead_pos, PLAYHEAD);
  position_set_to_pos (
    &unlooped_playhead, PLAYHEAD);
  transport_position_add_frames (
    self->transport, &playhead_pos, nframes);
  position_add_frames (
    &unlooped_playhead, (long) nframes);
  int loop_crossed =
    unlooped_playhead.frames !=
    playhead_pos.frames;
  if (loop_crossed)
    {
      /* find each bar / beat change until loop
       * end */
      find_and_queue_metronome (
        PLAYHEAD, &self->transport->loop_end_pos,
        loffset);

      /* find each bar / beat change after loop
       * start */
      find_and_queue_metronome (
        &self->transport->loop_start_pos,
        &playhead_pos,
        loffset +
          (nframes_t)
          (self->transport->loop_end_pos.frames -
           PLAYHEAD->frames));
    }
  else /* loop not crossed */
    {
      /* find each bar / beat change from start
       * to finish */
      find_and_queue_metronome (
        PLAYHEAD, &playhead_pos, loffset);
    }
}

void
metronome_free (
  Metronome * self)
{
  g_free_and_null (self->emphasis_path);
  g_free_and_null (self->normal_path);
  object_zero_and_free (self->emphasis);
  object_zero_and_free (self->normal);

  object_zero_and_free (self);
}
