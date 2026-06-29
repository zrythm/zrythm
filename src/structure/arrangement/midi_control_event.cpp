// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/midi_control_event.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{

MidiControlEvent::MidiControlEvent (
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  QObject *                   parent)
    : ArrangerObject (
        Type::MidiControlEvent,
        tempo_map_wrapper,
        ArrangerObjectFeatures::ClipOwned,
        parent)
{
  QObject::connect (
    this, &MidiControlEvent::controlTypeChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    this, &MidiControlEvent::channelChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    this, &MidiControlEvent::controllerChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    this, &MidiControlEvent::valueChanged, this,
    &ArrangerObject::propertiesChanged);
}

MidiControlEvent::~MidiControlEvent () = default;

MidiControlEvent::EventType
MidiControlEvent::controlType () const
{
  return type_;
}

void
MidiControlEvent::setControlType (EventType type)
{
  if (type_ == type)
    return;

  type_ = type;

  const auto max_val = maxValueForType ();
  if (value_ > static_cast<std::uint16_t> (max_val))
    {
      value_ = static_cast<std::uint16_t> (max_val);
      Q_EMIT valueChanged (static_cast<int> (value_));
    }

  Q_EMIT controlTypeChanged (type_);
}

void
MidiControlEvent::setChannel (int ch)
{
  const auto clamped = std::clamp (ch, 0, kMaxChannel);
  const auto new_ch = static_cast<std::uint8_t> (clamped);
  if (channel_ == new_ch)
    return;
  channel_ = new_ch;
  Q_EMIT channelChanged (channel_);
}

void
MidiControlEvent::setController (int ctrl)
{
  const auto clamped = std::clamp (ctrl, 0, kMaxController);
  const auto new_ctrl = static_cast<std::uint8_t> (clamped);
  if (controller_ == new_ctrl)
    return;
  controller_ = new_ctrl;
  Q_EMIT controllerChanged (controller_);
}

void
MidiControlEvent::setValue (int val)
{
  const auto max_val = maxValueForType ();
  const auto clamped = std::clamp (val, 0, max_val);
  const auto new_val = static_cast<std::uint16_t> (clamped);
  if (value_ == new_val)
    return;
  value_ = new_val;
  Q_EMIT valueChanged (value_);
}

int
MidiControlEvent::maxValueForType () const
{
  switch (type_)
    {
    case EventType::PitchBend:
      return kMaxValuePitchBend;
    case EventType::ControlChange:
    case EventType::ChannelPressure:
    case EventType::PolyKeyPressure:
    case EventType::ProgramChange:
      return kMaxValue7Bit;
    }
  return kMaxValue7Bit;
}

void
init_from (
  MidiControlEvent       &obj,
  const MidiControlEvent &other,
  utils::ObjectCloneType  clone_type)
{
  obj.type_ = other.type_;
  obj.channel_ = other.channel_;
  obj.controller_ = other.controller_;
  obj.value_ = other.value_;
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}

void
to_json (nlohmann::json &j, const MidiControlEvent &ev)
{
  to_json (j, static_cast<const ArrangerObject &> (ev));
  j[MidiControlEvent::kTypeKey] = static_cast<int> (ev.type_);
  j[MidiControlEvent::kChannelKey] = ev.channel_;
  j[MidiControlEvent::kControllerKey] = ev.controller_;
  j[MidiControlEvent::kValueKey] = ev.value_;
}

void
from_json (const nlohmann::json &j, MidiControlEvent &ev)
{
  from_json (j, static_cast<ArrangerObject &> (ev));
  const auto type_int = j.at (MidiControlEvent::kTypeKey).get<int> ();
  if (
    type_int < 0
    || type_int > static_cast<int> (MidiControlEvent::EventType::ProgramChange))
    {
      throw std::out_of_range (
        fmt::format (
          "Invalid MidiControlEvent type: {}. Expected 0-{}.", type_int,
          static_cast<int> (MidiControlEvent::EventType::ProgramChange)));
    }
  ev.type_ = static_cast<MidiControlEvent::EventType> (type_int);
  ev.channel_ = j.at (MidiControlEvent::kChannelKey).get<std::uint8_t> ();
  ev.controller_ = j.at (MidiControlEvent::kControllerKey).get<std::uint8_t> ();
  ev.value_ = j.at (MidiControlEvent::kValueKey).get<std::uint16_t> ();
}

}
