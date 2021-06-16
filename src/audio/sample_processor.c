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

#include "audio/engine.h"
#include "audio/metronome.h"
#include "audio/port.h"
#include "audio/sample_processor.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

static void
init_common (
  SampleProcessor * self)
{
  self->audio_track =
    track_new (
      TRACK_TYPE_AUDIO, -1, "Audio auditioner",
      true);
  self->ins_track =
    track_new (
      TRACK_TYPE_INSTRUMENT, -1, "MIDI auditioner",
      true);
}

void
sample_processor_init_loaded (
  SampleProcessor * self)
{
  fader_init_loaded (self->fader, F_PROJECT);

  init_common (self);
}

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
SampleProcessor *
sample_processor_new (void)
{
  SampleProcessor * self =
    object_new (SampleProcessor);

  self->schema_version =
    SAMPLE_PROCESSOR_SCHEMA_VERSION;

  self->fader =
    fader_new (
      FADER_TYPE_SAMPLE_PROCESSOR, NULL, false);
  fader_set_is_project (self->fader, true);

  init_common (self);

  return self;
}

/**
 * Clears the buffers.
 */
void
sample_processor_prepare_process (
  SampleProcessor * self,
  const nframes_t   nframes)
{
  fader_clear_buffers (self->fader);
}

/**
 * Removes a SamplePlayback from the array.
 */
void
sample_processor_remove_sample_playback (
  SampleProcessor * self,
  SamplePlayback *  in_sp)
{
  int i, j;
  SamplePlayback * sp, * next_sp;
  for (i = 0; i < self->num_current_samples; i++)
    {
      sp = &self->current_samples[i];
      if (in_sp != sp)
        continue;

      for (j = i; j < self->num_current_samples - 1;
           j++)
        {
          sp = &self->current_samples[j];
          next_sp = &self->current_samples[j + 1];

          sp->buf = next_sp->buf;
          sp->buf_size = next_sp->buf_size;
          sp->offset = next_sp->offset;
          sp->volume = next_sp->volume;
          sp->start_offset = next_sp->start_offset;
        }
      break;
    }
  self->num_current_samples--;
}


/**
 * Process the samples for the given number of
 * frames.
 *
 * @param cycle_offset The local offset in the
 *   processing cycle.
 */
void
sample_processor_process (
  SampleProcessor * self,
  const nframes_t   cycle_offset,
  const nframes_t   nframes)
{
  nframes_t j;
  nframes_t max_frames;
  SamplePlayback * sp;
  g_return_if_fail (
    self && self->fader &&
    self->fader->stereo_out &&
    self->num_current_samples < 256 &&
    self->fader->stereo_out->l &&
    self->fader->stereo_out->l->buf &&
    self->fader->stereo_out->r &&
    self->fader->stereo_out->r->buf);

  float * l = self->fader->stereo_out->l->buf,
        * r = self->fader->stereo_out->r->buf;

  /* process the samples in the queue */
  for (int i = self->num_current_samples - 1;
       i >= 0; i--)
    {
      sp = &self->current_samples[i];
      g_return_if_fail (sp->channels > 0);

      /* if sample is already playing */
      if (sp->offset > 0)
        {
          /* fill in the buffer for as many frames
           * as possible */
          max_frames =
            MIN (
              (nframes_t)
                (sp->buf_size - sp->offset),
              nframes);
          for (j = 0; j < max_frames; j++)
            {
              nframes_t buf_offset =
                j + cycle_offset;
              g_return_if_fail (
                buf_offset <
                  AUDIO_ENGINE->block_length &&
                sp->offset < sp->buf_size);
              if (sp->channels == 1)
                {
                  l[buf_offset] +=
                    (*sp->buf)[sp->offset] *
                    sp->volume;
                  r[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
              else if (sp->channels == 2)
                {
                  l[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                  r[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
            }
        }
      /* else if we can start playback in this
       * cycle */
      else if (sp->start_offset >= cycle_offset)
        {
          g_return_if_fail (
            sp->offset == 0 &&
            (cycle_offset + nframes) >=
               sp->start_offset);

          /* fill in the buffer for as many frames
           * as possible */
          max_frames =
            MIN (
              (nframes_t) sp->buf_size,
              (cycle_offset + nframes) -
                sp->start_offset);
          for (j = 0; j < max_frames; j++)
            {
              nframes_t buf_offset =
                j + sp->start_offset;
              g_return_if_fail (
                buf_offset <
                  AUDIO_ENGINE->block_length &&
                sp->offset < sp->buf_size);
              if (sp->channels == 1)
                {
                  l[buf_offset] +=
                    (*sp->buf)[sp->offset] *
                    sp->volume;
                  r[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
              else if (sp->channels == 2)
                {
                  l[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                  r[buf_offset] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
            }
        }

      /* if the sample is finished playing, remove
       * it */
      if (sp->offset >= sp->buf_size)
        {
          sample_processor_remove_sample_playback (
            self, sp);
        }
    }

  /* TODO process the tracks */
}

/**
 * Queues a metronomem tick at the given local
 * offset.
 */
void
sample_processor_queue_metronome (
  SampleProcessor * self,
  MetronomeType     type,
  nframes_t         offset)
{
  g_return_if_fail (
    METRONOME->emphasis && METRONOME->normal);

#if 0
  Position pos;
  position_set_to_pos (&pos, PLAYHEAD);
  position_add_frames (&pos, offset);
  char metronome_pos_str[60];
  position_to_string (&pos, metronome_pos_str);
  g_message (
    "%s metronome queued at %s (loffset %d)",
    (type == METRONOME_TYPE_EMPHASIS) ?
      "emphasis" : "normal",
    metronome_pos_str, offset);
#endif

  SamplePlayback * sp =
    &self->current_samples[
      self->num_current_samples];

  g_return_if_fail (
    offset < AUDIO_ENGINE->block_length);

  /*g_message ("queuing %u", offset);*/
  if (type == METRONOME_TYPE_EMPHASIS)
    {
      sample_playback_init (
        sp, &METRONOME->emphasis,
        METRONOME->emphasis_size,
        METRONOME->emphasis_channels,
        0.1f * METRONOME->volume, offset);
    }
  else if (type == METRONOME_TYPE_NORMAL)
    {
      sample_playback_init (
        sp, &METRONOME->normal,
        METRONOME->normal_size,
        METRONOME->normal_channels,
        0.1f * METRONOME->volume, offset);
    }

  self->num_current_samples++;
}

/**
 * Adds a sample to play to the queue from a file
 * path.
 */
void
sample_processor_queue_sample_from_file (
  SampleProcessor * self,
  const char *      path)
{
  /* TODO */
}

void
sample_processor_disconnect (
  SampleProcessor * self)
{
  fader_disconnect_all (self->fader);
}

void
sample_processor_free (
  SampleProcessor * self)
{
  sample_processor_disconnect (self);

  object_free_w_func_and_null (
    fader_free, self->fader);

  object_zero_and_free (self);
}
