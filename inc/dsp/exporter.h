// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_EXPORT_H__
#define __AUDIO_EXPORT_H__

#include "dsp/position.h"
#include "utils/audio.h"

typedef struct EngineState  EngineState;
typedef struct ProgressInfo ProgressInfo;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Export format.
 */
typedef enum ExportFormat
{
  EXPORT_FORMAT_AIFF,
  EXPORT_FORMAT_AU,
  EXPORT_FORMAT_CAF,
  EXPORT_FORMAT_FLAC,
  EXPORT_FORMAT_MP3,
  EXPORT_FORMAT_OGG_VORBIS,
  EXPORT_FORMAT_OGG_OPUS,

  /** Raw audio data. */
  EXPORT_FORMAT_RAW,

  EXPORT_FORMAT_WAV,
  EXPORT_FORMAT_W64,

  /** MIDI type 0. */
  EXPORT_FORMAT_MIDI0,

  /** MIDI type 1. */
  EXPORT_FORMAT_MIDI1,

  NUM_EXPORT_FORMATS,
} ExportFormat;

/**
 * Returns the format as a human friendly label.
 */
const char *
export_format_to_pretty_str (ExportFormat format);

/**
 * Returns the audio format as a file extension.
 */
const char *
export_format_to_ext (ExportFormat format);

ExportFormat
export_format_from_pretty_str (const char * pretty_str);

/**
 * Time range to export.
 */
typedef enum ExportTimeRange
{
  TIME_RANGE_LOOP,
  TIME_RANGE_SONG,
  TIME_RANGE_CUSTOM,
} ExportTimeRange;

static const char * export_time_range_str[] = {
  "Loop",
  "Song",
  "Custom",
};

static inline const char *
export_time_range_to_str (ExportTimeRange export_time_range)
{
  return export_time_range_str[export_time_range];
}

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

static const char * export_mode_str[] = {
  "Full",
  "Tracks",
  "Regions",
};

static inline const char *
export_mode_to_str (ExportMode export_mode)
{
  return export_mode_str[export_mode];
}

typedef enum BounceStep
{
  BOUNCE_STEP_BEFORE_INSERTS,
  BOUNCE_STEP_PRE_FADER,
  BOUNCE_STEP_POST_FADER,
} BounceStep;

static const char * bounce_step_str[] = {
  __ ("Before inserts"),
  __ ("Pre-fader"),
  __ ("Post fader"),
};

static inline const char *
bounce_step_to_str (BounceStep bounce_step)
{
  return bounce_step_str[bounce_step];
}

/**
 * Export settings to be passed to the exporter
 * to use.
 */
typedef struct ExportSettings
{
  ExportFormat format;
  char *       artist;
  char *       title;
  char *       genre;

  /** Bit depth (16/24/64). */
  BitDepth        depth;
  ExportTimeRange time_range;

  /** Export mode. */
  ExportMode mode;

  /**
   * Disable exported track (or mute region) after
   * bounce.
   */
  bool disable_after_bounce;

  /** Bounce with parent tracks (direct outs). */
  bool bounce_with_parents;

  /**
   * Bounce step (pre inserts, pre fader, post
   * fader).
   *
   * Only valid if ExportSettings.bounce_with_parents
   * is false.
   */
  BounceStep bounce_step;

  /** Positions in case of custom time range. */
  Position custom_start;
  Position custom_end;

  /** Export track lanes as separate MIDI tracks. */
  bool lanes_as_tracks;

  /**
   * Dither or not.
   */
  bool dither;

  /**
   * Absolute path for export file.
   */
  char * file_uri;

  /** Number of files being simultaneously exported,
   * for progress calculation. */
  int num_files;

  ProgressInfo * progress_info;
} ExportSettings;

/**
 * Returns an instance of default ExportSettings.
 *
 * It must be free'd with export_settings_free().
 */
ExportSettings *
export_settings_new (void);

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
export_settings_print (const ExportSettings * self);

void
export_settings_free (ExportSettings * self);

/**
 * This must be called on the main thread after the
 * intended tracks have been marked for bounce and
 * before exporting.
 *
 * @param engine_state Engine state when export was started so
 *   that it can be re-set after exporting.
 */
NONNULL GPtrArray *
exporter_prepare_tracks_for_export (
  const ExportSettings * const settings,
  EngineState *                engine_state);

/**
 * This must be called on the main thread after the
 * export is completed.
 *
 * @param connections The array returned from
 *   exporter_prepare_tracks_for_export(). This
 *   function takes ownership of it and is
 *   responsible for freeing it.
 * @param engine_state Engine state when export was started so
 *   that it can be re-set after exporting.
 */
NONNULL_ARGS (1)
void exporter_post_export (
  const ExportSettings * const settings,
  GPtrArray *                  connections,
  EngineState *                engine_state);

/**
 * Generic export thread to be used for simple
 * exporting.
 *
 * See bounce_dialog for an example.
 *
 * To be used as a GThreadFunc.
 */
void *
exporter_generic_export_thread (void * data);

/**
 * Generic export task thread function to be used for simple
 * exporting.
 *
 * To be used as a GTaskThreadFunc.
 *
 * TODO.
 */
void
exporter_generic_export_task_thread (
  GTask *        task,
  gpointer       source_obj,
  gpointer       task_data,
  GCancellable * cancellable);

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
