/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __AUDIO_EXPORT_H__
#define __AUDIO_EXPORT_H__

#include "audio/position.h"

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Audio format.
 */
typedef enum AudioFormat
{
  AUDIO_FORMAT_FLAC,
  AUDIO_FORMAT_OGG,
  AUDIO_FORMAT_WAV,
  AUDIO_FORMAT_MP3,
  AUDIO_FORMAT_MIDI,

  /** Raw audio data. */
  AUDIO_FORMAT_RAW,

  NUM_AUDIO_FORMATS,
} AudioFormat;

/**
 * Bit depth.
 */
typedef enum BitDepth
{
  BIT_DEPTH_16,
  BIT_DEPTH_24,
  BIT_DEPTH_32
} BitDepth;

/**
 * Time range to export.
 */
typedef enum ExportTimeRange
{
  TIME_RANGE_LOOP,
  TIME_RANGE_SONG,
  TIME_RANGE_CUSTOM,
} ExportTimeRange;

/**
 * Export mode.
 *
 * If this is anything other than
 * \ref EXPORT_MODE_FULL, the \ref Track.bounce_mode
 * or \ref ZRegion.bounce_mode should be set.
 */
typedef enum ExportMode
{
  /** Export everything within the range normally. */
  EXPORT_MODE_FULL,

  /** Export selected tracks within the range
   * only. */
  EXPORT_MODE_TRACKS,

  /** Export selected regions within the range
   * only. */
  EXPORT_MODE_REGIONS,
} ExportMode;

/**
 * Export settings to be passed to the exporter
 * to use.
 */
typedef struct ExportSettings
{
  AudioFormat       format;
  char *            artist;
  char *            genre;

  /** Bit depth (16/24/64). */
  BitDepth          depth;
  ExportTimeRange   time_range;

  /** Export mode. */
  ExportMode        mode;

  /** Positions in case of custom time range. */
  Position          custom_start;
  Position          custom_end;

  /**
   * The previous loop status (on/off).
   *
   * This is set internally.
   */
  int               prev_loop;

  /**
   * Dither or not.
   */
  int               dither;

  /**
   * Absolute path for export file.
   */
  char *            file_uri;

  /** Progress done (0.0 to 1.0). */
  double            progress;
} ExportSettings;

/**
 * Returns an instance of default ExportSettings.
 *
 * It must be free'd with export_settings_free().
 */
ExportSettings *
export_settings_default (void);

void
export_settings_free_members (
  ExportSettings * self);

void
export_settings_free (
  ExportSettings * self);

/**
 * Returns the audio format as string.
 *
 * Must be g_free()'d by caller.
 */
char *
exporter_stringize_audio_format (
  AudioFormat format);

/**
 * Exports an audio file based on the given
 * settings.
 */
int
exporter_export (ExportSettings * info);

/**
 * @}
 */

#endif
