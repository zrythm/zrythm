// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_input_selection.h"

namespace zrythm::dsp
{

QString
AudioInputSelection::deviceName () const
{
  return device_name_.to_qstring ();
}

void
AudioInputSelection::setDeviceName (const QString &name)
{
  const auto new_name = utils::Utf8String::from_qstring (name);
  if (utils::values_equal_for_qproperty_type (device_name_, new_name))
    return;
  device_name_ = new_name;
  Q_EMIT deviceNameChanged ();
}

void
AudioInputSelection::setFirstChannel (int channel)
{
  channel = std::clamp (
    channel, 0,
    static_cast<int> (std::numeric_limits<decltype (first_channel_)>::max ()));
  if (
    utils::values_equal_for_qproperty_type (
      first_channel_, static_cast<uint8_t> (channel)))
    return;
  first_channel_ = static_cast<uint8_t> (channel);
  Q_EMIT firstChannelChanged ();
}

void
AudioInputSelection::setStereo (bool s)
{
  if (utils::values_equal_for_qproperty_type (stereo_, s))
    return;
  stereo_ = s;
  Q_EMIT stereoChanged ();
}

void
to_json (nlohmann::json &j, const AudioInputSelection &sel)
{
  j[AudioInputSelection::kDeviceNameKey] = sel.device_name_;
  j[AudioInputSelection::kFirstChannelKey] = sel.first_channel_;
  j[AudioInputSelection::kStereoKey] = sel.stereo_;
}

void
from_json (const nlohmann::json &j, AudioInputSelection &sel)
{
  j.at (AudioInputSelection::kDeviceNameKey).get_to (sel.device_name_);
  j.at (AudioInputSelection::kFirstChannelKey).get_to (sel.first_channel_);
  j.at (AudioInputSelection::kStereoKey).get_to (sel.stereo_);
}

bool
operator== (const AudioInputSelection &a, const AudioInputSelection &b)
{
  return a.device_name_ == b.device_name_
         && a.first_channel_ == b.first_channel_ && a.stereo_ == b.stereo_;
}

} // namespace zrythm::dsp
