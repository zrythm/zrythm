// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_input_selection.h"

#include <nlohmann/json.hpp>

using namespace std::string_view_literals;

namespace zrythm::dsp
{

namespace
{
constexpr auto kDeviceIdentifierKey = "deviceIdentifier"sv;
constexpr auto kMidiChannelKey = "midiChannel"sv;
}

QString
MidiInputSelection::deviceIdentifier () const
{
  return device_identifier_.to_qstring ();
}

void
MidiInputSelection::setDeviceIdentifier (const QString &identifier)
{
  const auto new_id = utils::Utf8String::from_qstring (identifier);
  if (utils::values_equal_for_qproperty_type (device_identifier_, new_id))
    return;
  device_identifier_ = new_id;
  Q_EMIT deviceIdentifierChanged ();
}

void
MidiInputSelection::setMidiChannel (int ch)
{
  ch = std::clamp (ch, 0, 16);
  if (utils::values_equal_for_qproperty_type (midi_channel_, ch))
    return;
  midi_channel_ = ch;
  Q_EMIT midiChannelChanged ();
}

void
to_json (nlohmann::json &j, const MidiInputSelection &sel)
{
  j[kDeviceIdentifierKey] = sel.device_identifier_;
  j[kMidiChannelKey] = sel.midi_channel_;
}

void
from_json (const nlohmann::json &j, MidiInputSelection &sel)
{
  utils::Utf8String device_id;
  j.at (kDeviceIdentifierKey).get_to (device_id);
  sel.setDeviceIdentifier (device_id.to_qstring ());

  int ch = 0;
  j.at (kMidiChannelKey).get_to (ch);
  sel.setMidiChannel (ch);
}

bool
operator== (const MidiInputSelection &a, const MidiInputSelection &b)
{
  return a.device_identifier_ == b.device_identifier_
         && a.midi_channel_ == b.midi_channel_;
}

} // namespace zrythm::dsp
