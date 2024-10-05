// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "dsp/channel_track.h"
#include "dsp/control_port.h"
#include "dsp/engine.h"
#include "dsp/port.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtest_wrapper.h"
#include "utils/math.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

ControlPort::ControlPort (std::string label)
    : Port (label, PortType::Control, PortFlow::Input, 0.f, 1.f, 0.f)
{
}

void
ControlPort::init_after_cloning (const ControlPort &other)
{
  Port::copy_members_from (other);
  control_ = other.control_;
  base_value_ = other.base_value_;
  deff_ = other.deff_;
  carla_param_id_ = other.carla_param_id_;
}

void
ControlPort::set_unit_from_str (const std::string &str)
{
  if (str == "Hz")
    {
      id_.unit_ = PortUnit::Hz;
    }
  else if (str == "ms")
    {
      id_.unit_ = PortUnit::Ms;
    }
  else if (str == "dB")
    {
      id_.unit_ = PortUnit::Db;
    }
  else if (str == "s")
    {
      id_.unit_ = PortUnit::Seconds;
    }
  else if (str == "us")
    {
      id_.unit_ = PortUnit::Us;
    }
}

void
ControlPort::forward_control_change_event ()
{
  if (id_.owner_type_ == PortIdentifier::OwnerType::Plugin)
    {
      auto pl = get_plugin (true);
      if (pl)
        {
#if HAVE_CARLA
          if (pl->setting_.open_with_carla_ && carla_param_id_ >= 0)
            {
              auto carla = dynamic_cast<CarlaNativePlugin *> (pl);
              z_return_if_fail (carla);
              carla->set_param_value (
                static_cast<uint32_t> (carla_param_id_), control_);
            }
#endif
          if (!pl->state_changed_event_sent_.load (std::memory_order_acquire))
            {
              EVENTS_PUSH (EventType::ET_PLUGIN_STATE_CHANGED, pl);
              pl->state_changed_event_sent_.store (
                true, std::memory_order_release);
            }
        }
    }
  else if (id_.owner_type_ == PortIdentifier::OwnerType::Fader)
    {
      auto track = dynamic_cast<ChannelTrack *> (get_track (true));
      z_return_if_fail (track);

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::FaderMute)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::FaderSolo)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_.flags2_,
          PortIdentifier::Flags2::FaderListen)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_.flags2_,
          PortIdentifier::Flags2::FaderMonoCompat))
        {
          EVENTS_PUSH (EventType::ET_TRACK_FADER_BUTTON_CHANGED, track);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Amplitude))
        {
          if (ZRYTHM_HAVE_UI)
            z_return_if_fail (track->channel_->widget_);
          track->channel_->fader_->update_volume_and_fader_val ();
          EVENTS_PUSH (
            EventType::ET_CHANNEL_FADER_VAL_CHANGED, track->channel_.get ());
        }
    }
  else if (id_.owner_type_ == PortIdentifier::OwnerType::ChannelSend)
    {
      auto track = dynamic_cast<ChannelTrack *> (get_track (true));
      z_return_if_fail (track);
      auto &send = track->channel_->sends_[id_.port_index_];
      EVENTS_PUSH (EventType::ET_CHANNEL_SEND_CHANGED, send.get ());
    }
  else if (id_.owner_type_ == PortIdentifier::OwnerType::Track)
    {
      auto track = get_track (true);
      EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track);
    }
}

void
ControlPort::set_control_value (
  const float val,
  const bool  is_normalized,
  const bool  forward_events)
{
  /* set the base value */
  if (is_normalized)
    {
      base_value_ = minf_ + val * (maxf_ - minf_);
    }
  else
    {
      base_value_ = val;
    }

  unsnapped_control_ = base_value_;
  base_value_ = get_snapped_val_from_val (unsnapped_control_);

  if (!math_floats_equal (control_, base_value_))
    {
      control_ = base_value_;

      /* remember time */
      last_change_time_ = g_get_monotonic_time ();
      value_changed_from_reading_ = false;

      /* if bpm, update engine */
      if (ENUM_BITSET_TEST (
            PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Bpm))
        {
          /* this must only be called during processing kickoff or while the
           * engine is stopped */
          z_return_if_fail (
            !AUDIO_ENGINE->run_.load ()
            || ROUTER->is_processing_kickoff_thread ());

          int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
          AUDIO_ENGINE->update_frames_per_tick (
            beats_per_bar, control_, AUDIO_ENGINE->sample_rate_, false, true,
            true);
          EVENTS_PUSH (EventType::ET_BPM_CHANGED, nullptr);
        }

      /* if time sig value, update transport caches */
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_.flags2_,
          PortIdentifier::Flags2::BeatsPerBar)
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::BeatUnit))
        {
          /* this must only be called during processing kickoff or while the
           * engine is stopped */
          z_return_if_fail (
            !AUDIO_ENGINE->run_.load ()
            || ROUTER->is_processing_kickoff_thread ());

          int   beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
          int   beat_unit = P_TEMPO_TRACK->get_beat_unit ();
          bpm_t bpm = P_TEMPO_TRACK->get_current_bpm ();
          TRANSPORT->update_caches (beats_per_bar, beat_unit);
          bool update_from_ticks = ENUM_BITSET_TEST (
            PortIdentifier::Flags2, id_.flags2_,
            PortIdentifier::Flags2::BeatsPerBar);
          AUDIO_ENGINE->update_frames_per_tick (
            beats_per_bar, bpm, AUDIO_ENGINE->sample_rate_, false,
            update_from_ticks, false);
          EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, nullptr);
        }

      /* if plugin enabled port, also set plugin's own enabled port value and
       * vice versa */
      if (
        is_in_active_project ()
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_,
          PortIdentifier::Flags::PluginEnabled))
        {
          auto pl = get_plugin (true);
          z_return_if_fail (pl);

          if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags, id_.flags_,
              PortIdentifier::Flags::GenericPluginPort))
            {
              if (
                pl->own_enabled_port_
                && !math_floats_equal (pl->own_enabled_port_->control_, control_))
                {
                  z_debug (
                    "generic enabled changed - changing plugin's own enabled");

                  pl->own_enabled_port_->set_control_value (
                    control_, false, true);
                }
            }
          else if (!math_floats_equal (pl->enabled_->control_, control_))
            {
              z_debug (
                "plugin's own enabled changed - changing generic enabled");
              pl->enabled_->set_control_value (control_, false, true);
            }
        } /* endif plugin-enabled port */

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_,
          PortIdentifier::Flags::MidiAutomatable))
        {
          auto track = dynamic_cast<ProcessableTrack *> (get_track (true));
          z_return_if_fail (track);
          track->processor_->updated_midi_automatable_ports_->push_back (this);
        }

    } /* endif port value changed */

  if (forward_events)
    {
      forward_control_change_event ();
    }
}

float
ControlPort::get_control_value (const bool normalize) const
{
  /* verify that plugin exists if plugin control */
  if (
    ZRYTHM_TESTING && is_in_active_project ()
    && ENUM_BITSET_TEST (
      PortIdentifier::Flags, this->id_.flags_,
      PortIdentifier::Flags::PluginControl))
    {
      Plugin * pl = get_plugin (true);
      z_return_val_if_fail (pl, 0.f);
    }

  if (normalize)
    {
      return real_val_to_normalized (control_);
    }
  else
    {
      return control_;
    }
}

float
ControlPort::get_snapped_val_from_val (float val) const
{
  const auto flags = id_.flags_;
  if (ENUM_BITSET_TEST (
        PortIdentifier::Flags, flags, PortIdentifier::Flags::Toggle))
    {
      return is_val_toggled (val) ? 1.f : 0.f;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, flags, PortIdentifier::Flags::Integer))
    {
      return (float) get_int_from_val (val);
    }

  return val;
}

float
ControlPort::normalized_val_to_real (float normalized_val) const
{
  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::PluginControl))
    {
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Logarithmic))
        {
          auto minf = math_floats_equal (minf_, 0.f) ? 1e-20f : minf_;
          auto maxf = math_floats_equal (maxf_, 0.f) ? 1e-20f : maxf_;
          normalized_val =
            math_floats_equal (normalized_val, 0.f) ? 1e-20f : normalized_val;

          /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
          return minf * std::pow (maxf / minf, normalized_val);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Toggle))
        {
          return normalized_val >= 0.001f ? 1.f : 0.f;
        }
      else
        {
          return minf_ + normalized_val * (maxf_ - minf_);
        }
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Toggle))
    {
      return normalized_val > 0.0001f;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::ChannelFader))
    {
      return math_get_amp_val_from_fader (normalized_val);
    }
  else
    {
      return minf_ + normalized_val * (maxf_ - minf_);
    }
  z_return_val_if_reached (normalized_val);
}

float
ControlPort::real_val_to_normalized (float real_val) const
{
  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::PluginControl))
    {
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Logarithmic))
        {
          const auto minf = math_floats_equal (minf_, 0.f) ? 1e-20f : minf_;
          const auto maxf = math_floats_equal (maxf_, 0.f) ? 1e-20f : maxf_;
          real_val = math_floats_equal (real_val, 0.f) ? 1e-20f : real_val;

          /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
          return std::log (real_val / minf) / std::log (maxf / minf);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Toggle))
        {
          return real_val;
        }
      else
        {
          const auto sizef = maxf_ - minf_;
          return (sizef - (maxf_ - real_val)) / sizef;
        }
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Toggle))
    {
      return real_val;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::ChannelFader))
    {
      return math_get_fader_val_from_amp (real_val);
    }
  else
    {
      auto sizef = maxf_ - minf_;
      return (sizef - (maxf_ - real_val)) / sizef;
    }
  z_return_val_if_reached (0.f);
}

void
ControlPort::set_val_from_normalized (float val, bool automating)
{
  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::PluginControl))
    {
      auto real_val = normalized_val_to_real (val);
      if (!math_floats_equal (control_, real_val))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
          set_control_value (real_val, false, true);
        }
      automating_ = automating;
      base_value_ = real_val;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Toggle))
    {
      auto real_val = normalized_val_to_real (val);
      if (!math_floats_equal (control_, real_val))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
          control_ = is_val_toggled (real_val) ? 1.f : 0.f;
        }
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::FaderMute))
        {
          auto track = dynamic_cast<ChannelTrack *> (get_track (true));
          track->set_muted (is_toggled (), false, false, true);
        }
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::ChannelFader))
    {
      auto  track = get_track (true);
      auto &ch = dynamic_cast<ChannelTrack *> (track)->get_channel ();
      if (!math_floats_equal (ch->fader_->get_fader_val (), val))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
        }
      ch->fader_->set_amp (math_get_amp_val_from_fader (val));
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::StereoBalance))
    {
      auto  track = get_track (true);
      auto &ch = dynamic_cast<ChannelTrack *> (track)->get_channel ();
      if (!math_floats_equal (ch->get_balance_control (), val))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
        }
      ch->set_balance_control (val);
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::MidiAutomatable))
    {
      auto real_val = minf_ + val * (maxf_ - minf_);
      if (!math_floats_equal (val, control_))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
        }
      set_control_value (real_val, false, false);
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Automatable))
    {
      auto real_val = normalized_val_to_real (val);
      if (!math_floats_equal (real_val, control_))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
        }
      set_control_value (real_val, false, false);
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Amplitude))
    {
      auto real_val = normalized_val_to_real (val);
      set_control_value (real_val, false, false);
    }
  else
    {
      z_return_if_reached ();
    }
}

void
ControlPort::process (const EngineProcessTimeInfo time_nfo, const bool noroll)
{
  const auto owner_type = id_.owner_type_;

  if (
    !is_input()
    || (owner_type == PortIdentifier::OwnerType::Fader && (ENUM_BITSET_TEST (PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::MonitorFader) || ENUM_BITSET_TEST (PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::Prefader)))
    || ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::TpMono)
    || ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::TpInputGain)
    || !(ENUM_BITSET_TEST (
      PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::Automatable)))
    {
      return;
    }

  /* calculate value from automation track */
  auto at = at_;
  if (!at) [[unlikely]]
    {
      z_error ("No automation track found for port {}", get_label ());
    }
  if (ZRYTHM_TESTING && at)
    {
      auto found_at = AutomationTrack::find_from_port (*this, nullptr, true);
      z_return_if_fail (at == found_at);
    }

  if (at && at->should_read_automation (AUDIO_ENGINE->timestamp_start_))
    {
      const Position pos{ (signed_frame_t) time_nfo.g_start_frame_w_offset_ };

      /* if playhead pos changed manually recently or transport is
       * rolling, we will force the last known automation point value
       * regardless of whether there is a region at current pos */
      const bool can_read_previous_automation =
        TRANSPORT->is_rolling ()
        || (TRANSPORT->last_manual_playhead_change_ - AUDIO_ENGINE->last_timestamp_start_ > 0);

      /* if there was an automation event at the playhead position, set
       * val and flag */
      const auto ap =
        at->get_ap_before_pos (pos, !can_read_previous_automation, true);
      if (ap)
        {
          const float val = at->get_val_at_pos (
            pos, true, !can_read_previous_automation, Z_F_USE_SNAPSHOTS);
          set_val_from_normalized (val, true);
          value_changed_from_reading_ = true;
        }
    }

  /* whether this is the first CV processed on this control port */
  bool first_cv = true;
  for (size_t k = 0; k < srcs_.size (); k++)
    {
      const auto &conn = src_connections_[k];
      if (!conn.enabled_) [[unlikely]]
        continue;

      Port * src_port = srcs_[k];
      if (src_port->id_.type_ == PortType::CV)
        {
          const float depth_range = (maxf_ - minf_) / 2.f;

          /* figure out whether to use base value or the current value */
          float val_to_use;
          if (first_cv)
            {
              val_to_use = base_value_;
              first_cv = false;
            }
          else
            {
              val_to_use = control_;
            }

          control_ = std::clamp<float> (
            val_to_use + depth_range * src_port->buf_[0] * conn.multiplier_,
            minf_, maxf_);
          forward_control_change_event ();
        }
    }
}