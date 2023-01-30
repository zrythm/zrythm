// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio file encoder.
 */

#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "utils/types.h"

#include <glib.h>

#include <audec/audec.h>

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Struct for holding info for encoding.
 */
typedef struct AudioEncoder
{
  /** Filename, if encoding from file. */
  char * file;

  /** Number of channels. */
  channels_t channels;

  /** The output frames interleaved. */
  float * out_frames;

  /** Output number of frames per channel. */
  unsigned_frame_t num_out_frames;

  AudecInfo     nfo;
  AudecHandle * audec_handle;
} AudioEncoder;

/**
 * Creates a new instance of an AudioEncoder from
 * the given file, that can be used for encoding.
 *
 * It converts the file into a float array in its
 * original sample rate and prepares the instance
 * for decoding it into the project's sample rate.
 */
AudioEncoder *
audio_encoder_new_from_file (
  const char * filepath,
  GError **    error);

/**
 * Decodes the information in the AudioEncoder
 * instance and stores the results there.
 *
 * Resamples the input float array to match the
 * project's sample rate.
 *
 * Assumes that the input array is already filled in.
 *
 * @param show_progress Display a progress dialog.
 */
NONNULL void
audio_encoder_decode (
  AudioEncoder * self,
  int            samplerate,
  bool           show_progress);

/**
 * Free's the AudioEncoder and its members.
 */
NONNULL void
audio_encoder_free (AudioEncoder * self);

/**
 * @}
 */

#endif
