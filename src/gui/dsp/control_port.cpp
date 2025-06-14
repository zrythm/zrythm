// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/control_port.h"
#include "gui/dsp/cv_port.h"
#include "utils/math.h"

ControlPort::ControlPort (utils::Utf8String label)
    : Port (std::move (label), PortType::Control, PortFlow::Input, 0.f, 1.f, 0.f),
      time_provider_ (std::make_unique<utils::QElapsedTimeProvider> ())
{
}

void
init_from (
  ControlPort           &obj,
  const ControlPort     &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Port &> (obj), static_cast<const Port &> (other), clone_type);
  obj.control_ = other.control_;
  obj.base_value_ = other.base_value_;
  obj.deff_ = other.deff_;
  obj.carla_param_id_ = other.carla_param_id_;
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
      last_change_time_ = time_provider_->get_monotonic_time_usecs ();
      value_changed_from_reading_ = false;

      if (owner_ != nullptr)
        {
          owner_->on_control_change_event (get_uuid (), *id_, control_);
        }
    } /* endif port value changed */

  if (forward_event_to_plugin)
    {
      if (owner_ != nullptr)
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
      return normalized_val > 0.0001f ? 1.f : 0.f;
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
  if (automation_reader_)
    {
      const auto val = std::invoke (
        automation_reader_.value (), time_nfo.g_start_frame_w_offset_);
      if (val)
        {
          set_val_from_normalized (val.value (), true);
          value_changed_from_reading_ = true;
        }
    }

  /* whether this is the first CV processed on this control port */
  bool first_cv = true;
  for (const auto &[src_port, conn] : port_sources_)
    {
      if (!conn->enabled_) [[unlikely]]
        continue;

      const float depth_range = (range_.maxf_ - range_.minf_) / 2.f;

      /* figure out whether to use base value or the current value */
      const float val_to_use = [&] () {
        if (first_cv)
          {
            first_cv = false;
            return base_value_;
          }

        return control_;
      }();

      control_ = std::clamp<float> (
        val_to_use + (depth_range * src_port->buf_[0] * conn->multiplier_),
        range_.minf_, range_.maxf_);
      if (owner_ != nullptr)
        {
          owner_->on_control_change_event (get_uuid (), *id_, control_);
        }
    }
}
