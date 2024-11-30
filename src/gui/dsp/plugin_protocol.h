// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "juce_wrapper.h"

namespace zrythm::gui::old_dsp::plugins
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
  enum class ProtocolType
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

  static bool is_supported (ProtocolType protocol);

  static std::string get_icon_name (ProtocolType prot);
};

} // namespace zrythm::gui::old_dsp::plugins
