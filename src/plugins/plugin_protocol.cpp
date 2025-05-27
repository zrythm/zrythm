// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_protocol.h"
#include "utils/bidirectional_map.h"
#include "utils/types.h"

using namespace zrythm::plugins;

constexpr std::array<std::string_view, 11> plugin_protocol_strings = {
  "Internal",  "LV2", "DSSI", "LADSPA", "VST",  "VST3",
  "AudioUnit", "SFZ", "SF2",  "CLAP",   "JSFX",
};

std::string
Protocol::get_icon_name (ProtocolType prot)
{
  std::string icon;
  switch (prot)
    {
    case ProtocolType::LV2:
      icon = "logo-lv2";
      break;
    case ProtocolType::LADSPA:
      icon = "logo-ladspa";
      break;
    case ProtocolType::AudioUnit:
      icon = "logo-au";
      break;
    case ProtocolType::VST:
    case ProtocolType::VST3:
      icon = "logo-vst";
      break;
    case ProtocolType::SFZ:
    case ProtocolType::SF2:
      icon = "file-music-line";
      break;
    default:
      icon = "plug";
      break;
    }
  return icon;
}

std::string
Protocol::to_string (ProtocolType prot)
{
  return std::string{ plugin_protocol_strings.at (ENUM_VALUE_TO_INT (prot)) };
}

Protocol::ProtocolType
Protocol::from_string (const std::string &str)
{
  size_t i = 0;
  for (const auto &cur_str : plugin_protocol_strings)
    {
      if (cur_str == str)
        {
          return (ProtocolType) i;
        }
      ++i;
    }
  throw std::runtime_error ("Invalid protocol type: " + str);
}

bool
Protocol::is_supported (ProtocolType protocol)
{
#ifndef __APPLE__
  if (protocol == ProtocolType::AudioUnit)
    return false;
#endif
#if defined(_WIN32) || defined(__APPLE__)
  if (protocol == ProtocolType::LADSPA || protocol == ProtocolType::DSSI)
    return false;
#endif
#ifndef CARLA_HAVE_CLAP_SUPPORT
  if (protocol == ProtocolType::CLAP)
    return false;
#endif
  return true;
}

namespace
{
const zrythm::utils::ConstBidirectionalMap<Protocol::ProtocolType, juce::String>
  juce_format_mappings = {
    { Protocol::ProtocolType::LV2,       "LV2"       },
    { Protocol::ProtocolType::LADSPA,    "LADSPA"    },
    { Protocol::ProtocolType::VST,       "VST"       },
    { Protocol::ProtocolType::VST3,      "VST3"      },
    { Protocol::ProtocolType::AudioUnit, "AudioUnit" },
    { Protocol::ProtocolType::CLAP,      "CLAP"      },
};
}

Protocol::ProtocolType
Protocol::from_juce_format_name (const juce::String &str)
{
  const auto ret = juce_format_mappings.find_by_value (str);
  if (ret)
    return *ret;
  throw std::runtime_error ("Unknown protocol type: " + str.toStdString ());
}

juce::String
Protocol::to_juce_format_name (ProtocolType type)
{
  const auto ret = juce_format_mappings.find_by_key (type);
  if (ret)
    return *ret;
  throw std::runtime_error (
    fmt::format ("Unknown protocol type: {}", ENUM_NAME (type)));
}
