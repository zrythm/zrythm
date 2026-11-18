// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_protocol.h"
#include "utils/enum_utils.h"

#include <juce_core/juce_core.h>

using namespace zrythm::plugins;

namespace
{
/**
 * @brief Protocols a JUCE plugin description can round-trip; their JUCE
 * format names equal the enumerator names.
 */
constexpr bool
juce_hosts (Protocol::ProtocolType protocol)
{
  switch (protocol)
    {
    case Protocol::ProtocolType::Internal:
    case Protocol::ProtocolType::LV2:
    case Protocol::ProtocolType::LADSPA:
    case Protocol::ProtocolType::VST:
    case Protocol::ProtocolType::VST3:
    case Protocol::ProtocolType::AudioUnit:
    case Protocol::ProtocolType::CLAP:
      return true;
    default:
      return false;
    }
}
} // namespace

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
  return std::string{ magic_enum::enum_name (prot) };
}

Protocol::ProtocolType
Protocol::from_string (const std::string &str)
{
  const auto parsed = magic_enum::enum_cast<ProtocolType> (str);
  if (!parsed.has_value ())
    {
      throw std::runtime_error ("Invalid protocol type: " + str);
    }
  return *parsed;
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

  return true;
}

Protocol::ProtocolType
Protocol::from_juce_format_name (const juce::String &str)
{
  const auto parsed = magic_enum::enum_cast<ProtocolType> (str.toStdString ());
  if (!parsed.has_value () || !juce_hosts (*parsed))
    {
      throw std::runtime_error ("Unknown protocol type: " + str.toStdString ());
    }
  return *parsed;
}

juce::String
Protocol::to_juce_format_name (ProtocolType type)
{
  if (!juce_hosts (type))
    {
      throw std::runtime_error (
        fmt::format ("Unknown protocol type: {}", ENUM_NAME (type)));
    }
  return juce::String{ std::string{ magic_enum::enum_name (type) } };
}
