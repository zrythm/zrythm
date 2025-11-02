// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <utility>

#include "dsp/transport.h"

namespace zrythm::dsp
{
Transport::Transport (
  const dsp::TempoMap &tempo_map,
  const dsp::SnapGrid &snap_grid,
  ConfigProvider       config_provider,
  QObject *            parent)
    : QObject (parent), playhead_ (tempo_map),
      playhead_adapter_ (new dsp::PlayheadQmlWrapper (playhead_, this)),
      time_conversion_funcs_ (
        dsp::AtomicPosition::TimeConversionFunctions::from_tempo_map (
          playhead_.get_tempo_map ())),
      cue_position_ (*time_conversion_funcs_),
      cue_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (cue_position_, std::nullopt, this)),
      loop_start_position_ (*time_conversion_funcs_),
      loop_start_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (loop_start_position_, std::nullopt, this)),
      loop_end_position_ (*time_conversion_funcs_),
      loop_end_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (loop_end_position_, std::nullopt, this)),
      punch_in_position_ (*time_conversion_funcs_),
      punch_in_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (punch_in_position_, std::nullopt, this)),
      punch_out_position_ (*time_conversion_funcs_),
      punch_out_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (punch_out_position_, std::nullopt, this)),
      property_notification_timer_ (new QTimer (this)),
      config_provider_ (std::move (config_provider)), snap_grid_ (snap_grid)
{
  z_debug ("Creating transport...");

  property_notification_timer_->setInterval (16);
  property_notification_timer_->callOnTimeout (this, [this] () {
    if (needs_property_notification_.exchange (false))
      {
        Q_EMIT playStateChanged (play_state_);
      }
  });
  property_notification_timer_->start ();

  if (parent == nullptr)
    {
      return;
    }

  loop_end_position_.set_ticks (tempo_map.musical_position_to_tick (
    { .bar = 5, .beat = 1, .sixteenth = 1, .tick = 0 }));
  punch_in_position_.set_ticks (tempo_map.musical_position_to_tick (
    { .bar = 3, .beat = 1, .sixteenth = 1, .tick = 0 }));
  punch_out_position_.set_ticks (tempo_map.musical_position_to_tick (
    { .bar = 5, .beat = 1, .sixteenth = 1, .tick = 0 }));
}

void
Transport::setLoopEnabled (bool enabled)
{
  if (loop_ == enabled)
    {
      return;
    }

  loop_ = enabled;
  Q_EMIT (loopEnabledChanged (loop_));
}

void
Transport::setRecordEnabled (bool enabled)
{
  if (recording_ == enabled)
    {
      return;
    }

  recording_ = enabled;
  Q_EMIT recordEnabledChanged (recording_);
}

void
Transport::setPunchEnabled (bool enabled)
{
  if (punch_mode_ == enabled)
    return;

  punch_mode_ = enabled;
  Q_EMIT punchEnabledChanged (enabled);
}

Transport::PlayState
Transport::getPlayState () const
{
  return play_state_;
}

void
Transport::setPlayState (PlayState state)
{
  if (play_state_ == state)
    {
      return;
    }

  play_state_ = state;
  Q_EMIT playStateChanged (play_state_);
}

void
init_from (
  Transport             &obj,
  const Transport       &other,
  utils::ObjectCloneType clone_type)
{
  obj.loop_start_position_.set_ticks (other.loop_start_position_.get_ticks ());
  obj.playhead_.set_position_ticks (other.playhead_.position_ticks ());
  obj.loop_end_position_.set_ticks (other.loop_end_position_.get_ticks ());
  obj.cue_position_.set_ticks (other.cue_position_.get_ticks ());
  obj.punch_in_position_.set_ticks (other.punch_in_position_.get_ticks ());
  obj.punch_out_position_.set_ticks (other.punch_out_position_.get_ticks ());
}

void
Transport::set_play_state_rt_safe (PlayState state)
{
  if (play_state_ == state)
    {
      return;
    }

  play_state_ = state;
  needs_property_notification_.store (true);
}

void
Transport::requestPause ()
{
  set_play_state_rt_safe (PlayState::PauseRequested);

  playhead_before_pause_ = playhead_.position_ticks ();
  if (config_provider_.return_to_cue_on_pause_ ())
    {
      move_playhead (cue_position_.get_ticks (), false);
    }
}

void
Transport::requestRoll ()
{
  // Generic lambda to calculate target position for bars
  auto calculate_target_position_for_n_bars_before =
    [this] (int num_bars) -> units::sample_t {
    if (num_bars <= 0)
      return playhead_.get_tempo_map ().tick_to_samples_rounded (
        playhead_.position_ticks ());

    // Get time signature at current position to calculate ticks per bar
    const auto time_sig = playhead_.get_tempo_map ().time_signature_at_tick (
      au::round_as<int64_t> (units::ticks, playhead_.position_ticks ()));
    const int64_t ticks_per_bar = time_sig.ticks_per_bar ().in (units::ticks);

    // Calculate target position: current position - (num_bars * ticks_per_bar)
    const auto target_ticks =
      playhead_.position_ticks () - units::ticks (num_bars * ticks_per_bar);
    return target_ticks < units::ticks (0.0)
             ? -playhead_.get_tempo_map ().tick_to_samples_rounded (-target_ticks)
             : playhead_.get_tempo_map ().tick_to_samples_rounded (target_ticks);
  };

  // Get current position in samples for calculations
  const auto current_samples =
    playhead_.get_tempo_map ().tick_to_samples_rounded (
      playhead_.position_ticks ());

  /* handle countin */
  const auto countin_target_pos = calculate_target_position_for_n_bars_before (
    config_provider_.metronome_countin_bars_ ());
  countin_frames_remaining_ = current_samples - countin_target_pos;

  if (recording_)
    {
      /* handle preroll */
      const auto preroll_target_pos =
        calculate_target_position_for_n_bars_before (
          config_provider_.recording_preroll_bars_ ());
      recording_preroll_frames_remaining_ = current_samples - preroll_target_pos;

      // Move playhead to preroll position
      playhead_adapter_->setTicks (
        playhead_.get_tempo_map ()
          .samples_to_tick (preroll_target_pos)
          .in (units::ticks));
    }

  setPlayState (PlayState::RollRequested);
}

void
Transport::add_to_playhead_in_audio_thread (const units::sample_t nframes)
{
  const auto cur_pos = get_playhead_position_in_audio_thread ();
  auto new_pos = get_playhead_position_after_adding_frames_in_audio_thread (
    get_playhead_position_in_audio_thread (), nframes);
  auto diff = new_pos - cur_pos;
  playhead_.advance_processing (diff);
}

bool
Transport::can_user_move_playhead () const
{
  return !recording_ || play_state_ != PlayState::Rolling;
}

void
Transport::move_playhead (units::precise_tick_t target_ticks, bool set_cue_point)
{
  /* if currently recording, do nothing */
  if (!can_user_move_playhead ())
    {
      z_info ("currently recording - refusing to move playhead manually");
      return;
    }

  /* move to new pos */
  playhead_adapter_->setTicks (target_ticks.in (units::ticks));

  if (set_cue_point)
    {
      /* move cue point */
      cue_position_adapter_->setTicks (target_ticks.in (units::ticks));
    }
}

void
Transport::set_loop_range (
  bool                  range1,
  units::precise_tick_t start_pos,
  units::precise_tick_t pos,
  bool                  snap)
{
  auto * pos_to_set = range1 ? &loop_start_position_ : &loop_end_position_;

  if (pos < units::ticks (0))
    {
      pos_to_set->set_ticks (units::ticks (0));
    }
  else
    {
      pos_to_set->set_ticks (pos);
    }

  if (snap)
    {
      pos_to_set->set_ticks (
        snap_grid_.snap (pos_to_set->get_ticks (), start_pos));
    }
}

bool
Transport::position_is_inside_punch_range (const units::sample_t pos)
{
  return pos >= punch_in_position_.get_samples ()
         && pos < punch_out_position_.get_samples ();
}

void
Transport::moveBackward ()
{
  const auto &tempo_map = playhead_.get_tempo_map ();
  auto        pos_ticks = units::ticks (
    snap_grid_.prevSnapPoint (playhead_.position_ticks ().in (units::ticks)));
  const auto pos_frames = tempo_map.tick_to_samples_rounded (pos_ticks);
  /* if prev snap point is exactly at the playhead or very close it, go back
   * more */
  const auto playhead_ticks = playhead_.position_ticks ();
  const auto playhead_frames =
    tempo_map.tick_to_samples_rounded (playhead_ticks);
  if (
    pos_frames > units::samples(0)
    && (pos_frames == playhead_frames || (isRolling () && (tempo_map.tick_to_seconds(playhead_ticks) - tempo_map.tick_to_seconds(pos_ticks) < REPEATED_BACKWARD_MS))))
    {
      pos_ticks = units::ticks (snap_grid_.prevSnapPoint (
        (pos_ticks - units::ticks (1.0)).in (units::ticks)));
    }
  move_playhead (pos_ticks, true);
}

void
Transport::moveForward ()
{
  double pos_ticks =
    snap_grid_.nextSnapPoint (playhead_.position_ticks ().in (units::ticks));
  move_playhead (units::ticks (pos_ticks), true);
}

void
to_json (nlohmann::json &j, const Transport &transport)
{
  j = nlohmann::json{
    { Transport::kPlayheadKey,     transport.playhead_            },
    { Transport::kCuePosKey,       transport.cue_position_        },
    { Transport::kLoopStartPosKey, transport.loop_start_position_ },
    { Transport::kLoopEndPosKey,   transport.loop_end_position_   },
    { Transport::kPunchInPosKey,   transport.punch_in_position_   },
    { Transport::kPunchOutPosKey,  transport.punch_out_position_  },
  };
}

void
from_json (const nlohmann::json &j, Transport &transport)
{
  j.at (Transport::kPlayheadKey).get_to (transport.playhead_);
  transport.playhead ()->setTicks (
    transport.playhead_.position_ticks ().in (units::ticks));
  j.at (Transport::kCuePosKey).get_to (transport.cue_position_);
  j.at (Transport::kLoopStartPosKey).get_to (transport.loop_start_position_);
  j.at (Transport::kLoopEndPosKey).get_to (transport.loop_end_position_);
  j.at (Transport::kPunchInPosKey).get_to (transport.punch_in_position_);
  j.at (Transport::kPunchOutPosKey).get_to (transport.punch_out_position_);
}
}
