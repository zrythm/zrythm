// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_EXPORT_H__
#define __AUDIO_EXPORT_H__

#include <memory>
#include <utility>

#include "dsp/engine.h"
#include "dsp/position.h"
#include "utils/audio.h"

#include <glib/gi18n.h>

class ProgressInfo;

/**
 * @addtogroup dsp
 *
 * @{
 */

class Exporter final
{
public:
  /**
   * Export format.
   */
  enum class Format
  {
    AIFF,
    AU,
    CAF,
    FLAC,
    MP3,
    Vorbis,
    OggOpus,

    /** Raw audio data. */
    Raw,

    WAV,
    W64,

    /** MIDI type 0. */
    Midi0,

    /** MIDI type 1. */
    Midi1,
  };

  /**
   * Returns the audio format as a file extension.
   */
  static const char * format_get_ext (Format format)
  {

    static constexpr const char * format_exts[] = {
      "aiff", "au",  "caf", "flac", "mp3", "ogg",
      "ogg",  "raw", "wav", "w64",  "mid", "mid",
    };
    return format_exts[static_cast<int> (format)];
  }

  /**
   * Time range to export.
   */
  enum class TimeRange
  {
    Loop,
    Song,
    Custom,
  };

  /**
   * Export mode.
   *
   * If this is anything other than
   * @ref EXPORT_MODE_FULL, the @ref Track.bounce
   * or @ref Region.bounce_mode should be set.
   */
  enum class Mode
  {
    /** Export everything within the range normally. */
    Full,

    /** Export selected tracks within the range only. */
    Tracks,

    /** Export selected regions within the range only. */
    Regions,
  };

  /**
   * Export settings to be passed to the exporter to use.
   */
  class Settings
  {
  public:
    /**
     * Sets the defaults for bouncing.
     *
     * @note @ref ExportSettings.mode must already be set at this point.
     *
     * @param filepath Path to bounce to. If empty, this will generate a
     * temporary filepath.
     * @param bounce_name Name used for the file if
     *   @ref filepath is NULL.
     */
    void set_bounce_defaults (
      Format             format,
      const std::string &filepath,
      const std::string &bounce_name);

    /**
     * @brief Calculate and return the export time range positions.
     */
    std::pair<Position, Position> get_export_time_range () const;

    void print () const;

  public:
    Format      format_ = Format::WAV;
    std::string artist_;
    std::string title_;
    std::string genre_;
    /** Bit depth (16/24/64). */
    BitDepth  depth_ = BitDepth::BIT_DEPTH_16;
    TimeRange time_range_ = TimeRange::Song;

    /** Export mode. */
    Mode mode_ = Mode::Full;

    /**
     * Disable exported track (or mute region) after bounce.
     */
    bool disable_after_bounce_ = false;

    /** Bounce with parent tracks (direct outs). */
    bool bounce_with_parents_ = false;

    /**
     * Bounce step (pre inserts, pre fader, post fader).
     *
     * Only valid if @ref bounce_with_parents_ is false.
     */
    BounceStep bounce_step_ = BounceStep::PostFader;

    /** Positions in case of custom time range. */
    Position custom_start_;
    Position custom_end_;

    /** Export track lanes as separate MIDI tracks. */
    bool lanes_as_tracks_ = false;

    /**
     * Dither or not.
     */
    bool dither_ = false;

    /**
     * Absolute path for export file.
     */
    std::string file_uri_;

    /** Number of files being simultaneously exported, for progress calculation.
     */
    int num_files_ = 1;
  };

public:
  Exporter (
    Settings                      settings,
    GtkWidget *                   parent_owner = nullptr,
    std::shared_ptr<ProgressInfo> progress_info = nullptr);

  /**
   * This must be called on the main thread after the intended tracks have been
   * marked for bounce and before exporting.
   */

  void prepare_tracks_for_export (AudioEngine &engine, Transport &transport);

  /**
   * This must be called on the main thread after the export is completed.
   *
   * @param connections The array returned from @ref
   * prepare_tracks_for_export(). This function takes ownership of it.
   * @param engine_state Engine state when export was started so that it can be
   * re-set after exporting.
   */
  void post_export ();

  /**
   * Begins a generic export thread to be used for simple exporting.
   *
   * See bounce_dialog for an example.
   */
  void begin_generic_thread ()
  {
    thread_ = std::make_unique<std::jthread> ([&] (std::stop_token stoken) {
      export_to_file ();
    });
  }

  void join_generic_thread ()
  {
    if (thread_)
      thread_->join ();
  }

  /**
   * @brief Returns the exported file path.
   */
  fs::path get_exported_path () const;

  /**
   * To be called to create and perform an undoable action for creating an audio
   * track with the bounced material.
   *
   * @param pos Position to place the audio region at.
   */

  void create_audio_track_after_bounce (Position pos);

  /**
   * Exports an audio file based on the given settings.
   *
   * @throw ZrythmException if export fails.
   */

  void export_to_file ();

private:
  void export_audio (Settings &info);
  void export_midi (Settings &info);

public:
  Settings settings_;

  bool export_stems_ = false;

  /** Not owned by this instance. */
  // GThread * thread_ = nullptr;

  std::vector<Track *> tracks_;
  size_t               cur_track_ = 0;

  std::shared_ptr<ProgressInfo> progress_info_;

  GtkWidget * parent_owner_ = nullptr;

private:
  std::unique_ptr<std::vector<PortConnection>> connections_;

  /**
   * @brief Engine state when export was started so that it can be re-set after
   * exporting.
   */
  std::unique_ptr<AudioEngine::State> state_;

  std::unique_ptr<std::jthread> thread_;
};

DEFINE_ENUM_FORMATTER (
  Exporter::Format,
  Exporter_Format,
  "AIFF",
  "AU",
  "CAF",
  "FLAC",
  "MP3",
  "OGG (Vorbis)",
  "OGG (OPUS)",
  "RAW",
  "WAV",
  "W64",
  "MIDI Type 0",
  "MIDI Type 1");

DEFINE_ENUM_FORMATTER (
  Exporter::TimeRange,
  Exporter_TimeRange,
  "Loop",
  "Song",
  "Custom");

DEFINE_ENUM_FORMATTER (Exporter::Mode, Exporter_Mode, "Full", "Tracks", "Regions");

/**
 * @}
 */

#endif
