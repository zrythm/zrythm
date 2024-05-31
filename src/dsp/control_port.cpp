// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "dsp/control_port.h"
#include "dsp/engine.h"
#include "dsp/port.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "zrythm.h"
#include "zrythm_app.h"

/**
 * Get the current real value of the control.
 */
float
control_port_get_val (Port * self)
{
  return self->control_;
}

/**
 * Get the current real value of the control.
 */
float
control_port_get_normalized_val (Port * self)
{
  return control_port_real_val_to_normalized (self, self->control_);
}

/**
 * Get the current real unsnapped value of the
 * control.
 */
float
control_port_get_unsnapped_val (Port * self)
{
  return self->unsnapped_control_;
}

/**
 * Get the default real value of the control.
 */
float
control_port_get_default_val (Port * self)
{
  return self->deff_;
}

/**
 * Get the default real value of the control.
 */
void
control_port_set_real_val (Port * self, float val)
{
  g_return_if_fail (IS_PORT (self));
  self->set_control_value (val, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
}

/**
 * Get the default real value of the control and
 * sends UI events.
 */
void
control_port_set_real_val_w_events (Port * self, float val)
{
  g_return_if_fail (IS_PORT (self));
  self->set_control_value (val, F_NOT_NORMALIZED, F_PUBLISH_EVENTS);
}

void
control_port_set_toggled (Port * self, bool toggled, bool forward_events)
{
  g_return_if_fail (IS_PORT (self));
  self->set_control_value (
    toggled ? 1.f : 0.f, F_NOT_NORMALIZED, forward_events);
}

/**
 * Gets the control value for an integer port.
 */
int
control_port_get_int (Port * self)
{
  return control_port_get_int_from_val (self->control_);
}

/**
 * Gets the control value for an integer port.
 */
int
control_port_get_int_from_val (float val)
{
  return math_round_float_to_signed_32 (val);
}

/**
 * Returns the snapped value (eg, if toggle,
 * returns 0.f or 1.f).
 */
float
control_port_get_snapped_val (Port * self)
{
  float val = control_port_get_val (self);

  return control_port_get_snapped_val_from_val (self, val);
}

/**
 * Returns the snapped value (eg, if toggle,
 * returns 0.f or 1.f).
 */
float
control_port_get_snapped_val_from_val (Port * self, float val)
{
  PortIdentifier::Flags flags = self->id_.flags_;
  if (ENUM_BITSET_TEST (
        PortIdentifier::Flags, flags, PortIdentifier::Flags::TOGGLE))
    {
      return control_port_is_val_toggled (val) ? 1.f : 0.f;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, flags, PortIdentifier::Flags::INTEGER))
    {
      return (float) control_port_get_int_from_val (val);
    }

  return val;
}

/**
 * Converts normalized value (0.0 to 1.0) to
 * real value (eg. -10.0 to 100.0).
 */
float
control_port_normalized_val_to_real (
  const Port * const self,
  float              normalized_val)
{
  const PortIdentifier * const id = &self->id_;
  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::PLUGIN_CONTROL))
    {
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::LOGARITHMIC))
        {
          /* make sure none of the values is 0 */
          float minf =
            math_floats_equal (self->minf_, 0.f) ? 1e-20f : self->minf_;
          float maxf =
            math_floats_equal (self->maxf_, 0.f) ? 1e-20f : self->maxf_;
          normalized_val =
            math_floats_equal (normalized_val, 0.f) ? 1e-20f : normalized_val;

          /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
          return minf * powf (maxf / minf, normalized_val);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::TOGGLE))
        {
          return normalized_val >= 0.001f ? 1.f : 0.f;
        }
      else
        {
          return self->minf_ + normalized_val * (self->maxf_ - self->minf_);
        }
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::TOGGLE))
    {
      return normalized_val > 0.0001f;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::CHANNEL_FADER))
    {
      return (float) math_get_amp_val_from_fader (normalized_val);
    }
  else
    {
      return self->minf_ + normalized_val * (self->maxf_ - self->minf_);
    }
  g_return_val_if_reached (normalized_val);
}

/**
 * Converts real value (eg. -10.0 to 100.0) to
 * normalized value (0.0 to 1.0).
 *
 * @note This behaves differently from
 *   \ref port_set_control_value() and
 *   \ref port_get_control_value() and should be
 *   used in widgets.
 */
float
control_port_real_val_to_normalized (const Port * const self, float real_val)
{
  const PortIdentifier * const id = &self->id_;
  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::PLUGIN_CONTROL))
    {
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, self->id_.flags_,
          PortIdentifier::Flags::LOGARITHMIC))
        {
          /* make sure none of the values is 0 */
          float minf =
            math_floats_equal (self->minf_, 0.f) ? 1e-20f : self->minf_;
          float maxf =
            math_floats_equal (self->maxf_, 0.f) ? 1e-20f : self->maxf_;
          real_val = math_floats_equal (real_val, 0.f) ? 1e-20f : real_val;

          /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
          return logf (real_val / minf) / logf (maxf / minf);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, self->id_.flags_, PortIdentifier::Flags::TOGGLE))
        {
          return real_val;
        }
      else
        {
          float sizef = self->maxf_ - self->minf_;
          return (sizef - (self->maxf_ - real_val)) / sizef;
        }
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::TOGGLE))
    {
      return real_val;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::CHANNEL_FADER))
    {
      return (float) math_get_fader_val_from_amp (real_val);
    }
  else
    {
      float sizef = self->maxf_ - self->minf_;
      return (sizef - (self->maxf_ - real_val)) / sizef;
    }
  g_return_val_if_reached (0.f);
}

/**
 * Updates the actual value.
 *
 * The given value is always a normalized 0.0-1.0
 * value and must be translated to the actual value
 * before setting it.
 *
 * @param automating Whether this is from an
 *   automation event. This will set Lv2Port's
 *   automating field to true, which will cause the
 *   plugin to receive a UI event for this change.
 */
void
control_port_set_val_from_normalized (Port * self, float val, bool automating)
{
  PortIdentifier * id = &self->id_;
  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::PLUGIN_CONTROL))
    {
      float real_val = control_port_normalized_val_to_real (self, val);
      if (!math_floats_equal (self->control_, real_val))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, self);

          self->set_control_value (real_val, F_NOT_NORMALIZED, F_PUBLISH_EVENTS);
        }

      self->automating_ = automating;
      self->base_value_ = real_val;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::TOGGLE))
    {
      float real_val = control_port_normalized_val_to_real (self, val);
      if (!math_floats_equal (self->control_, real_val))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, self);
          self->control_ = control_port_is_val_toggled (real_val) ? 1.f : 0.f;
        }

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::FADER_MUTE))
        {
          Track * track = self->get_track (1);
          track_set_muted (
            track, control_port_is_toggled (self), F_NO_TRIGGER_UNDO,
            F_NO_AUTO_SELECT, F_PUBLISH_EVENTS);
        }
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::CHANNEL_FADER))
    {
      Track *   track = self->get_track (1);
      Channel * ch = track_get_channel (track);
      g_return_if_fail (ch);
      if (!math_floats_equal (fader_get_fader_val (ch->fader), val))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, self);
        }
      fader_set_amp (ch->fader, (float) math_get_amp_val_from_fader (val));
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::STEREO_BALANCE))
    {
      Track *   track = self->get_track (true);
      Channel * ch = track_get_channel (track);
      g_return_if_fail (ch);
      if (!math_floats_equal (ch->get_balance_control (), val))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, self);
        }
      ch->set_balance_control (val);
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::MIDI_AUTOMATABLE))
    {
      float real_val = self->minf_ + val * (self->maxf_ - self->minf_);
      if (!math_floats_equal (val, self->control_))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, self);
        }
      self->set_control_value (real_val, 0, 0);
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::AUTOMATABLE))
    {
      float real_val = control_port_normalized_val_to_real (self, val);
      if (!math_floats_equal (real_val, self->control_))
        {
          EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_CHANGED, self);
        }
      self->set_control_value (real_val, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::AMPLITUDE))
    {
      float real_val = control_port_normalized_val_to_real (self, val);
      self->set_control_value (real_val, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
    }
  else
    {
      g_return_if_reached ();
    }
}
