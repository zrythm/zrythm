// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
   Copyright (C) 2011-2013 Robin Gareus <robin@gareus.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser Public License as published by
   the Free Software Foundation; either version 2.1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * SPDX-FileCopyrightText: (C) 2011-2013 Robin Gareus <robin@gareus.org>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * ---
 */

#include <inttypes.h>

#include "io/audio_file.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/objects.h"
#include "utils/resampler.h"

#include <glib/gi18n.h>

#include <sndfile.h>

typedef enum
{
  Z_AUDIO_FILE_ERROR_FILE_OPEN_FAILED,
  Z_AUDIO_FILE_ERROR_FILE_CLOSE_FAILED,
  Z_AUDIO_FILE_ERROR_INVALID_DATA,
  Z_AUDIO_FILE_ERROR_FAILED,
} ZAudioFileError;

#define Z_AUDIO_FILE_ERROR z_audio_file_error_quark ()
GQuark
z_audio_file_error_quark (void);
G_DEFINE_QUARK (
  z - audio - file - error - quark,
  z_audio_file_error)

typedef struct InternalData
{
  SNDFILE * sffile;
  SF_INFO   sfinfo;
  size_t    last_seek;
} InternalData;

static void
clear_metadata (AudioFile * self)
{
  memset (&self->metadata, 0, sizeof (AudioFileMetadata));
}

/**
 * Creates a new instance of an AudioFile for the given path.
 *
 * This may be a file to read from or a file to write to.
 */
AudioFile *
audio_file_new (const char * filepath)
{
  AudioFile * self = object_new (AudioFile);

  self->filepath = g_strdup (filepath);
  self->internal_data = object_new (InternalData);

  return self;
}

static int
parse_bit_depth (int format)
{
  /* see http://www.mega-nerd.com/libsndfile/api.html */
  switch (format & 0x0f)
    {
    case SF_FORMAT_PCM_S8:
      return 8;
    case SF_FORMAT_PCM_16:
      return 16; /* Signed 16 bit data */
    case SF_FORMAT_PCM_24:
      return 24; /* Signed 24 bit data */
    case SF_FORMAT_PCM_32:
      return 32; /* Signed 32 bit data */
    case SF_FORMAT_PCM_U8:
      return 8; /* Unsigned 8 bit data (WAV and RAW only) */
    case SF_FORMAT_FLOAT:
      return 32; /* 32 bit float data */
    case SF_FORMAT_DOUBLE:
      return 64; /* 64 bit float data */
    default:
      break;
    }
  return 0;
}

/**
 * Ensures there is an opened file.
 *
 * When writing, sfinfo must be pre-filled.
 */
static bool
ensure_sndfile (AudioFile * self, int mode, GError ** error)
{
  InternalData * idata = (InternalData *) self->internal_data;
  if (idata->sffile)
    {
      g_debug ("sndfile already exists, skipping");
      return true;
    }

  if (mode == SFM_WRITE)
    {
      if (!sf_format_check (&idata->sfinfo))
        {
          g_set_error (
            error, Z_AUDIO_FILE_ERROR, Z_AUDIO_FILE_ERROR_INVALID_DATA,
            _ ("sf_format_check failed for '%s': %s (error code %i)"),
            self->filepath, sf_strerror (NULL), sf_error (NULL));
          return false;
        }
    }

  g_debug ("opening sndfile at %s...", self->filepath);
  idata->sffile = sf_open (self->filepath, mode, &idata->sfinfo);
  if (!idata->sffile)
    {
      g_set_error (
        error, Z_AUDIO_FILE_ERROR, Z_AUDIO_FILE_ERROR_FILE_OPEN_FAILED,
        _ ("Failed to open audio file at '%s': %s (error code %i)"),
        self->filepath, sf_strerror (NULL), sf_error (NULL));
      return false;
    }

  return true;
}

/**
 * Reads the metadata for the given file.
 */
bool
audio_file_read_metadata (AudioFile * self, GError ** error)
{
  clear_metadata (self);

  InternalData * idata = (InternalData *) self->internal_data;

  g_debug ("attempting to read metadata for %s", self->filepath);

  memset (&idata->sfinfo, 0, sizeof (SF_INFO));
  GError * err = NULL;
  if (!ensure_sndfile (self, SFM_READ, &err))
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, _ ("Failed to open audio file"));
      return false;
    }

  SNDFILE * sffile = idata->sffile;
  SF_INFO * sfinfo = &idata->sfinfo;

  AudioFileMetadata * metadata = &self->metadata;
  metadata->channels = sfinfo->channels;
  metadata->num_frames = sfinfo->frames;
  if (metadata->num_frames > 18446744073709550)
    {
      g_set_error (
        error, Z_AUDIO_FILE_ERROR, Z_AUDIO_FILE_ERROR_FAILED,
        "Too many frames returned: %" PRIi64, metadata->num_frames);
      return false;
    }
  metadata->samplerate = sfinfo->samplerate;
  metadata->length =
    sfinfo->samplerate ? (sfinfo->frames * 1000) / sfinfo->samplerate : 0;
  metadata->bit_depth = parse_bit_depth (sfinfo->format);
  metadata->bit_rate =
    metadata->bit_depth * metadata->channels * metadata->samplerate;

  SF_LOOP_INFO loop;
  if (sf_command (sffile, SFC_GET_LOOP_INFO, &loop, sizeof (loop)) == SF_TRUE)
    {
      metadata->bpm = loop.bpm;
    }

  if (!audio_file_finish (self, &err))
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, _ ("Failed to close audio file after reading info"));
      return false;
    }

  self->metadata.filled = true;
  return true;
}

/**
 * Reads the file into an internal float array (interleaved).
 *
 * @param samples Samples to fill in.
 * @param in_parts Whether to read the file in parts. If true,
 *   @p start_from and @p num_frames_to_read must be specified.
 * @param samples[out] Pre-allocated frame array. Caller must
 *   ensure there is enough space (ie, number of frames *
 *   number of channels).
 */
bool
audio_file_read_samples (
  AudioFile * self,
  bool        in_parts,
  float *     samples,
  size_t      start_from,
  size_t      num_frames_to_read,
  GError **   error)
{
  g_return_val_if_fail (self->metadata.filled, false);

  AudioFileMetadata * metadata = &self->metadata;
  InternalData *      idata = (InternalData *) self->internal_data;

  GError * err = NULL;
  idata->sfinfo.format = 0;
  if (!ensure_sndfile (self, SFM_READ, &err))
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, _ ("Failed to open audio file"));
      return false;
    }

  if (in_parts)
    {
      sf_count_t num_seeked =
        sf_seek (idata->sffile, (sf_count_t) start_from, SEEK_SET | SFM_READ);
      if (num_seeked != (sf_count_t) start_from)
        {
          g_set_error (
            error, Z_AUDIO_FILE_ERROR, Z_AUDIO_FILE_ERROR_FAILED,
            _ ("Tried to seek %zu frames but actually "
               "seeked %zd frames"),
            start_from, num_seeked);
          return false;
        }
      sf_count_t num_read = sf_readf_float (
        idata->sffile, samples, (sf_count_t) num_frames_to_read);
      if (num_read != (sf_count_t) num_frames_to_read)
        {
          g_set_error (
            error, Z_AUDIO_FILE_ERROR, Z_AUDIO_FILE_ERROR_FAILED,
            _ ("Expected to read %zu frames but actually "
               "read %zd frames"),
            num_frames_to_read, num_read);
          return false;
        }
    }
  else
    {
      /* read all frames */
      g_debug ("reading all frames from file %s", self->filepath);
      sf_count_t num_read =
        sf_readf_float (idata->sffile, samples, metadata->num_frames);
      if (num_read != metadata->num_frames)
        {
          g_set_error (
            error, Z_AUDIO_FILE_ERROR, Z_AUDIO_FILE_ERROR_FAILED,
            _ ("Expected to read %" PRId64 " frames but actually "
               "read %zd frames"),
            metadata->num_frames, num_read);
          return false;
        }
    }

  return true;
}

/**
 * Must be called when done reading or writing files (or when
 * the operation was cancelled).
 */
bool
audio_file_finish (AudioFile * self, GError ** error)
{
  InternalData * idata = (InternalData *) self->internal_data;

  g_return_val_if_fail (idata->sffile, false);

  g_debug ("closing sndfile at %s...", self->filepath);
  if (sf_close (idata->sffile) != 0)
    {
      g_set_error (
        error, Z_AUDIO_FILE_ERROR, Z_AUDIO_FILE_ERROR_FAILED,
        _ ("Failed to close audio file at '%s': %s (error code %i)"),
        self->filepath, sf_strerror (idata->sffile), sf_error (idata->sffile));
      return false;
    }
  idata->sffile = NULL;

  return true;
}

/**
 * Simple blocking API for reading and optionally resampling
 * audio files.
 *
 * Only to be used on small files.
 *
 * @param[out] frames Pointer to store newly allocated
 *   interlaved frames to.
 * @param[out] metadata File metadata will be pasted here if
 *   non-NULL.
 * @param samplerate If specified, the audio will be resampled
 *   to the given samplerate. Pass 0 to avoid resampling.
 */
bool
audio_file_read_simple (
  const char *        filepath,
  float **            frames,
  size_t *            num_frames,
  AudioFileMetadata * metadata,
  size_t              samplerate,
  GError **           error)
{
  g_return_val_if_fail (filepath, false);
  g_return_val_if_fail (frames, false);
  g_return_val_if_fail (num_frames, false);

  /* read metadata */
  AudioFile * af = audio_file_new (filepath);
  GError *    err = NULL;
  bool        success = audio_file_read_metadata (af, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "Error reading metadata from %s", filepath);
      return false;
    }
  if (metadata)
    {
      *metadata = af->metadata;
    }

  /* read frames in file's sample rate */
  z_return_val_if_fail_cmp (af->metadata.num_frames, >=, 0, false);
  z_return_val_if_fail_cmp (af->metadata.channels, >, 0, false);
  size_t arr_size =
    (size_t) af->metadata.num_frames * (size_t) af->metadata.channels;
  *frames =
    static_cast<float *> (g_realloc (*frames, arr_size * sizeof (float)));
  success = audio_file_read_samples (
    af, false, *frames, 0, (size_t) af->metadata.num_frames, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "Error reading frames from %s", filepath);
      return false;
    }

  if (samplerate != 0)
    {
      /* resample to project's sample rate */
      Resampler * r = resampler_new (
        *frames, (size_t) af->metadata.num_frames, af->metadata.samplerate,
        samplerate, (unsigned int) af->metadata.channels,
        RESAMPLER_QUALITY_VERY_HIGH, &err);
      if (!r)
        {
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            error, err, "Failed to create resampler");
          return false;
        }
      while (!resampler_is_done (r))
        {
          success = resampler_process (r, &err);
          if (!success)
            {
              PROPAGATE_PREFIXED_ERROR_LITERAL (
                error, err, "Resampler failed to process");
              return false;
            }
        }
      *num_frames = r->num_out_frames;
      arr_size = *num_frames * (size_t) af->metadata.channels;
      *frames =
        static_cast<float *> (g_realloc (*frames, arr_size * sizeof (float)));
      dsp_copy (*frames, r->out_frames, arr_size);
      object_free_w_func_and_null (resampler_free, r);
    }

  success = audio_file_finish (af, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, "Failed to finish audio file usage");
      return false;
    }
  object_free_w_func_and_null (audio_file_free, af);

  return true;
}

void
audio_file_free (AudioFile * self)
{
  InternalData * idata = (InternalData *) self->internal_data;
  if (idata->sffile)
    {
      g_critical ("SNDFILE is still opened.");
    }
  clear_metadata (self);

  object_zero_and_free (self);
}
