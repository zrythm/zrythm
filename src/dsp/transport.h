// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/itransport.h"
#include "dsp/playhead_qml_adapter.h"
#include "dsp/position.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "utils/icloneable.h"
#include "utils/views.h"

#include <farbot/RealtimeObject.hpp>

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
class Transport : public QObject
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
    zrythm::dsp::ITransport::PlayState playState READ getPlayState WRITE
      setPlayState NOTIFY playStateChanged)
  Q_PROPERTY (zrythm::dsp::PlayheadQmlWrapper * playhead READ playhead CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::TimelinePosition * cuePosition READ cuePosition CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::TimelinePosition * loopStartPosition READ loopStartPosition
      CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::TimelinePosition * loopEndPosition READ loopEndPosition CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::TimelinePosition * punchInPosition READ punchInPosition CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::TimelinePosition * punchOutPosition READ punchOutPosition
      CONSTANT)
  QML_UNCREATABLE ("")

  /** Millisec to allow moving further backward when very close to the
   * calculated backward position. */
  static constexpr auto REPEATED_BACKWARD_MS = au::milli (units::seconds) (240);

public:
  using PlayState = dsp::ITransport::PlayState;
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
     * Overwrite events in first recorded clip.
     *
     * In the case of MIDI, this will remove existing MIDI notes during
     * recording.
     *
     * In the case of audio, this will act exactly the same as @ref
     * RECORDING_MODE_TAKES_MUTED.
     */
    OverwriteEvents,

    /**
     * Merge events in existing clip.
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

  class TransportSnapshot : public dsp::ITransport
  {
  public:
    TransportSnapshot (
      std::pair<units::sample_t, units::sample_t> loop_range,
      std::pair<units::sample_t, units::sample_t> punch_range,
      units::sample_t                             playhead_position,
      units::sample_t recording_preroll_frames_remaining,
      units::sample_t metronome_countin_frames_remaining,
      PlayState       play_state,
      bool            loop_enabled,
      bool            punch_enabled,
      bool            recording_enabled)
        : loop_range_ (loop_range), punch_range_ (punch_range),
          playhead_position_ (playhead_position),
          recording_preroll_frames_remaining_ (recording_preroll_frames_remaining),
          metronome_countin_frames_remaining_ (metronome_countin_frames_remaining),
          play_state_ (play_state), loop_enabled_ (loop_enabled),
          punch_enabled_ (punch_enabled), recording_enabled_ (recording_enabled)
    {
    }

    std::pair<units::sample_t, units::sample_t>
    get_loop_range_positions () const noexcept override
    {
      return loop_range_;
    }
    std::pair<units::sample_t, units::sample_t>
    get_punch_range_positions () const noexcept override
    {
      return punch_range_;
    }
    PlayState get_play_state () const noexcept override { return play_state_; }
    units::sample_t
    get_playhead_position_in_audio_thread () const noexcept override
    {
      return playhead_position_;
    }
    bool loop_enabled () const noexcept override { return loop_enabled_; }

    bool punch_enabled () const noexcept override { return punch_enabled_; }
    bool recording_enabled () const noexcept override
    {
      return recording_enabled_;
    }
    units::sample_t
    recording_preroll_frames_remaining () const noexcept override
    {
      return recording_preroll_frames_remaining_;
    }
    units::sample_t
    metronome_countin_frames_remaining () const noexcept override
    {
      return metronome_countin_frames_remaining_;
    }

    void set_play_state (dsp::ITransport::PlayState play_state)
    {
      play_state_ = play_state;
    }
    void set_position (units::sample_t position)
    {
      playhead_position_ = position;
    }
    void consume_metronome_countin_samples (units::sample_t samples)
    {
      metronome_countin_frames_remaining_ -= samples;
    }
    void consume_recording_preroll_samples (units::sample_t samples)
    {
      recording_preroll_frames_remaining_ -= samples;
    }

  private:
    std::pair<units::sample_t, units::sample_t> loop_range_;
    std::pair<units::sample_t, units::sample_t> punch_range_;
    units::sample_t                             playhead_position_;
    units::sample_t recording_preroll_frames_remaining_;
    units::sample_t metronome_countin_frames_remaining_;
    PlayState       play_state_;
    bool            loop_enabled_;
    bool            punch_enabled_;
    bool            recording_enabled_;
  };

  Transport (
    const dsp::TempoMapWrapper &tempo_map_wrapper,
    ConfigProvider              config_provider,
    QObject *                   parent = nullptr);

  // ==================================================================
  // QML Interface
  // ==================================================================

  bool          loopEnabled () const { return loop_; }
  void          setLoopEnabled (bool enabled);
  Q_SIGNAL void loopEnabledChanged (bool enabled);

  bool          recordEnabled () const { return recording_; }
  void          setRecordEnabled (bool enabled);
  Q_SIGNAL void recordEnabledChanged (bool enabled);

  bool          punchEnabled () const { return punch_mode_; }
  void          setPunchEnabled (bool enabled);
  Q_SIGNAL void punchEnabledChanged (bool enabled);

  PlayState     getPlayState () const { return play_state_; }
  void          setPlayState (PlayState state);
  Q_SIGNAL void playStateChanged (PlayState state);

  dsp::PlayheadQmlWrapper * playhead () const
  {
    return playhead_adapter_.get ();
  }
  dsp::TimelinePosition * cuePosition () const { return cue_position_.get (); }
  dsp::TimelinePosition * loopStartPosition () const
  {
    return loop_start_position_.get ();
  }
  dsp::TimelinePosition * loopEndPosition () const
  {
    return loop_end_position_.get ();
  }
  dsp::TimelinePosition * punchInPosition () const
  {
    return punch_in_position_.get ();
  }
  dsp::TimelinePosition * punchOutPosition () const
  {
    return punch_out_position_.get ();
  }

  /**
   * Request pause.
   */
  Q_INVOKABLE void requestPause () [[clang::blocking]];

  /**
   * Request playback.
   */
  Q_INVOKABLE void requestRoll () [[clang::blocking]];

  /**
   * @brief Moves the playhead to the given tick position.
   *
   * This is intended for user-initiated playhead moves (e.g., clicking on the
   * ruler).
   *
   * @param ticks The target position in ticks.
   * @param setCuePoint If true, also sets the cue position to this position.
   */
  Q_INVOKABLE void movePlayhead (double ticks, bool setCuePoint);

  // ==================================================================

  // ==================================================================
  // Audio-thread-safe accessors
  // ==================================================================

  units::sample_t
  get_playhead_position_in_audio_thread () const noexcept [[clang::nonblocking]]
  {
    return playhead_.position_during_processing_rounded ();
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
  void add_to_playhead_in_audio_thread (
    const dsp::Transport::TransportSnapshot &transport_snapshot,
    units::sample_t nframes) noexcept [[clang::nonblocking]];

  void set_play_state_rt_safe (PlayState state);

  /**
   * Moves playhead to given pos.
   *
   * This is only for moves other than while playing and for looping while
   * playing. For example it should be used for moves when the user clicks on a
   * position in the ruler.
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
    bool                                         prev,
    utils::RangeOf<units::precise_tick_t> auto &&extra_markers)
  {
    /* gather all markers */
    std::vector<units::precise_tick_t> marker_ticks;
    static_assert (__cpp_lib_containers_ranges >= 202202L);
    marker_ticks.append_range (extra_markers);
    marker_ticks.emplace_back (units::ticks (cue_position_->ticks ()));
    marker_ticks.emplace_back (units::ticks (loop_start_position_->ticks ()));
    marker_ticks.emplace_back (units::ticks (loop_end_position_->ticks ()));
    marker_ticks.emplace_back ();
    std::ranges::sort (marker_ticks);

    if (prev)
      {
        for (
          const auto &[index, marker_tick] :
          marker_ticks | utils::views::enumerate | std::views::reverse)
          {
            if (marker_tick >= playhead_.position_ticks ())
              continue;

            if (
              isRolling () && index > 0
              && (playhead_.get_tempo_map ().tick_to_seconds (
                    TimelineTick{ playhead_.position_ticks () })
                  - playhead_.get_tempo_map ().tick_to_seconds (
                    TimelineTick{ marker_tick }))
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
        for (const auto &marker : marker_ticks)
          {
            if (marker > playhead_.position_ticks ())
              {
                move_playhead (marker, true);
                break;
              }
          }
      }
  }

  bool position_is_inside_punch_range (units::sample_t pos) const;

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

  auto get_snapshot () const noexcept [[clang::nonblocking]]
  {
    decltype (rt_markers_)::ScopedAccess<farbot::ThreadType::realtime> access{
      rt_markers_
    };
    return TransportSnapshot{
      { units::samples (access->loop_start_samples),
       units::samples (access->loop_end_samples)  },
      { units::samples (access->punch_in_samples),
       units::samples (access->punch_out_samples) },
      get_playhead_position_in_audio_thread (),
      recording_preroll_frames_remaining_,
      countin_frames_remaining_,
      play_state_,
      loop_,
      punch_mode_,
      recording_
    };
  }

  friend void init_from (
    Transport             &obj,
    const Transport       &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kPlayheadKey = "playheadPosition"sv;
  static constexpr auto kCuePosKey = "cuePosition"sv;
  static constexpr auto kLoopStartPosKey = "loopStartPosition"sv;
  static constexpr auto kLoopEndPosKey = "loopEndPosition"sv;
  static constexpr auto kPunchInPosKey = "punchInPosition"sv;
  static constexpr auto kPunchOutPosKey = "punchOutPosition"sv;
  friend void           to_json (nlohmann::json &j, const Transport &transport);
  friend void from_json (const nlohmann::json &j, Transport &transport);

  /**
   * Returns whether the user can currently move the playhead
   * (eg, via the UI or via scripts).
   */
  bool can_user_move_playhead () const;

  /** Pre-computed marker sample positions for lock-free audio-thread reads. */
  struct MarkerSnapshot
  {
    int64_t loop_start_samples{};
    int64_t loop_end_samples{};
    int64_t punch_in_samples{};
    int64_t punch_out_samples{};
  };

  void update_rt_marker_snapshot ();

private:
  /** Playhead position. */
  dsp::Playhead                                    playhead_;
  utils::QObjectUniquePtr<dsp::PlayheadQmlWrapper> playhead_adapter_;

  /** Cue point position. */
  utils::QObjectUniquePtr<dsp::TimelinePosition> cue_position_;

  /** Loop start position. */
  utils::QObjectUniquePtr<dsp::TimelinePosition> loop_start_position_;

  /** Loop end position. */
  utils::QObjectUniquePtr<dsp::TimelinePosition> loop_end_position_;

  /** Punch in position. */
  utils::QObjectUniquePtr<dsp::TimelinePosition> punch_in_position_;

  /** Punch out position. */
  utils::QObjectUniquePtr<dsp::TimelinePosition> punch_out_position_;

  /**
   * @brief Pre-computed marker sample positions for audio-thread access.
   *
   * Updated on the main thread whenever a position changes, read on the
   * audio thread via get_snapshot().
   */
  mutable farbot::RealtimeObject<
    MarkerSnapshot,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    rt_markers_;

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
};
}
