// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <inttypes.h>
#include <stdlib.h>

#include "audio/encoder.h"
#include "audio/engine.h"
#include "audio/metronome.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/audio.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/flags.h"
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
      self->emphasis_path = g_build_filename (
        src_root, "data", "samples", "klick",
        "square_emphasis.wav", NULL);
      self->normal_path = g_build_filename (
        src_root, "data", "samples", "klick",
        "/square_normal.wav", NULL);
    }
  else
    {
      char * samplesdir = zrythm_get_dir (
        ZRYTHM_DIR_SYSTEM_SAMPLESDIR);
      self->emphasis_path = g_build_filename (
        samplesdir, "square_emphasis.wav", NULL);
      self->normal_path = g_build_filename (
        samplesdir, "square_normal.wav", NULL);
    }

  /* decode */
  AudioEncoder * enc = audio_encoder_new_from_file (
    self->emphasis_path);
  if (!enc)
    {
      g_critical (
        "Failed to load samples for metronome "
        "from %s",
        self->emphasis_path);
      metronome_free (self);
      return NULL;
    }
  audio_encoder_decode (
    enc, (int) AUDIO_ENGINE->sample_rate,
    F_NO_SHOW_PROGRESS);
  self->emphasis = object_new_n (
    (size_t) (enc->num_out_frames * enc->channels),
    float);
  self->emphasis_size = enc->num_out_frames;
  self->emphasis_channels = enc->channels;
  if (enc->channels <= 0)
    {
      g_critical ("channels: %d", enc->channels);
      audio_encoder_free (enc);
      metronome_free (self);
      return NULL;
    }

  dsp_copy (
    self->emphasis, enc->out_frames,
    (size_t) enc->num_out_frames
      * (size_t) enc->channels);
  audio_encoder_free (enc);

  enc =
    audio_encoder_new_from_file (self->normal_path);
  audio_encoder_decode (
    enc, (int) AUDIO_ENGINE->sample_rate,
    F_NO_SHOW_PROGRESS);
  self->normal = object_new_n (
    (size_t) (enc->num_out_frames * enc->channels),
    float);
  self->normal_size = enc->num_out_frames;
  self->normal_channels = enc->channels;
  if (enc->channels <= 0)
    {
      g_critical ("channels: %d", enc->channels);
      audio_encoder_free (enc);
      metronome_free (self);
      return NULL;
    }
  dsp_copy (
    self->normal, enc->out_frames,
    (size_t) enc->num_out_frames
      * (size_t) enc->channels);
  audio_encoder_free (enc);

  /* set volume */
  self->volume =
    ZRYTHM_TESTING
      ? 1.f
      : (float) g_settings_get_double (
        S_TRANSPORT, "metronome-volume");

  return self;
}

/**
 * Finds all metronome events (beat and bar changes)
 * within the given range and adds them to the
 * queue of the sample processor.
 *
 * @param end_pos End position, exclusive.
 * @param loffset Local offset (this is where \ref
 *   start_pos starts at).
 */
static void
find_and_queue_metronome (
  const Position * start_pos,
  const Position * end_pos,
  const nframes_t  loffset)
{
  /* special case */
  if (start_pos->frames == end_pos->frames)
    return;

  /* find each bar / beat change from start
   * to finish */
  int num_bars_before =
    position_get_total_bars (start_pos, false);
  int num_bars_after =
    position_get_total_bars (end_pos, false);
  int bars_diff = num_bars_after - num_bars_before;

#if 0
  char start_pos_str[60];
  char end_pos_str[60];
  position_to_string (start_pos, start_pos_str);
  position_to_string (end_pos, end_pos_str);
  g_message (
    "%s: %s ~ %s <num bars before %d after %d>",
    __func__, start_pos_str, end_pos_str,
    num_bars_before, num_bars_after);
#endif

  /* handle start (not caught below) */
  if (start_pos->frames == 0)
    {
      sample_processor_queue_metronome (
        SAMPLE_PROCESSOR, METRONOME_TYPE_EMPHASIS,
        loffset);
    }

  for (int i = 0; i < bars_diff; i++)
    {
      /* get position of bar */
      Position bar_pos;
      position_init (&bar_pos);
      position_add_bars (
        &bar_pos, num_bars_before + i + 1);

      /* offset of bar pos from start pos */
      signed_frame_t bar_offset_long =
        bar_pos.frames - start_pos->frames;
      if (bar_offset_long < 0)
        {
          g_message ("bar pos:");
          position_print (&bar_pos);
          g_message ("start pos:");
          position_print (start_pos);
          g_critical (
            "bar offset long (%" PRId64 ") is < 0",
            bar_offset_long);
          return;
        }

      /* add local offset */
      signed_frame_t metronome_offset_long =
        bar_offset_long + (signed_frame_t) loffset;
      z_return_if_fail_cmp (
        metronome_offset_long, >=, 0);
      nframes_t metronome_offset =
        (nframes_t) metronome_offset_long;
      z_return_if_fail_cmp (
        metronome_offset, <,
        AUDIO_ENGINE->block_length);
      sample_processor_queue_metronome (
        SAMPLE_PROCESSOR, METRONOME_TYPE_EMPHASIS,
        metronome_offset);
    }

  int num_beats_before =
    position_get_total_beats (start_pos, false);
  int num_beats_after =
    position_get_total_beats (end_pos, false);
  int beats_diff =
    num_beats_after - num_beats_before;

  for (int i = 0; i < beats_diff; i++)
    {
      /* get position of beat */
      Position beat_pos;
      position_init (&beat_pos);
      position_add_beats (
        &beat_pos, num_beats_before + i + 1);

      /* if not a bar (already handled above) */
      if (position_get_beats (&beat_pos, true) != 1)
        {
          /* adjust position because even though
           * the start and beat pos have the same
           * ticks, their frames differ (the beat
           * position might be before the start
           * position in frames) */
          if (beat_pos.frames < start_pos->frames)
            {
              beat_pos.frames = start_pos->frames;
            }

          /* offset of beat pos from start pos */
          signed_frame_t beat_offset_long =
            beat_pos.frames - start_pos->frames;
          g_return_if_fail (beat_offset_long >= 0);

          /* add local offset */
          signed_frame_t metronome_offset_long =
            beat_offset_long
            + (signed_frame_t) loffset;
          g_return_if_fail (
            metronome_offset_long >= 0);

          nframes_t metronome_offset =
            (nframes_t) metronome_offset_long;
          g_return_if_fail (
            metronome_offset
            < AUDIO_ENGINE->block_length);
          sample_processor_queue_metronome (
            SAMPLE_PROCESSOR, METRONOME_TYPE_NORMAL,
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
  position_set_to_pos (&unlooped_playhead, PLAYHEAD);
  transport_position_add_frames (
    self->transport, &playhead_pos, nframes);
  position_add_frames (
    &unlooped_playhead, (long) nframes);
  int loop_crossed =
    unlooped_playhead.frames != playhead_pos.frames;
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
metronome_set_volume (Metronome * self, float volume)
{
  self->volume = volume;

  g_settings_set_double (
    S_TRANSPORT, "metronome-volume",
    (double) volume);
}

void
metronome_free (Metronome * self)
{
  g_free_and_null (self->emphasis_path);
  g_free_and_null (self->normal_path);
  object_zero_and_free (self->emphasis);
  object_zero_and_free (self->normal);

  object_zero_and_free (self);
}
