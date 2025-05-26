// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_protocol.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::plugins;

TEST (PluginProtocolTest, StringConversion)
{
  // Test all protocol string conversions
  EXPECT_EQ (Protocol::to_string (Protocol::ProtocolType::Internal), "Internal");
  EXPECT_EQ (Protocol::to_string (Protocol::ProtocolType::LV2), "LV2");
  EXPECT_EQ (Protocol::to_string (Protocol::ProtocolType::VST3), "VST3");

  // Test round-trip conversion
  auto prot = Protocol::ProtocolType::CLAP;
  EXPECT_EQ (Protocol::from_string (Protocol::to_string (prot)), prot);

  // Test invalid string
  EXPECT_THROW (Protocol::from_string ("InvalidProtocol"), std::runtime_error);
}

TEST (PluginProtocolTest, JuceFormatConversion)
{
  EXPECT_EQ (
    Protocol::from_juce_format_name ("LV2"), Protocol::ProtocolType::LV2);
  EXPECT_EQ (
    Protocol::from_juce_format_name ("VST3"), Protocol::ProtocolType::VST3);
  EXPECT_THROW (
    Protocol::from_juce_format_name ("UnknownFormat"), std::runtime_error);
}

TEST (PluginProtocolTest, IconNames)
{
  EXPECT_EQ (Protocol::get_icon_name (Protocol::ProtocolType::LV2), "logo-lv2");
  EXPECT_EQ (Protocol::get_icon_name (Protocol::ProtocolType::VST), "logo-vst");
  EXPECT_EQ (
    Protocol::get_icon_name (Protocol::ProtocolType::SFZ), "file-music-line");
  EXPECT_EQ (
    Protocol::get_icon_name (Protocol::ProtocolType::Internal),
    "plug"); // Default
}

TEST (PluginProtocolTest, SupportDetection)
{
  // Test platform-specific support
  EXPECT_TRUE (Protocol::is_supported (Protocol::ProtocolType::LV2));

#ifdef __APPLE__
  EXPECT_TRUE (Protocol::is_supported (Protocol::ProtocolType::AudioUnit));
#else
  EXPECT_FALSE (Protocol::is_supported (Protocol::ProtocolType::AudioUnit));
#endif

#if defined(_WIN32) || defined(__APPLE__)
  EXPECT_FALSE (Protocol::is_supported (Protocol::ProtocolType::LADSPA));
#else
  EXPECT_TRUE (Protocol::is_supported (Protocol::ProtocolType::LADSPA));
#endif

#ifndef CARLA_HAVE_CLAP_SUPPORT
  EXPECT_FALSE (Protocol::is_supported (Protocol::ProtocolType::CLAP));
#endif
}
