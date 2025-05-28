// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "juce_wrapper.h"

namespace zrythm::plugins
{

class Protocol : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_NAMED_ELEMENT (ProtocolType)

public:
  /**
   * Plugin protocol.
   */
  enum class ProtocolType : std::uint_fast8_t
  {
    /** Dummy protocol for tests. */
    Internal,
    LV2,
    DSSI,
    LADSPA,
    VST,
    VST3,
    AudioUnit,
    SFZ,
    SF2,
    CLAP,
    JSFX,
  };
  Q_ENUM (ProtocolType)

public:
  static std::string to_string (ProtocolType prot);

  static ProtocolType from_string (const std::string &str);

  static ProtocolType from_juce_format_name (const juce::String &str);
  static juce::String to_juce_format_name (ProtocolType prot);

  static bool is_supported (ProtocolType protocol);

  static std::string get_icon_name (ProtocolType prot);
};

} // namespace zrythm::plugins
