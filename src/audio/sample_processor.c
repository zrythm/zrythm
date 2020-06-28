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

#include "audio/engine.h"
#include "audio/metronome.h"
#include "audio/port.h"
#include "audio/sample_processor.h"
#include "project.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

void
sample_processor_init_loaded (
  SampleProcessor * self)
{
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

  self->stereo_out =
    stereo_ports_new_generic (
      0, _("Sample Processor"),
      PORT_OWNER_TYPE_SAMPLE_PROCESSOR, self);
  g_warn_if_fail (
    IS_PORT (self->stereo_out->l) &&
    IS_PORT (self->stereo_out->r));

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
  port_clear_buffer (self->stereo_out->l);
  port_clear_buffer (self->stereo_out->r);
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
 * Process the samples for the given number of frames.
 *
 * @param offset The local offset in the processing
 *   cycle.
 */
void
sample_processor_process (
  SampleProcessor * self,
  const nframes_t   offset,
  const nframes_t   nframes)
{
  nframes_t j;
  nframes_t max_frames;
  SamplePlayback * sp;
  /*g_message ("num current samples %d",self->num_current_samples);*/
  g_return_if_fail (
    self && self->stereo_out &&
    self->num_current_samples < 256 &&
    self->stereo_out->l &&
    self->stereo_out->l->buf &&
    self->stereo_out->r &&
    self->stereo_out->r->buf);
  float * l = self->stereo_out->l->buf,
        * r = self->stereo_out->r->buf;
  /*g_message ("current offset %d", offset);*/
  /*g_message ("processing %d samples",*/
             /*self->num_current_samples);*/
  for (int i = self->num_current_samples - 1;
       i >= 0; i--)
    {
      sp = &self->current_samples[i];
      g_return_if_fail (sp->channels > 0);

      /* if sample is already playing */
      if (sp->offset > 0)
        {
          /* fill in the buffer for as many frames as
           * possible */
          max_frames =
            MIN (
              (nframes_t)
                (sp->buf_size - sp->offset),
              (nframes_t) (nframes - offset));
          /*g_message ("already playing at %d - %d",*/
                     /*offset,*/
                     /*offset + max_frames);*/
          for (j = offset;
               j < offset + max_frames; j++)
            {
              if (sp->channels == 1)
                {
                  l[j] +=
                    (*sp->buf)[sp->offset] *
                    sp->volume;
                  r[j] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
              else if (sp->channels == 2)
                {
                  l[j] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                  r[j] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
            }
        }
      /* else if we can start playback in this
       * cycle */
      else if (sp->start_offset >= offset)
        {
          /* fill in the buffer for as many frames as
           * possible */
          max_frames =
            MIN (
              (nframes_t)
                (sp->buf_size - sp->offset),
              nframes - sp->start_offset);
          /*g_message ("starting at %d - %d",*/
                     /*sp->start_offset,*/
                     /*sp->start_offset + max_frames);*/
          for (j = sp->start_offset;
               j < sp->start_offset + max_frames;
               j++)
            {
              if (sp->channels == 1)
                {
                  l[j] +=
                    (*sp->buf)[sp->offset] *
                    sp->volume;
                  r[j] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
              else if (sp->channels == 2)
                {
                  l[j] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                  r[j] +=
                    (*sp->buf)[sp->offset++] *
                    sp->volume;
                }
            }
        }
      /* if the offset is greater than the size of
       * the sample, simply add the frames */
      /*else if (sp->offset > sp->buf_size)*/
        /*{*/
          /*sp->offset += nframes;*/
        /*}*/

      /* if sample finished playing */
      /*g_message (*/
        /*"[sample %d] start offset %u, offset %lu, buf size %lu",*/
        /*i, sp->start_offset,*/
        /*sp->offset, sp->buf_size);*/
      if (sp->offset >= sp->buf_size)
        {
          /*g_message ("removed %d", i);*/
          sample_processor_remove_sample_playback (
            self, sp);
        }
    }
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

  /*g_message ("metronome queued for %d", offset);*/
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
        0.1f, offset);
    }
  else if (type == METRONOME_TYPE_NORMAL)
    {
      sample_playback_init (
        sp, &METRONOME->normal,
        METRONOME->normal_size,
        METRONOME->normal_channels,
        0.1f, offset);
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
  stereo_ports_disconnect (self->stereo_out);
}

void
sample_processor_free (
  SampleProcessor * self)
{
  sample_processor_disconnect (self);

  object_free_w_func_and_null (
    stereo_ports_free, self->stereo_out);

  object_zero_and_free (self);
}
