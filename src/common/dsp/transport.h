// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Transport API.
 */

#ifndef __AUDIO_TRANSPORT_H__
#define __AUDIO_TRANSPORT_H__

#include <glib/gi18n.h>

#include "common/dsp/midi_port.h"
#include "common/dsp/port.h"
#include "common/dsp/position.h"
#include "common/utils/types.h"

class TimelineSelections;
class Marker;
class ArrangerSelections;
class TempoTrack;
TYPEDEF_STRUCT_UNDERSCORED (ArrangerWidget);

/**
 * @addtogroup dsp
 *
 * @{
 */

#define TRANSPORT (AUDIO_ENGINE->transport_)
constexpr int TRANSPORT_DEFAULT_TOTAL_BARS = 128;

#define PLAYHEAD (TRANSPORT->playhead_pos_)

enum class PrerollCountBars
{
  PREROLL_COUNT_BARS_NONE,
  PREROLL_COUNT_BARS_ONE,
  PREROLL_COUNT_BARS_TWO,
  PREROLL_COUNT_BARS_FOUR,
};

static const char * preroll_count_bars_str[] = {
  N_ ("None"),
  N_ ("1 bar"),
  N_ ("2 bars"),
  N_ ("4 bars"),
};

/**
 * The Transport class represents the transport controls and state for an audio
 * engine. It manages playback, recording, and other transport-related
 * functionality.
 */
class Transport final
    : public ICloneable<Transport>,
      public ISerializable<Transport>
{
public:
  enum class PlayState
  {
    RollRequested,
    Rolling,
    PauseRequested,
    Paused
  };

  /**
   * Corrseponts to "transport-display" in the
   * gsettings.
   */
  enum class Display
  {
    BBT,
    Time,
  };

  /**
   * Recording mode for MIDI and audio.
   *
   * In all cases, only objects created during the current recording cycle can
   * be changed. Previous objects shall not be touched.
   */
  enum class RecordingMode
  {
    /**
     * Overwrite events in first recorded region.
     *
     * In the case of MIDI, this will remove existing MIDI notes during
     * recording.
     *
     * In the case of audio, this will act exactly the same as @ref
     * RECORDING_MODE_TAKES_MUTED.
     */
    OverwriteEvents,

    /**
     * Merge events in existing region.
     *
     * In the case of MIDI, this will append MIDI notes.
     *
     * In the case of audio, this will act exactly the same as @ref
     * RECORDING_MODE_TAKES.
     */
    MergeEvents,

    /**
     * Events get put in new lanes each time recording starts/resumes (eg,
     * when looping or entering/leaving the punch range).
     */
    Takes,

    /**
     * Same as @ref RECORDING_MODE_TAKES, except previous takes (since
     * recording started) are muted.
     */
    TakesMuted,
  };

public:
  Transport () { init_common (); };
  Transport (AudioEngine * audio_engine);

  static inline const char * preroll_count_to_str (PrerollCountBars bars)
  {
    return preroll_count_bars_str[static_cast<int> (bars)];
  }

  static inline int preroll_count_bars_enum_to_int (PrerollCountBars bars)
  {
    switch (bars)
      {
      case PrerollCountBars::PREROLL_COUNT_BARS_NONE:
        return 0;
      case PrerollCountBars::PREROLL_COUNT_BARS_ONE:
        return 1;
      case PrerollCountBars::PREROLL_COUNT_BARS_TWO:
        return 2;
      case PrerollCountBars::PREROLL_COUNT_BARS_FOUR:
        return 4;
      }
    return -1;
  }

  bool is_in_active_project () const;

  bool is_rolling () const { return play_state_ == PlayState::Rolling; }

  bool is_paused () const { return play_state_ == PlayState::Paused; }

  bool is_looping () const { return loop_; }

  bool is_recording () const { return recording_; }
  /**
   * Initialize loaded transport.
   *
   * @param engine Owner engine, if any.
   * @param tempo_track Tempo track, used to initialize the caches. Only needed
   * on the active project transport.
   */
  void init_loaded (AudioEngine * engine, const TempoTrack * tempo_track);

  /**
   * Prepares audio regions for stretching (sets the
   * @ref Region.before_length).
   *
   * @param selections If nullptr, all audio regions
   *   are used. If non-nullptr, only the regions in the
   *   selections are used.
   */
  void prepare_audio_regions_for_stretch (TimelineSelections * sel);

  /**
   * Stretches regions.
   *
   * @param selections If nullptr, all regions
   *   are used. If non-nullptr, only the regions in the
   *   selections are used.
   * @param with_fixed_ratio Stretch all regions with
   *   a fixed ratio. If this is off, the current
   *   region length and @ref Region.before_length
   *   will be used to calculate the ratio.
   * @param force Force stretching, regardless of
   *   musical mode.
   *
   * @throw ZrythmException if stretching fails.
   */
  void stretch_regions (
    TimelineSelections * sel,
    bool                 with_fixed_ratio,
    double               time_ratio,
    bool                 force);

  void set_punch_mode_enabled (bool enabled);

  void set_start_playback_on_midi_input (bool enabled);

  void set_recording_mode (RecordingMode mode);

  /**
   * Sets whether metronome is enabled or not.
   */
  void set_metronome_enabled (bool enabled);

  /**
   * Moves the playhead by the time corresponding to
   * given samples, taking into account the loop
   * end point.
   */
  void add_to_playhead (const signed_frame_t nframes);

  /**
   * Request pause.
   *
   * Must only be called in-between engine processing
   * calls.
   *
   * @param with_wait Wait for lock before requesting.
   */
  void request_pause (bool with_wait);

  /**
   * Request playback.
   *
   * Must only be called in-between engine processing
   * calls.
   *
   * @param with_wait Wait for lock before requesting.
   */
  void request_roll (bool with_wait);

  /**
   * Setter for playhead Position.
   */
  void set_playhead_pos (const Position pos);

  void set_playhead_to_bar (int bar);

  /**
   * Getter for playhead Position.
   */
  void get_playhead_pos (Position * pos);

  static void get_playhead_pos_static (Transport * self, Position * pos)
  {
    self->get_playhead_pos (pos);
  }

  /**
   * Move to the previous snap point on the timeline.
   */
  void move_backward (bool with_wait);

  /**
   * Move to the next snap point on the timeline.
   */
  void move_forward (bool with_wait);

  /**
   * Returns whether the user can currently move the playhead
   * (eg, via the UI or via scripts).
   */
  bool can_user_move_playhead () const;

  /**
   * Moves playhead to given pos.
   *
   * This is only for moves other than while playing and for looping while
   * playing.
   *
   * Should not be used during exporting.
   *
   * @param target Position to set to.
   * @param panic Send MIDI panic or not FIXME unused.
   * @param set_cue_point Also set the cue point at this position.
   */
  void move_playhead (
    const Position * target,
    bool             panic,
    bool             set_cue_point,
    bool             fire_events);

  /**
   * Enables or disables loop.
   */
  void set_loop (bool enabled, bool with_wait);

  /**
   * Moves the playhead to the start Marker.
   */
  void goto_start_marker ();

  /**
   * Moves the playhead to the end Marker.
   */
  void goto_end_marker ();

  /**
   * Moves the playhead to the prev Marker.
   */
  void goto_prev_marker ();

  /**
   * Moves the playhead to the next Marker.
   */
  void goto_next_marker ();

  /**
   * Updates the frames in all transport positions
   *
   * @param update_from_ticks Whether to update the positions based on ticks
   * (true) or frames (false).
   */
  void update_positions (bool update_from_ticks);

#if 0
/**
 * Adds frames to the given global frames, while
 * adjusting the new frames to loop back if the
 * loop point was crossed.
 *
 * @return The new frames adjusted.
 */
long
frames_add_frames (
  const long        gframes,
  const nframes_t   frames);
#endif

  /**
   * Adds frames to the given position similar to position_add_frames, except
   * that it adjusts the new Position if the loop end point was crossed.
   */
  void position_add_frames (Position * pos, const signed_frame_t frames);

  /**
   * Returns the PPQN (Parts/Ticks Per Quarter Note).
   */
  double get_ppqn () const;

  /**
   * Stores the position of the range in @ref pos.
   */
  std::pair<Position, Position> get_range_positions () const;

  /**
   * Stores the position of the range in @ref pos.
   */
  std::pair<Position, Position> get_loop_range_positions () const;

  /**
   * Sets if the project has range and updates UI.
   */
  void set_has_range (bool has_range);

  /**
   * Set the range1 or range2 position.
   *
   * @param range1 True to set range1, false to set range2.
   */
  void set_range (
    bool             range1,
    const Position * start_pos,
    const Position * pos,
    bool             snap);

  /**
   * Set the loop range.
   *
   * @param start True to set start pos, false to set end pos.
   */
  void set_loop_range (
    bool             start,
    const Position * start_pos,
    const Position * pos,
    bool             snap);

  /**
   * Returns the number of processable frames until and excluding the loop end
   * point as a positive number (>= 1) if the loop point was met between
   * g_start_frames and (g_start_frames + nframes), otherwise returns 0;
   */
  [[nodiscard]] ATTR_HOT inline nframes_t
  is_loop_point_met (const signed_frame_t g_start_frames, const nframes_t nframes)
  {
    bool loop_end_between_start_and_end =
      (loop_end_pos_.frames_ > g_start_frames
       && loop_end_pos_.frames_ <= g_start_frames + (long) nframes);

    if (loop_ && loop_end_between_start_and_end) [[unlikely]]
      {
        return (nframes_t) (loop_end_pos_.frames_ - g_start_frames);
      }
    return 0;
  }

  bool position_is_inside_punch_range (const Position pos);

  /**
   * Recalculates the total bars based on the last object's position.
   *
   * @param sel If given, only these objects will be checked, otherwise every
   * object in the project will be checked.
   */
  void recalculate_total_bars (ArrangerSelections * sel);

  /**
   * Updates the total bars.
   */
  void update_total_bars (int total_bars, bool fire_events);

  void update_caches (int beats_per_bar, int beat_unit);

  /**
   * Sets recording on/off.
   */
  void set_recording (bool record, bool with_wait, bool fire_events);

  void init_after_cloning (const Transport &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_common ();

  /**
   * One of @param marker or @param pos must be non-NULL.
   */
  void move_to_marker_or_pos_and_fire_events (
    const Marker *   marker,
    const Position * pos);

  static void
  foreach_arranger_handle_playhead_auto_scroll (ArrangerWidget * arranger);

public:
  /** Total bars in the song. */
  int total_bars_ = 0;

  /** Playhead position. */
  Position playhead_pos_;

  /** Cue point position. */
  Position cue_pos_;

  /** Loop start position. */
  Position loop_start_pos_;

  /** Loop end position. */
  Position loop_end_pos_;

  /** Punch in position. */
  Position punch_in_pos_;

  /** Punch out position. */
  Position punch_out_pos_;

  /**
   * Selected range.
   *
   * This is 2 points instead of start/end because the 2nd point can be dragged
   * past the 1st point so the order gets swapped.
   *
   * Should be compared each time to see which one is first.
   */
  Position range_1_;
  Position range_2_;

  /** Whether range should be displayed or not. */
  bool has_range_ = false;

  // TimeSignature time_sig;

  /* ---------- CACHE -------------- */
  int ticks_per_beat_ = 0;
  int ticks_per_bar_ = 0;
  int sixteenths_per_beat_ = 0;
  int sixteenths_per_bar_ = 0;

  /* ------------------------------- */

  /** Transport position in frames.
   * FIXME is this used? */
  nframes_t position_ = 0;

  /** Looping or not. */
  bool loop_ = false;

  /** Whether punch in/out mode is enabled. */
  bool punch_mode_ = false;

  /** Whether MIDI/audio recording is enabled (recording toggle in transport
   * bar). */
  bool recording_ = false;

  /** Metronome enabled or not. */
  bool metronome_enabled_ = false;

  /** Recording preroll frames remaining. */
  signed_frame_t preroll_frames_remaining_ = 0;

  /** Metronome countin frames remaining. */
  signed_frame_t countin_frames_remaining_ = 0;

  /** Whether to start playback on MIDI input. */
  bool start_playback_on_midi_input_ = false;

  RecordingMode recording_mode_ = (RecordingMode) 0;

  /**
   * Position of the playhead before pausing.
   *
   * Used by UndoableAction.
   */
  Position playhead_before_pause_;

  /**
   * Roll/play MIDI port.
   *
   * Any event received on this port will request a roll.
   */
  std::unique_ptr<MidiPort> roll_;

  /**
   * Stop MIDI port.
   *
   * Any event received on this port will request a stop/pause.
   */
  std::unique_ptr<MidiPort> stop_;

  /** Backward MIDI port. */
  std::unique_ptr<MidiPort> backward_;

  /** Forward MIDI port. */
  std::unique_ptr<MidiPort> forward_;

  /** Loop toggle MIDI port. */
  std::unique_ptr<MidiPort> loop_toggle_;

  /** Rec toggle MIDI port. */
  std::unique_ptr<MidiPort> rec_toggle_;

  /** Play state. */
  PlayState play_state_ = {};

  /** Last timestamp the playhead position was changed manually. */
  RtTimePoint last_manual_playhead_change_;

  /** Pointer to owner audio engine, if any. */
  AudioEngine * audio_engine_ = nullptr;
};

/**
 * @}
 */

#endif
