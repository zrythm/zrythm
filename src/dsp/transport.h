// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>

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
      assert (loop_range_.first >= units::samples (0));
      assert (loop_range_.second >= loop_range_.first);
      assert (punch_range_.first >= units::samples (0));
      assert (punch_range_.second >= punch_range_.first);
      assert (playhead_position_ >= units::samples (0));
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
   * @brief Requests a pause without moving the playhead.
   *
   * Unlike requestPause(), this does not record the pre-pause playhead
   * position and does not move the playhead to the cue point when
   * return-to-cue is enabled.
   */
  void pause_for_engine_internal () [[clang::blocking]];

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

  /**
   * @brief Effective play state as last set by the audio thread.
   *
   * Unlike getPlayState(), which reflects the latest state known on the main
   * thread (including pending requests), this returns the audio thread's
   * state directly. Safe to call from any thread, including while the main
   * thread event loop is blocked.
   *
   * @note Between the audio thread latching a request and settling it, this
   * returns the requested state (e.g., PauseRequested until the pause is
   * applied).
   *
   * @note The engine's flush cycle also sets this from the main thread
   * while holding the processing lock.
   */
  PlayState effective_rt_play_state () const noexcept [[clang::nonblocking]]
  {
    return rt_feedback_.load ().state;
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

  /**
   * @brief Sets the effective play state from the audio thread.
   *
   * The main thread is notified via the property notification timer.
   *
   * @warning Same calling contract as get_snapshot(): audio thread, or main
   * thread while the engine is stopped and holding the processing lock.
   */
  void set_play_state_rt_safe (PlayState state) noexcept [[clang::nonblocking]];

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
   * @brief For engine use only (audio thread).
   *
   * @param samples Samples to consume.
   */
  void consume_metronome_countin_samples (units::sample_t samples) noexcept
    [[clang::nonblocking]]
  {
    assert (rt_countin_frames_remaining_ >= samples);
    rt_countin_frames_remaining_ -= samples;
  }

  /**
   * @brief For engine use only (audio thread).
   *
   * @param samples Samples to consume.
   */
  void consume_recording_preroll_samples (units::sample_t samples) noexcept
    [[clang::nonblocking]]
  {
    assert (rt_preroll_frames_remaining_ >= samples);
    rt_preroll_frames_remaining_ -= samples;
  }

  /**
   * @brief Returns a snapshot of the current transport state for processing.
   *
   * Latches any newly requested state published by the main thread (hence
   * non-const).
   *
   * @warning Must only be called from the audio thread, or from the main
   * thread while the engine is stopped and holding the processing lock
   * (e.g., the flush cycle). The audio callback reaches this even without
   * acquiring the processing lock, so only the engine being stopped
   * excludes it - the lock alone does not.
   */
  auto get_snapshot () noexcept [[clang::nonblocking]]
  {
    decltype (rt_state_)::ScopedAccess<farbot::ThreadType::realtime> access{
      rt_state_
    };

    // Latch newly requested play state / countin / preroll
    if (access->request_generation != rt_feedback_.load ().generation)
      {
        rt_feedback_.store (
          access->request_generation, access->play_state_request);
        rt_countin_frames_remaining_ = units::samples (access->countin_frames);
        rt_preroll_frames_remaining_ =
          units::samples (access->recording_preroll_frames);
      }

    return TransportSnapshot{
      { units::samples (access->loop_start_samples),
       units::samples (access->loop_end_samples)  },
      { units::samples (access->punch_in_samples),
       units::samples (access->punch_out_samples) },
      get_playhead_position_in_audio_thread (),
      rt_preroll_frames_remaining_,
      rt_countin_frames_remaining_,
      rt_feedback_.load ().state,
      access->loop_enabled,
      access->punch_enabled,
      access->recording_enabled
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

  /**
   * @brief State published from the main thread to the audio thread.
   *
   * Replaced wholesale via @ref rt_state_ on any change, so the audio thread
   * always reads a consistent snapshot.
   */
  struct TransportRtState
  {
    /** Pre-computed marker sample positions. */
    int64_t loop_start_samples{};
    int64_t loop_end_samples{};
    int64_t punch_in_samples{};
    int64_t punch_out_samples{};

    /** Countin/preroll frames from the last roll request. */
    int64_t countin_frames{};
    int64_t recording_preroll_frames{};

    /** Incremented on each new play state request. */
    uint64_t request_generation{};

    /** Last requested play state (latched by the audio thread when
     * @ref request_generation changes). */
    PlayState play_state_request{ PlayState::Paused };

    bool loop_enabled{ true };
    bool punch_enabled{ false };
    bool recording_enabled{ false };
  };

  /** Publishes the current main-thread state to @ref rt_state_. */
  void publish_rt_state ();

  /**
   * @brief Audio-thread play state feedback, packed with the generation of
   * the request it corresponds to.
   *
   * A single atomic word so readers always observe a consistent
   * (generation, state) pair: the staleness check and the state read can
   * never straddle a latch.
   *
   * Writers follow the get_snapshot() calling contract (single writer at a
   * time), so the read-modify-write in set_state() is safe.
   */
  class RtPlayStateFeedback
  {
  public:
    struct Snapshot
    {
      /** Generation of the latest request latched by the audio thread. */
      uint64_t generation;

      /** Effective play state on the audio thread. */
      PlayState state;
    };

    Snapshot load () const noexcept [[clang::nonblocking]]
    {
      const auto packed = packed_.load ();
      return {
        .generation = packed >> kStateBitWidth,
        .state = static_cast<PlayState> (packed & kStateMask)
      };
    }

    void
    store (uint64_t generation, PlayState state) noexcept [[clang::nonblocking]]
    {
      assert ((generation >> kGenerationBitWidth) == 0);
      packed_.store (pack (generation, state));
    }

    void set_state (PlayState state) noexcept [[clang::nonblocking]]
    {
      packed_.store (pack (packed_.load () >> kStateBitWidth, state));
    }

  private:
    /** The state gets its enum's width; the generation gets the rest. */
    static constexpr auto kStateBitWidth = sizeof (PlayState) * 8;
    static constexpr auto kGenerationBitWidth = 64 - kStateBitWidth;
    static constexpr uint64_t kStateMask = (uint64_t{ 1 } << kStateBitWidth) - 1;

    static_assert (kGenerationBitWidth >= 32);

    static constexpr uint64_t pack (uint64_t generation, PlayState state)
    {
      return (generation << kStateBitWidth) | static_cast<uint64_t> (state);
    }

    std::atomic<uint64_t> packed_{ pack (0, PlayState::Paused) };
  };

  /**
   * @brief Requests a play state change.
   *
   * Publishes the request to the audio thread with a new generation, then
   * updates the main-thread display state and emits @ref playStateChanged
   * if it changed.
   *
   * Unlike setPlayState(), this always publishes, even when the display
   * state already matches @a state (e.g., re-requesting roll during
   * count-in must re-publish the recomputed count-in).
   *
   * @warning Must only be called on the object's thread (the thread the
   * property notification timer fires on), like all other publishers.
   */
  void request_play_state (PlayState state);

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
   * @brief Main→RT transport state.
   *
   * Replaced on the main thread via publish_rt_state(), read on the audio
   * thread via get_snapshot().
   */
  farbot::RealtimeObject<
    TransportRtState,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    rt_state_;

  // ==================================================================
  // Main-thread-owned state (only the main thread may read/write these)
  // ==================================================================

  /** Looping or not. */
  bool loop_{ true };

  /** Whether punch in/out mode is enabled. */
  bool punch_mode_{ false };

  /** Whether MIDI/audio recording is enabled (recording toggle in transport
   * bar). */
  bool recording_{ false };

  /** Play state as known on the main thread (backs the QML property). */
  PlayState play_state_{ PlayState::Paused };

  /** Last requested play state / countin / preroll, published to the audio
   * thread via publish_rt_state(). */
  PlayState       play_state_request_{ PlayState::Paused };
  units::sample_t countin_request_frames_{};
  units::sample_t preroll_request_frames_{};
  uint64_t        request_generation_{};

  // ==================================================================
  // Audio-thread-owned state (only the audio thread may write these)
  // ==================================================================

  /** Play state feedback from the audio thread, polled by the main thread
   * for QML notification. */
  RtPlayStateFeedback rt_feedback_;

  /**
   * @brief Latch of the latest published countin/preroll request.
   *
   * Only latched from get_snapshot(), whose calling contract guarantees a
   * single writer, so no further synchronization is needed.
   */
  units::sample_t rt_countin_frames_remaining_{};
  units::sample_t rt_preroll_frames_remaining_{};

  /**
   * Position of the playhead before pausing, in ticks.
   *
   * Used by UndoableAction.
   */
  units::precise_tick_t playhead_before_pause_;

  /**
   * @brief Timer used to poll audio-thread state and notify the property
   * system of changes (e.g. play state).
   *
   * This is used to avoid Q_EMIT on realtime threads because Q_EMIT is not
   * realtime safe.
   */
  QTimer * property_notification_timer_ = nullptr;

  ConfigProvider config_provider_;
};
}
