// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/itransport.h"
#include "dsp/playhead.h"
#include "dsp/playhead_qml_adapter.h"
#include "dsp/position.h"
#include "gui/backend/position_proxy.h"
#include "gui/dsp/midi_port.h"
#include "gui/dsp/port.h"
#include "structure/arrangement/arranger_object_span.h"
#include "utils/types.h"

class Project;

namespace zrythm::structure::arrangement
{
class Marker;
}

#define TRANSPORT (PROJECT->transport_)
constexpr int TRANSPORT_DEFAULT_TOTAL_BARS = 128;

namespace zrythm::engine::session
{
enum class PrerollCountBars
{
  PREROLL_COUNT_BARS_NONE,
  PREROLL_COUNT_BARS_ONE,
  PREROLL_COUNT_BARS_TWO,
  PREROLL_COUNT_BARS_FOUR,
};

static const char * preroll_count_bars_str[] = {
  QT_TR_NOOP_UTF8 ("None"),
  QT_TR_NOOP_UTF8 ("1 bar"),
  QT_TR_NOOP_UTF8 ("2 bars"),
  QT_TR_NOOP_UTF8 ("4 bars"),
};

/**
 * The Transport class represents the transport controls and state for an audio
 * engine. It manages playback, recording, and other transport-related
 * functionality.
 */
class Transport final : public QObject, public dsp::ITransport, public IPortOwner
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    bool loopEnabled READ is_looping WRITE setLoopEnabled NOTIFY
      loopEnabledChanged)
  Q_PROPERTY (
    bool recordEnabled READ is_recording WRITE setRecordEnabled NOTIFY
      recordEnabledChanged)
  Q_PROPERTY (
    PlayState playState READ getPlayState WRITE setPlayState NOTIFY
      playStateChanged)
  Q_PROPERTY (dsp::PlayheadQmlWrapper * playhead READ getPlayhead CONSTANT)
  Q_PROPERTY (PositionProxy * cuePosition READ getCuePosition CONSTANT)
  Q_PROPERTY (
    PositionProxy * loopStartPosition READ getLoopStartPosition CONSTANT)
  Q_PROPERTY (PositionProxy * loopEndPosition READ getLoopEndPosition CONSTANT)
  Q_PROPERTY (PositionProxy * punchInPosition READ getPunchInPosition CONSTANT)
  Q_PROPERTY (PositionProxy * punchOutPosition READ getPunchOutPosition CONSTANT)

public:
  Q_ENUM (PlayState)

  /**
   * Corrseponts to "transport-display" in the
   * gsettings.
   */
  enum class Display
  {
    BBT,
    Time,
  };
  Q_ENUM (Display)

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
  Q_ENUM (RecordingMode)

  using Position = zrythm::dsp::Position;
  using PortIdentifier = zrythm::dsp::PortIdentifier;
  using PortFlow = zrythm::dsp::PortFlow;

public:
  Transport (Project * parent = nullptr);

  // ==================================================================
  // QML Interface
  // ==================================================================

  void          setLoopEnabled (bool enabled);
  Q_SIGNAL void loopEnabledChanged (bool enabled);

  void          setRecordEnabled (bool enabled);
  Q_SIGNAL void recordEnabledChanged (bool enabled);

  PlayState     getPlayState () const;
  void          setPlayState (PlayState state);
  Q_SIGNAL void playStateChanged (PlayState state);

  dsp::PlayheadQmlWrapper * getPlayhead () const;

  PositionProxy * getCuePosition () const;

  PositionProxy * getLoopStartPosition () const;

  PositionProxy * getLoopEndPosition () const;

  PositionProxy * getPunchInPosition () const;

  PositionProxy * getPunchOutPosition () const;

  Q_INVOKABLE void moveBackward ();
  Q_INVOKABLE void moveForward ();

  // ==================================================================

  static const char * preroll_count_to_str (PrerollCountBars bars)
  {
    return preroll_count_bars_str[static_cast<int> (bars)];
  }

  static int preroll_count_bars_enum_to_int (PrerollCountBars bars)
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

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  utils::Utf8String
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  Q_INVOKABLE bool isRolling () const
  {
    return play_state_ == PlayState::Rolling;
  }

  Q_INVOKABLE bool isPaused () const
  {
    return play_state_ == PlayState::Paused;
  }

  bool is_looping () const { return loop_; }

  bool is_recording () const { return recording_; }
  /**
   * Initialize loaded transport.
   *
   * @param engine Owner engine, if any.
   * @param tempo_track Tempo track, used to initialize the caches. Only needed
   * on the active project transport.
   */
  void init_loaded (Project * project);

  /**
   * Prepares audio regions for stretching (sets the
   * @ref Region.before_length).
   *
   * @param selections If nullptr, all audio regions
   *   are used. If non-nullptr, only the regions in the
   *   selections are used.
   */
  void prepare_audio_regions_for_stretch (
    std::optional<structure::arrangement::ArrangerObjectSpan> sel_var);

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
    std::optional<structure::arrangement::ArrangerObjectSpan> sel_var,
    bool                                                      with_fixed_ratio,
    double                                                    time_ratio,
    bool                                                      force);

  void set_punch_mode_enabled (bool enabled);

  void set_start_playback_on_midi_input (bool enabled);

  void set_recording_mode (RecordingMode mode);

  /**
   * Sets whether metronome is enabled or not.
   */
  void set_metronome_enabled (bool enabled);

  /**
   * Moves the playhead by the time corresponding to given samples, taking into
   * account the loop end point.
   */
  void add_to_playhead_in_audio_thread (signed_frame_t nframes);

  /**
   * Request pause.
   *
   * Must only be called in-between engine processing
   * calls.
   *
   * @param with_wait Wait for lock before requesting.
   */
  Q_INVOKABLE void requestPause (bool with_wait);

  /**
   * Request playback.
   *
   * Must only be called in-between engine processing
   * calls.
   *
   * @param with_wait Wait for lock before requesting.
   */
  Q_INVOKABLE void requestRoll (bool with_wait);

  void set_play_state_rt_safe (PlayState state);

  signed_frame_t get_playhead_position_in_audio_thread () const override
  {
    return static_cast<signed_frame_t> (
      playhead_.position_during_processing_rounded ());
  }

  [[deprecated (
    "Use the Playhead API to get ticks instead. Optionally convert via the TempoMap API")]]
  Position get_playhead_position_in_gui_thread () const;

  nframes_t is_loop_point_met_in_audio_thread (
    const signed_frame_t g_start_frames,
    const nframes_t      nframes) const override
  {
    auto [loop_start_pos, loop_end_pos] = get_loop_range_positions ();
    bool loop_end_between_start_and_end =
      (loop_end_pos > g_start_frames
       && loop_end_pos <= g_start_frames + (long) nframes);

    if (loop_end_between_start_and_end && get_loop_enabled ()) [[unlikely]]
      {
        return (nframes_t) (loop_end_pos - g_start_frames);
      }
    return 0;
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
   * @param target_ticks Position to set to.
   * @param panic Send MIDI panic or not FIXME unused.
   * @param set_cue_point Also set the cue point at this position.
   */
  void move_playhead (
    double target_ticks,
    bool   panic,
    bool   set_cue_point,
    bool   fire_events);

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

  signed_frame_t get_playhead_position_after_adding_frames_in_audio_thread (
    signed_frame_t frames) const override;

  /**
   * Returns the PPQN (Parts/Ticks Per Quarter Note).
   */
  double get_ppqn () const;

  /**
   * Returns the selected range positions.
   */
  std::pair<Position, Position> get_range_positions () const;

  std::pair<signed_frame_t, signed_frame_t>
  get_loop_range_positions () const override;

  PlayState get_play_state () const override { return play_state_; }

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

  bool position_is_inside_punch_range (Position pos);

  bool get_loop_enabled () const override { return loop_; }

  /**
   * Recalculates the total bars based on the last object's position.
   *
   * @param sel If given, only these objects will be checked, otherwise every
   * object in the project will be checked.
   *
   * FIXME: use signals to update the total bars.
   */
  void recalculate_total_bars (
    std::optional<structure::arrangement::ArrangerObjectSpan> objects =
      std::nullopt);

  /**
   * Updates the total bars.
   */
  void update_total_bars (int total_bars, bool fire_events);

  void update_caches (int beats_per_bar, int beat_unit);

  /**
   * Sets recording on/off.
   */
  void set_recording (bool record, bool with_wait);

  friend void init_from (
    Transport             &obj,
    const Transport       &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kTotalBarsKey = "totalBars"sv;
  static constexpr auto kPlayheadKey = "playhead"sv;
  static constexpr auto kCuePosKey = "cuePos"sv;
  static constexpr auto kLoopStartPosKey = "loopStartPos"sv;
  static constexpr auto kLoopEndPosKey = "loopEndPos"sv;
  static constexpr auto kPunchInPosKey = "punchInPos"sv;
  static constexpr auto kPunchOutPosKey = "punchOutPos"sv;
  static constexpr auto kRange1Key = "range1"sv;
  static constexpr auto kRange2Key = "range2"sv;
  static constexpr auto kHasRangeKey = "hasRange"sv;
  static constexpr auto kPositionKey = "position"sv;
  static constexpr auto kRollKey = "roll"sv;
  static constexpr auto kStopKey = "stop"sv;
  static constexpr auto kBackwardKey = "backward"sv;
  static constexpr auto kForwardKey = "forward"sv;
  static constexpr auto kLoopToggleKey = "loopToggle"sv;
  static constexpr auto kRecToggleKey = "recToggle"sv;
  friend void           to_json (nlohmann::json &j, const Transport &transport)
  {
    j = nlohmann::json{
      { kTotalBarsKey,    transport.total_bars_     },
      { kPlayheadKey,     transport.playhead_       },
      { kCuePosKey,       transport.cue_pos_        },
      { kLoopStartPosKey, transport.loop_start_pos_ },
      { kLoopEndPosKey,   transport.loop_end_pos_   },
      { kPunchInPosKey,   transport.punch_in_pos_   },
      { kPunchOutPosKey,  transport.punch_out_pos_  },
      { kRange1Key,       transport.range_1_        },
      { kRange2Key,       transport.range_2_        },
      { kHasRangeKey,     transport.has_range_      },
      { kPositionKey,     transport.position_       },
      { kRollKey,         transport.roll_           },
      { kStopKey,         transport.stop_           },
      { kBackwardKey,     transport.backward_       },
      { kForwardKey,      transport.forward_        },
      { kLoopToggleKey,   transport.loop_toggle_    },
      { kRecToggleKey,    transport.rec_toggle_     },
    };
  }
  friend void from_json (const nlohmann::json &j, Transport &transport)
  {
    j.at (kTotalBarsKey).get_to (transport.total_bars_);
    j.at (kPlayheadKey).get_to (transport.playhead_);
    j.at (kCuePosKey).get_to (*transport.cue_pos_);
    j.at (kLoopStartPosKey).get_to (*transport.loop_start_pos_);
    j.at (kLoopEndPosKey).get_to (*transport.loop_end_pos_);
    j.at (kPunchInPosKey).get_to (*transport.punch_in_pos_);
    j.at (kPunchOutPosKey).get_to (*transport.punch_out_pos_);
    j.at (kRange1Key).get_to (transport.range_1_);
    j.at (kRange2Key).get_to (transport.range_2_);
    j.at (kHasRangeKey).get_to (transport.has_range_);
    j.at (kPositionKey).get_to (transport.position_);
    j.at (kRollKey).get_to (transport.roll_);
    j.at (kStopKey).get_to (transport.stop_);
    j.at (kBackwardKey).get_to (transport.backward_);
    j.at (kForwardKey).get_to (transport.forward_);
    j.at (kLoopToggleKey).get_to (transport.loop_toggle_);
    j.at (kRecToggleKey).get_to (transport.rec_toggle_);
  }

  void init_common ();

  /**
   * One of @param marker or @param pos must be non-NULL.
   */
  void move_to_marker_or_pos_and_fire_events (
    const structure::arrangement::Marker * marker,
    std::optional<double>                  pos_ticks);

  // static void
  // foreach_arranger_handle_playhead_auto_scroll (ArrangerWidget * arranger);

public:
  /** Total bars in the song. */
  int total_bars_ = 0;

  /** Playhead position. */
  dsp::Playhead                                    playhead_;
  utils::QObjectUniquePtr<dsp::PlayheadQmlWrapper> playhead_adapter_;

  /** Cue point position. */
  PositionProxy * cue_pos_ = nullptr;

  /** Loop start position. */
  PositionProxy * loop_start_pos_ = nullptr;

  /** Loop end position. */
  PositionProxy * loop_end_pos_ = nullptr;

  /** Punch in position. */
  PositionProxy * punch_in_pos_ = nullptr;

  /** Punch out position. */
  PositionProxy * punch_out_pos_ = nullptr;

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
   * Position of the playhead before pausing, in ticks.
   *
   * Used by UndoableAction.
   */
  double playhead_before_pause_{};

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
  RtTimePoint last_manual_playhead_change_ = 0;

  /** Pointer to owner, if any. */
  Project * project_ = nullptr;

private:
  /**
   * @brief Timer used to notify the property system of changes (e.g.
   * playhead position).
   *
   * This is used to avoid Q_EMIT on realtime threads because Q_EMIT is not
   * realtime safe.
   */
  QTimer *          property_notification_timer_ = nullptr;
  std::atomic<bool> needs_property_notification_{ false };
};
}
