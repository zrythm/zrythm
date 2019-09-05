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

typedef enum ExportTimeRange
{
  TIME_RANGE_LOOP,
  TIME_RANGE_SONG,
  TIME_RANGE_CUSTOM,
} ExportTimeRange;

/**
 * Export settings to be passed to the exporter
 * to use.
 */
typedef struct ExportSettings
{
  AudioFormat       format;
  const char *      artist;
  const char *      genre;

  /**
   * Bit depth (16/24/64).
   */
  BitDepth          depth;
  ExportTimeRange   time_range;

  /**
   * In case of custom time range.
   */
  Position          custom_start;
  Position          custom_end;

  /** The previous loop status (on/off). */
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
 *
 * TODO move some things into functions.
 */
void
exporter_export (ExportSettings * info);

/**
 * @}
 */

#endif
