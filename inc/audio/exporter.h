/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/audio.h"

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Export format.
 */
typedef enum ExportFormat
{
  EXPORT_FORMAT_FLAC,
  EXPORT_FORMAT_OGG_VORBIS,
  EXPORT_FORMAT_OGG_OPUS,
  EXPORT_FORMAT_WAV,
  EXPORT_FORMAT_MP3,

  /** MIDI type 0. */
  EXPORT_FORMAT_MIDI0,

  /** MIDI type 1. */
  EXPORT_FORMAT_MIDI1,

  /** Raw audio data. */
  EXPORT_FORMAT_RAW,

  NUM_EXPORT_FORMATS,
} ExportFormat;

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
 * @ref EXPORT_MODE_FULL, the @ref Track.bounce
 * or @ref ZRegion.bounce_mode should be set.
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

static const char * export_mode_str[] =
{
  "Full", "Tracks", "Regions",
};

static inline const char *
export_mode_to_str (
  ExportMode export_mode)
{
  return export_mode_str[export_mode];
}

typedef enum BounceStep
{
  BOUNCE_STEP_BEFORE_INSERTS,
  BOUNCE_STEP_PRE_FADER,
  BOUNCE_STEP_POST_FADER,
} BounceStep;

static const char * bounce_step_str[] =
{
  __("Before inserts"),
  __("Pre-fader"),
  __("Post fader"),
};

static inline const char *
bounce_step_to_str (
  BounceStep bounce_step)
{
  return bounce_step_str[bounce_step];
}

/**
 * Export settings to be passed to the exporter
 * to use.
 */
typedef struct ExportSettings
{
  ExportFormat      format;
  char *            artist;
  char *            title;
  char *            genre;

  /** Bit depth (16/24/64). */
  BitDepth          depth;
  ExportTimeRange   time_range;

  /** Export mode. */
  ExportMode        mode;

  /**
   * Disable exported track (or mute region) after
   * bounce.
   */
  bool              disable_after_bounce;

  /** Bounce with parent tracks (direct outs). */
  bool              bounce_with_parents;

  /**
   * Bounce step (pre inserts, pre fader, post
   * fader).
   *
   * Only valid if ExportSettings.bounce_with_parents
   * is false.
   */
  BounceStep        bounce_step;

  /** Positions in case of custom time range. */
  Position          custom_start;
  Position          custom_end;

  /** Export track lanes as separate MIDI tracks. */
  bool              lanes_as_tracks;

  /**
   * Dither or not.
   */
  bool              dither;

  /**
   * Absolute path for export file.
   */
  char *            file_uri;

  /** Number of files being simultaneously exported,
   * for progress calculation. */
  int               num_files;

  GenericProgressInfo progress_info;
} ExportSettings;

/**
 * Returns an instance of default ExportSettings.
 *
 * It must be free'd with export_settings_free().
 */
ExportSettings *
export_settings_default (void);

/**
 * Sets the defaults for bouncing.
 *
 * @note \ref ExportSettings.mode must already be
 *   set at this point.
 *
 * @param filepath Path to bounce to. If NULL, this
 *   will generate a temporary filepath.
 * @param bounce_name Name used for the file if
 *   \ref filepath is NULL.
 */
void
export_settings_set_bounce_defaults (
  ExportSettings * self,
  ExportFormat     format,
  const char *     filepath,
  const char *     bounce_name);

void
export_settings_print (
  const ExportSettings * self);

void
export_settings_free_members (
  ExportSettings * self);

void
export_settings_free (
  ExportSettings * self);

/**
 * This must be called on the main thread after the
 * intended tracks have been marked for bounce and
 * before exporting.
 */
NONNULL
GPtrArray *
exporter_prepare_tracks_for_export (
  const ExportSettings * const settings);

/**
 * This must be called on the main thread after the
 * export is completed.
 *
 * @param connections The array returned from
 *   exporter_prepare_tracks_for_export(). This
 *   function takes ownership of it and is
 *   responsible for freeing it.
 */
NONNULL_ARGS (1)
void
exporter_return_connections_post_export (
  const ExportSettings * const settings,
  GPtrArray *                  connections);

/**
 * Generic export thread to be used for simple
 * exporting.
 *
 * See bounce_dialog for an example.
 */
void *
exporter_generic_export_thread (
  void * data);

/**
 * To be called to create and perform an undoable
 * action for creating an audio track with the
 * bounced material.
 *
 * @param pos Position to place the audio region
 *   at.
 */
void
exporter_create_audio_track_after_bounce (
  ExportSettings * settings,
  const Position * pos);

/**
 * Returns the audio format as string.
 *
 * @param extension Whether to return the extension
 *   for this format, or a human friendly label.
 */
const char *
exporter_stringize_export_format (
  ExportFormat format,
  bool         extension);

/**
 * Exports an audio file based on the given
 * settings.
 *
 * @return Non-zero if fail.
 */
int
exporter_export (ExportSettings * info);

/**
 * @}
 */

#endif
