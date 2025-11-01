// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/itransport.h"
#include "dsp/metronome.h"
#include "dsp/midi_port.h"
#include "dsp/playhead.h"
#include "dsp/playhead_qml_adapter.h"
#include "dsp/port.h"
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
  PrerollNone,
  PrerollOne,
  PrerollTwo,
  PrerollFour,
};

inline int
preroll_count_bars_enum_to_int (PrerollCountBars bars)
{
  switch (bars)
    {
    case PrerollCountBars::PrerollNone:
      return 0;
    case PrerollCountBars::PrerollOne:
      return 1;
    case PrerollCountBars::PrerollTwo:
      return 2;
    case PrerollCountBars::PrerollFour:
      return 4;
    }
  return -1;
}

/**
 * The Transport class represents the transport controls and state for an audio
 * engine. It manages playback, recording, and other transport-related
 * functionality.
 */
class Transport : public QObject, public dsp::ITransport
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    bool loopEnabled READ loopEnabled WRITE setLoopEnabled NOTIFY
      loopEnabledChanged)
  Q_PROPERTY (
    bool recordEnabled READ recordEnabled WRITE setRecordEnabled NOTIFY
      recordEnabledChanged)
  Q_PROPERTY (
    bool punchEnabled READ punchEnabled WRITE setPunchEnabled NOTIFY
      punchEnabledChanged)
  Q_PROPERTY (
    PlayState playState READ getPlayState WRITE setPlayState NOTIFY
      playStateChanged)
  Q_PROPERTY (zrythm::dsp::PlayheadQmlWrapper * playhead READ playhead CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * cuePosition READ cuePosition CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * loopStartPosition READ
      loopStartPosition CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * loopEndPosition READ loopEndPosition
      CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * punchInPosition READ punchInPosition
      CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * punchOutPosition READ
      punchOutPosition CONSTANT)
  Q_PROPERTY (zrythm::dsp::Metronome * metronome READ metronome CONSTANT)
  QML_UNCREATABLE ("")

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

public:
  struct ConfigProvider
  {
    /**
     * @brief Whether to return to cue position on pause.
     */
    std::function<bool ()> return_to_cue_on_pause_;

    /**
     * @brief Number of bars to count-in when requesting playback with metronome
     * enabled.
     */
    std::function<int ()> metronome_countin_bars_;

    /**
     * @brief Number of bars to pre-roll before recording.
     *
     * FIXME: add more details here.
     */
    std::function<int ()> recording_preroll_bars_;
  };

  Transport (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    const dsp::TempoMap                          &tempo_map,
    ConfigProvider                                config_provider,
    QObject *                                     parent = nullptr);

  // ==================================================================
  // QML Interface
  // ==================================================================

  bool          loopEnabled () const { return loop_enabled (); }
  void          setLoopEnabled (bool enabled);
  Q_SIGNAL void loopEnabledChanged (bool enabled);

  bool          recordEnabled () const { return recording_enabled (); }
  void          setRecordEnabled (bool enabled);
  Q_SIGNAL void recordEnabledChanged (bool enabled);

  bool          punchEnabled () const { return punch_enabled (); }
  void          setPunchEnabled (bool enabled);
  Q_SIGNAL void punchEnabledChanged (bool enabled);

  PlayState     getPlayState () const;
  void          setPlayState (PlayState state);
  Q_SIGNAL void playStateChanged (PlayState state);

  dsp::PlayheadQmlWrapper * playhead () const
  {
    return playhead_adapter_.get ();
  }
  dsp::AtomicPositionQmlAdapter * cuePosition () const
  {
    return cue_position_adapter_.get ();
  }
  dsp::AtomicPositionQmlAdapter * loopStartPosition () const
  {
    return loop_start_position_adapter_.get ();
  }
  dsp::AtomicPositionQmlAdapter * loopEndPosition () const
  {
    return loop_end_position_adapter_.get ();
  }
  dsp::AtomicPositionQmlAdapter * punchInPosition () const
  {
    return punch_in_position_adapter_.get ();
  }
  dsp::AtomicPositionQmlAdapter * punchOutPosition () const
  {
    return punch_out_position_adapter_.get ();
  }

  dsp::Metronome * metronome () const { return metronome_.get (); }

  Q_INVOKABLE void moveBackward () [[clang::blocking]];
  Q_INVOKABLE void moveForward () [[clang::blocking]];

  /**
   * Request pause.
   */
  Q_INVOKABLE void requestPause () [[clang::blocking]];

  /**
   * Request playback.
   */
  Q_INVOKABLE void requestRoll () [[clang::blocking]];

  // ==================================================================

  // ==================================================================
  // ITransport Interface
  // ==================================================================

  units::sample_t
  get_playhead_position_in_audio_thread () const noexcept override
  {
    return playhead_.position_during_processing_rounded ();
  }

  std::pair<units::sample_t, units::sample_t>
  get_loop_range_positions () const noexcept override
  {
    return std::make_pair (
      loop_start_position_.get_samples (), loop_end_position_.get_samples ());
  }

  std::pair<units::sample_t, units::sample_t>
  get_punch_range_positions () const noexcept override
  {
    return std::make_pair (
      punch_in_position_.get_samples (), punch_out_position_.get_samples ());
  }

  PlayState get_play_state () const noexcept override { return play_state_; }

  bool loop_enabled () const noexcept override { return loop_; }
  bool punch_enabled () const noexcept override { return punch_mode_; }
  bool recording_enabled () const noexcept override { return recording_; }
  units::sample_t recording_preroll_frames_remaining () const noexcept override
  {
    return recording_preroll_frames_remaining_;
  }
  units::sample_t metronome_countin_frames_remaining () const noexcept override
  {
    return countin_frames_remaining_;
  }

  // ==================================================================

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  Q_INVOKABLE bool isRolling () const
  {
    return play_state_ == PlayState::Rolling;
  }

  Q_INVOKABLE bool isPaused () const
  {
    return play_state_ == PlayState::Paused;
  }

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

  /**
   * Moves the playhead by the time corresponding to given samples, taking into
   * account the loop end point.
   */
  void add_to_playhead_in_audio_thread (units::sample_t nframes);

  void set_play_state_rt_safe (PlayState state);

  /**
   * Moves playhead to given pos.
   *
   * This is only for moves other than while playing and for looping while
   * playing.
   *
   * Should not be used during exporting.
   *
   * @param target_ticks Position to set to.
   * @param set_cue_point Also set the cue point at this position.
   */
  void move_playhead (units::precise_tick_t target_ticks, bool set_cue_point);

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
   * Set the loop range.
   *
   * @param start True to set start pos, false to set end pos.
   */
  void set_loop_range (
    bool                  start,
    units::precise_tick_t start_pos,
    units::precise_tick_t pos,
    bool                  snap);

  bool position_is_inside_punch_range (units::sample_t pos);

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
  static constexpr auto kPositionKey = "position"sv;
  static constexpr auto kRollKey = "roll"sv;
  static constexpr auto kStopKey = "stop"sv;
  static constexpr auto kBackwardKey = "backward"sv;
  static constexpr auto kForwardKey = "forward"sv;
  static constexpr auto kLoopToggleKey = "loopToggle"sv;
  static constexpr auto kRecToggleKey = "recToggle"sv;
  friend void           to_json (nlohmann::json &j, const Transport &transport);
  friend void from_json (const nlohmann::json &j, Transport &transport);

  /**
   * One of @param marker or @param pos must be non-NULL.
   */
  void move_to_marker_or_pos_and_fire_events (
    const structure::arrangement::Marker * marker,
    std::optional<units::precise_tick_t>   pos_ticks);

  // static void
  // foreach_arranger_handle_playhead_auto_scroll (ArrangerWidget * arranger);

  /**
   * Returns whether the user can currently move the playhead
   * (eg, via the UI or via scripts).
   */
  bool can_user_move_playhead () const;

public:
  /** Total bars in the song. */
  int total_bars_ = 0;

  /** Playhead position. */
  dsp::Playhead                                    playhead_;
  utils::QObjectUniquePtr<dsp::PlayheadQmlWrapper> playhead_adapter_;

  /** The metronome. */
  utils::QObjectUniquePtr<dsp::Metronome> metronome_;

  std::unique_ptr<dsp::AtomicPosition::TimeConversionFunctions>
    time_conversion_funcs_;

  /** Cue point position. */
  dsp::AtomicPosition                                    cue_position_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> cue_position_adapter_;

  /** Loop start position. */
  dsp::AtomicPosition loop_start_position_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter>
    loop_start_position_adapter_;

  /** Loop end position. */
  dsp::AtomicPosition loop_end_position_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter>
    loop_end_position_adapter_;

  /** Punch in position. */
  dsp::AtomicPosition punch_in_position_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter>
    punch_in_position_adapter_;

  /** Punch out position. */
  dsp::AtomicPosition punch_out_position_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter>
    punch_out_position_adapter_;

  /** Looping or not. */
  bool loop_{ true };

  /** Whether punch in/out mode is enabled. */
  bool punch_mode_{ false };

  /** Whether MIDI/audio recording is enabled (recording toggle in transport
   * bar). */
  bool recording_{ false };

  /** Recording preroll frames remaining. */
  units::sample_t recording_preroll_frames_remaining_;

  /** Metronome countin frames remaining. */
  units::sample_t countin_frames_remaining_;

  /** Whether to start playback on MIDI input. */
  bool start_playback_on_midi_input_ = false;

  /**
   * Position of the playhead before pausing, in ticks.
   *
   * Used by UndoableAction.
   */
  units::precise_tick_t playhead_before_pause_;

  /**
   * Roll/play MIDI port.
   *
   * Any event received on this port will request a roll.
   */
  std::unique_ptr<dsp::MidiPort> roll_;

  /**
   * Stop MIDI port.
   *
   * Any event received on this port will request a stop/pause.
   */
  std::unique_ptr<dsp::MidiPort> stop_;

  /** Backward MIDI port. */
  std::unique_ptr<dsp::MidiPort> backward_;

  /** Forward MIDI port. */
  std::unique_ptr<dsp::MidiPort> forward_;

  /** Loop toggle MIDI port. */
  std::unique_ptr<dsp::MidiPort> loop_toggle_;

  /** Rec toggle MIDI port. */
  std::unique_ptr<dsp::MidiPort> rec_toggle_;

private:
  /** Play state. */
  PlayState play_state_{ PlayState::Paused };

  /** Last timestamp the playhead position was changed manually. */
  RtTimePoint last_manual_playhead_change_ = 0;

  /**
   * @brief Timer used to notify the property system of changes (e.g.
   * playhead position).
   *
   * This is used to avoid Q_EMIT on realtime threads because Q_EMIT is not
   * realtime safe.
   */
  QTimer *          property_notification_timer_ = nullptr;
  std::atomic<bool> needs_property_notification_{ false };

  ConfigProvider config_provider_;
};
}
