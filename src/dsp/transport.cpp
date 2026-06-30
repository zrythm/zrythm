// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <utility>

#include "dsp/transport.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>

namespace zrythm::dsp
{
Transport::Transport (
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  ConfigProvider              config_provider,
  QObject *                   parent)
    : QObject (parent), playhead_ (tempo_map_wrapper.get_tempo_map ()),
      playhead_adapter_ (new dsp::PlayheadQmlWrapper (playhead_, this)),
      cue_position_ (utils::make_qobject_unique<dsp::TimelinePosition> (this)),
      loop_start_position_ (
        utils::make_qobject_unique<dsp::TimelinePosition> (this)),
      loop_end_position_ (
        utils::make_qobject_unique<dsp::TimelinePosition> (this)),
      punch_in_position_ (
        utils::make_qobject_unique<dsp::TimelinePosition> (this)),
      punch_out_position_ (
        utils::make_qobject_unique<dsp::TimelinePosition> (this)),
      property_notification_timer_ (new QTimer (this)),
      config_provider_ (std::move (config_provider))
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

  // Recompute RT snapshot whenever any marker position changes
  for (
    auto * pos :
    { cue_position_.get (), loop_start_position_.get (),
      loop_end_position_.get (), punch_in_position_.get (),
      punch_out_position_.get () })
    {
      QObject::connect (pos, &dsp::Position::positionChanged, this, [this] () {
        update_rt_marker_snapshot ();
      });
    }

  // Recompute RT snapshot when tempo changes (BPM, time signature, etc.)
  // — tick values stay the same but sample positions change.
  QObject::connect (
    &tempo_map_wrapper, &dsp::TempoMapWrapper::tempoEventsChanged, this,
    [this] () { update_rt_marker_snapshot (); });

  // Recompute RT snapshot when the sample rate changes.
  QObject::connect (
    &tempo_map_wrapper, &dsp::TempoMapWrapper::sampleRateChanged, this,
    [this] () { update_rt_marker_snapshot (); });

  if (parent == nullptr)
    {
      update_rt_marker_snapshot ();
      return;
    }

  loop_end_position_->setTicks (
    tempo_map_wrapper.get_tempo_map ()
      .musical_position_to_tick (
        { .bar = 5, .beat = 1, .sixteenth = 1, .tick = 0 })
      .asDouble ());
  punch_in_position_->setTicks (
    tempo_map_wrapper.get_tempo_map ()
      .musical_position_to_tick (
        { .bar = 3, .beat = 1, .sixteenth = 1, .tick = 0 })
      .asDouble ());
  punch_out_position_->setTicks (
    tempo_map_wrapper.get_tempo_map ()
      .musical_position_to_tick (
        { .bar = 5, .beat = 1, .sixteenth = 1, .tick = 0 })
      .asDouble ());

  update_rt_marker_snapshot ();
}

void
Transport::update_rt_marker_snapshot ()
{
  const auto &tm = playhead_.get_tempo_map ();
  rt_markers_.nonRealtimeReplace (
    MarkerSnapshot{
      .loop_start_samples =
        tm.tick_to_samples_rounded (loop_start_position_->asTick ())
          .in (units::samples),
      .loop_end_samples =
        tm.tick_to_samples_rounded (loop_end_position_->asTick ())
          .in (units::samples),
      .punch_in_samples =
        tm.tick_to_samples_rounded (punch_in_position_->asTick ())
          .in (units::samples),
      .punch_out_samples =
        tm.tick_to_samples_rounded (punch_out_position_->asTick ())
          .in (units::samples),
    });
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
Transport::movePlayhead (double ticks, bool setCuePoint)
{
  move_playhead (units::ticks (ticks), setCuePoint);
}

void
init_from (
  Transport             &obj,
  const Transport       &other,
  utils::ObjectCloneType clone_type)
{
  obj.loop_start_position_->setTicks (other.loop_start_position_->ticks ());
  obj.playhead_.set_position_ticks (other.playhead_.position_ticks ());
  obj.loop_end_position_->setTicks (other.loop_end_position_->ticks ());
  obj.cue_position_->setTicks (other.cue_position_->ticks ());
  obj.punch_in_position_->setTicks (other.punch_in_position_->ticks ());
  obj.punch_out_position_->setTicks (other.punch_out_position_->ticks ());
  obj.update_rt_marker_snapshot ();
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
      move_playhead (cue_position_->asTick ().asQuantity (), false);
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
        TimelineTick{ playhead_.position_ticks () });

    // Get time signature at current position to calculate ticks per bar
    const auto time_sig = playhead_.get_tempo_map ().time_signature_at_tick (
      au::round_as<int64_t> (units::ticks, playhead_.position_ticks ()));
    const int64_t ticks_per_bar = time_sig.ticks_per_bar ().in (units::ticks);

    // Calculate target position: current position - (num_bars * ticks_per_bar)
    const auto target_ticks =
      playhead_.position_ticks () - units::ticks (num_bars * ticks_per_bar);
    return target_ticks < units::ticks (0.0)
             ? -playhead_.get_tempo_map ().tick_to_samples_rounded (
                 TimelineTick{ -target_ticks })
             : playhead_.get_tempo_map ().tick_to_samples_rounded (
                 TimelineTick{ target_ticks });
  };

  // Get current position in samples for calculations
  const auto current_samples =
    playhead_.get_tempo_map ().tick_to_samples_rounded (
      TimelineTick{ playhead_.position_ticks () });

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
          .asDouble ());
    }

  setPlayState (PlayState::RollRequested);
}

void
Transport::add_to_playhead_in_audio_thread (
  const dsp::Transport::TransportSnapshot &transport_snapshot,
  const units::sample_t                    nframes) noexcept
{
  const auto cur_pos = get_playhead_position_in_audio_thread ();
  const auto new_pos = dsp::playhead_position_after_adding_frames (
    cur_pos, nframes, transport_snapshot.loop_enabled (),
    transport_snapshot.get_loop_range_positions ().first,
    transport_snapshot.get_loop_range_positions ().second);
  playhead_.advance_processing (new_pos - cur_pos);
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
      cue_position_->setTicks (target_ticks.in (units::ticks));
    }
}

bool
Transport::position_is_inside_punch_range (const units::sample_t pos) const
{
  const auto &tm = playhead_.get_tempo_map ();
  return pos >= tm.tick_to_samples_rounded (punch_in_position_->asTick ())
         && pos < tm.tick_to_samples_rounded (punch_out_position_->asTick ());
}

void
to_json (nlohmann::json &j, const Transport &transport)
{
  j = nlohmann::json{
    { Transport::kPlayheadKey,     transport.playhead_             },
    { Transport::kCuePosKey,       *transport.cue_position_        },
    { Transport::kLoopStartPosKey, *transport.loop_start_position_ },
    { Transport::kLoopEndPosKey,   *transport.loop_end_position_   },
    { Transport::kPunchInPosKey,   *transport.punch_in_position_   },
    { Transport::kPunchOutPosKey,  *transport.punch_out_position_  },
  };
}

void
from_json (const nlohmann::json &j, Transport &transport)
{
  // All position fields are optional - use defaults if not present
  if (j.contains (Transport::kPlayheadKey))
    {
      j.at (Transport::kPlayheadKey).get_to (transport.playhead_);
      transport.playhead ()->setTicks (
        transport.playhead_.position_ticks ().in (units::ticks));
    }
  if (j.contains (Transport::kCuePosKey))
    {
      j.at (Transport::kCuePosKey).get_to (*transport.cue_position_);
    }
  if (j.contains (Transport::kLoopStartPosKey))
    {
      j.at (Transport::kLoopStartPosKey).get_to (*transport.loop_start_position_);
    }
  if (j.contains (Transport::kLoopEndPosKey))
    {
      j.at (Transport::kLoopEndPosKey).get_to (*transport.loop_end_position_);
    }
  if (j.contains (Transport::kPunchInPosKey))
    {
      j.at (Transport::kPunchInPosKey).get_to (*transport.punch_in_position_);
    }
  if (j.contains (Transport::kPunchOutPosKey))
    {
      j.at (Transport::kPunchOutPosKey).get_to (*transport.punch_out_position_);
    }
  transport.update_rt_marker_snapshot ();
}
}
