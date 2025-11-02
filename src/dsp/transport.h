// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/itransport.h"
#include "dsp/playhead_qml_adapter.h"
#include "dsp/snap_grid.h"
#include "utils/icloneable.h"

namespace zrythm::dsp
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
  QML_UNCREATABLE ("")

  /** Millisec to allow moving further backward when very close to the
   * calculated backward position. */
  static constexpr auto REPEATED_BACKWARD_MS = au::milli (units::seconds) (240);

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
    const dsp::TempoMap &tempo_map,
    const dsp::SnapGrid &snap_grid,
    ConfigProvider       config_provider,
    QObject *            parent = nullptr);

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

  Q_INVOKABLE bool isRolling () const
  {
    return play_state_ == PlayState::Rolling;
  }

  Q_INVOKABLE bool isPaused () const
  {
    return play_state_ == PlayState::Paused;
  }

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
   * @brief Moves the playhead to the previous or next marker.
   *
   * @param prev True for previous, false for next.
   */
  void goto_prev_or_next_marker (
    bool                                  prev,
    RangeOf<units::precise_tick_t> auto &&extra_markers)
  {
    /* gather all markers */
    std::vector<units::precise_tick_t> marker_ticks;
    marker_ticks.append_range (extra_markers);
    marker_ticks.emplace_back (cue_position_.get_ticks ());
    marker_ticks.emplace_back (loop_start_position_.get_ticks ());
    marker_ticks.emplace_back (loop_end_position_.get_ticks ());
    marker_ticks.emplace_back ();
    std::ranges::sort (marker_ticks);

    if (prev)
      {
        // Iterate backwards through marker_ticks with manual index.
        // Equivalent to (but can't use enumerate yet on AppleClang):
        // marker_ticks | std::views::enumerate | std::views::reverse
        for (size_t i = 0; i < marker_ticks.size (); ++i)
          {
            const auto  index = marker_ticks.size () - 1 - i;
            const auto &marker_tick = marker_ticks[index];

            if (marker_tick >= playhead_.position_ticks ())
              continue;

            if (
              isRolling () && index > 0
              && (playhead_.get_tempo_map ().tick_to_seconds (
                    playhead_.position_ticks ())
                  - playhead_.get_tempo_map ().tick_to_seconds (marker_tick))
                   < REPEATED_BACKWARD_MS)
              {
                continue;
              }

            move_playhead (marker_tick, true);
            break;
          }
      }
    else
      {
        for (auto &marker : marker_ticks)
          {
            if (marker > playhead_.position_ticks ())
              {
                move_playhead (marker, true);
                break;
              }
          }
      }
  }

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

  auto playhead_ticks_before_pause () const [[clang::blocking]]
  {
    return playhead_before_pause_;
  }

  /**
   * @brief For engine use only.
   *
   * @param samples Samples to consume.
   */
  void consume_metronome_countin_samples (units::sample_t samples)
  {
    assert (countin_frames_remaining_ >= samples);
    countin_frames_remaining_ -= samples;
  }

  /**
   * @brief For engine use only.
   *
   * @param samples Samples to consume.
   */
  void consume_recording_preroll_samples (units::sample_t samples)
  {
    assert (recording_preroll_frames_remaining_ >= samples);
    recording_preroll_frames_remaining_ -= samples;
  }

  friend void init_from (
    Transport             &obj,
    const Transport       &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kPlayheadKey = "playhead"sv;
  static constexpr auto kCuePosKey = "cuePos"sv;
  static constexpr auto kLoopStartPosKey = "loopStartPos"sv;
  static constexpr auto kLoopEndPosKey = "loopEndPos"sv;
  static constexpr auto kPunchInPosKey = "punchInPos"sv;
  static constexpr auto kPunchOutPosKey = "punchOutPos"sv;
  static constexpr auto kPositionKey = "position"sv;
  friend void           to_json (nlohmann::json &j, const Transport &transport);
  friend void from_json (const nlohmann::json &j, Transport &transport);

  /**
   * Returns whether the user can currently move the playhead
   * (eg, via the UI or via scripts).
   */
  bool can_user_move_playhead () const;

private:
  /** Playhead position. */
  dsp::Playhead                                    playhead_;
  utils::QObjectUniquePtr<dsp::PlayheadQmlWrapper> playhead_adapter_;

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

  /**
   * Position of the playhead before pausing, in ticks.
   *
   * Used by UndoableAction.
   */
  units::precise_tick_t playhead_before_pause_;

  /** Play state. */
  PlayState play_state_{ PlayState::Paused };

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

  const dsp::SnapGrid &snap_grid_;
};
}
