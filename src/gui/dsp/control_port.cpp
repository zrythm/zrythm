// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "engine/device_io/engine.h"
#include "engine/session/router.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/carla_native_plugin.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/port.h"
#include "structure/tracks/channel_track.h"
#include "structure/tracks/tempo_track.h"
#include "structure/tracks/tracklist.h"
#include "utils/gtest_wrapper.h"
#include "utils/math.h"
#include "utils/rt_thread_id.h"

ControlPort::ControlPort () : ControlPort (utils::Utf8String{}) { }

ControlPort::ControlPort (utils::Utf8String label)
    : Port (std::move (label), PortType::Control, PortFlow::Input, 0.f, 1.f, 0.f)
{
}

void
ControlPort::init_after_cloning (
  const ControlPort &other,
  ObjectCloneType    clone_type)
{
  Port::copy_members_from (other, clone_type);
  control_ = other.control_;
  base_value_ = other.base_value_;
  deff_ = other.deff_;
  carla_param_id_ = other.carla_param_id_;
}

void
ControlPort::set_unit_from_str (const utils::Utf8String &str)
{
  if (str == u8"Hz")
    {
      id_->unit_ = dsp::PortUnit::Hz;
    }
  else if (str == u8"ms")
    {
      id_->unit_ = dsp::PortUnit::Ms;
    }
  else if (str == u8"dB")
    {
      id_->unit_ = dsp::PortUnit::Db;
    }
  else if (str == u8"s")
    {
      id_->unit_ = dsp::PortUnit::Seconds;
    }
  else if (str == u8"us")
    {
      id_->unit_ = dsp::PortUnit::Us;
    }
}

void
ControlPort::set_control_value (
  const float val,
  const bool  is_normalized,
  const bool  forward_event_to_plugin)
{
  /* set the base value */
  if (is_normalized)
    {
      base_value_ = range_.minf_ + val * (range_.maxf_ - range_.minf_);
    }
  else
    {
      base_value_ = val;
    }

  unsnapped_control_ = base_value_;
  base_value_ = get_snapped_val_from_val (unsnapped_control_);

  if (!utils::math::floats_equal (control_, base_value_))
    {
      control_ = base_value_;

      /* remember time */
      last_change_time_ = Zrythm::getInstance ()->get_monotonic_time_usecs ();
      value_changed_from_reading_ = false;

      if (owner_)
        {
          owner_->on_control_change_event (get_uuid (), *id_, control_);
        }
    } /* endif port value changed */

  if (forward_event_to_plugin)
    {
      if (owner_)
        {
          owner_->on_control_change_event (get_uuid (), *id_, control_);
        }
    }
}

float
ControlPort::get_control_value (const bool normalize) const
{
  if (normalize)
    {
      return real_val_to_normalized (control_);
    }

  return control_;
}

float
ControlPort::get_snapped_val_from_val (float val) const
{
  const auto flags = id_->flags_;
  if (ENUM_BITSET_TEST (flags, PortIdentifier::Flags::Toggle))
    {
      return is_val_toggled (val) ? 1.f : 0.f;
    }
  if (ENUM_BITSET_TEST (flags, PortIdentifier::Flags::Integer))
    {
      return (float) get_int_from_val (val);
    }

  return val;
}

float
ControlPort::normalized_val_to_real (float normalized_val) const
{
  if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::PluginControl))
    {
      if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Logarithmic))
        {
          auto minf =
            utils::math::floats_equal (range_.minf_, 0.f) ? 1e-20f : range_.minf_;
          auto maxf =
            utils::math::floats_equal (range_.maxf_, 0.f) ? 1e-20f : range_.maxf_;
          normalized_val =
            utils::math::floats_equal (normalized_val, 0.f)
              ? 1e-20f
              : normalized_val;

          /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
          return minf * std::pow (maxf / minf, normalized_val);
        }
      if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Toggle))
        {
          return normalized_val >= 0.001f ? 1.f : 0.f;
        }
      else
        {
          return range_.minf_ + normalized_val * (range_.maxf_ - range_.minf_);
        }
    }
  else if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Toggle))
    {
      return normalized_val > 0.0001f;
    }
  else if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::ChannelFader))
    {
      return utils::math::get_amp_val_from_fader (normalized_val);
    }
  else
    {
      return range_.minf_ + normalized_val * (range_.maxf_ - range_.minf_);
    }
  z_return_val_if_reached (normalized_val);
}

float
ControlPort::real_val_to_normalized (float real_val) const
{
  if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::PluginControl))
    {
      if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Logarithmic))
        {
          const auto minf =
            utils::math::floats_equal (range_.minf_, 0.f) ? 1e-20f : range_.minf_;
          const auto maxf =
            utils::math::floats_equal (range_.maxf_, 0.f) ? 1e-20f : range_.maxf_;
          real_val =
            utils::math::floats_equal (real_val, 0.f) ? 1e-20f : real_val;

          /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
          return std::log (real_val / minf) / std::log (maxf / minf);
        }
      if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Toggle))
        {
          return real_val;
        }

      const auto sizef = range_.maxf_ - range_.minf_;
      return (sizef - (range_.maxf_ - real_val)) / sizef;
    }
  if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Toggle))
    {
      return real_val;
    }
  if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::ChannelFader))
    {
      return utils::math::get_fader_val_from_amp (real_val);
    }
  auto sizef = range_.maxf_ - range_.minf_;
  return (sizef - (range_.maxf_ - real_val)) / sizef;
  z_return_val_if_reached (0.f);
}

void
ControlPort::set_val_from_normalized (float val, bool automating)
{
  if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::PluginControl))
    {
      auto real_val = normalized_val_to_real (val);
      if (!utils::math::floats_equal (control_, real_val))
        {
          // EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
          set_control_value (real_val, false, true);
        }
      automating_ = automating;
      base_value_ = real_val;
    }
  else if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Toggle))
    {
      auto real_val = normalized_val_to_real (val);
      if (!utils::math::floats_equal (control_, real_val))
        {
          // EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
          control_ = is_val_toggled (real_val) ? 1.f : 0.f;
        }
      if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::FaderMute))
        {
          set_control_value (is_toggled () ? 1.f : 0.f, false, true);
        }
    }
  else if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::ChannelFader))
    {
      set_control_value (
        utils::math::get_amp_val_from_fader (val), false, false);
    }
  else if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::StereoBalance))
    {
      auto real_val = normalized_val_to_real (val);
      set_control_value (real_val, false, false);
    }
  else if (
    ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::MidiAutomatable))
    {
      auto real_val = range_.minf_ + val * (range_.maxf_ - range_.minf_);
      if (!utils::math::floats_equal (val, control_))
        {
          // EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
        }
      set_control_value (real_val, false, false);
    }
  else if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Automatable))
    {
      auto real_val = normalized_val_to_real (val);
      if (!utils::math::floats_equal (real_val, control_))
        {
          // EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, this);
        }
      set_control_value (real_val, false, false);
    }
  else if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Amplitude))
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
ControlPort::process_block (const EngineProcessTimeInfo time_nfo)
{
  if (
    !is_input ()
    || ENUM_BITSET_TEST (id_->flags2_, PortIdentifier::Flags2::MonitorFader)
    || ENUM_BITSET_TEST (id_->flags2_, PortIdentifier::Flags2::Prefader)
    || ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::TpMono)
    || ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::TpInputGain)
    || !(ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::Automatable)))
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
      auto found_at = structure::tracks::AutomationTrack::find_from_port (
        *this, nullptr, true);
      z_return_if_fail (at == found_at);
    }

  if (at && at->should_read_automation (AUDIO_ENGINE->timestamp_start_))
    {
      const dsp::Position pos{
        (signed_frame_t) time_nfo.g_start_frame_w_offset_,
        AUDIO_ENGINE->ticks_per_frame_
      };

      /* if playhead pos changed manually recently or transport is
       * rolling, we will force the last known automation point value
       * regardless of whether there is a region at current pos */
      const bool can_read_previous_automation =
        TRANSPORT->isRolling ()
        || (TRANSPORT->last_manual_playhead_change_ - AUDIO_ENGINE->last_timestamp_start_ > 0);

      /* if there was an automation event at the playhead position, set
       * val and flag */
      const auto ap =
        at->get_ap_before_pos (pos, !can_read_previous_automation, true);
      if (ap)
        {
          const float val =
            at->get_val_at_pos (pos, true, !can_read_previous_automation, true);
          set_val_from_normalized (val, true);
          value_changed_from_reading_ = true;
        }
    }

  /* whether this is the first CV processed on this control port */
  bool first_cv = true;
  for (size_t k = 0; k < srcs_.size (); k++)
    {
      const auto &conn = src_connections_[k];
      if (!conn->enabled_) [[unlikely]]
        continue;

      Port * src_port = srcs_[k];
      if (src_port->id_->type_ == PortType::CV)
        {
          const auto * cv_src_port = dynamic_cast<const CVPort *> (src_port);
          const float  depth_range = (range_.maxf_ - range_.minf_) / 2.f;

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
            val_to_use + (depth_range * cv_src_port->buf_[0] * conn->multiplier_),
            range_.minf_, range_.maxf_);
          if (owner_)
            {
              owner_->on_control_change_event (get_uuid (), *id_, control_);
            }
        }
    }
}
