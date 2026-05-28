// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string>

#include "utils/qt.h"
#include "utils/utf8_string.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::dsp
{

/**
 * @brief MIDI hardware input selection for a track.
 *
 * Stores which MIDI device and channel to use as input.
 * An empty device name means no input is selected.
 * midiChannel 0 = all channels, 1-16 = specific channel.
 */
class MidiInputSelection : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    QString deviceIdentifier READ deviceIdentifier WRITE setDeviceIdentifier
      NOTIFY deviceIdentifierChanged)
  Q_PROPERTY (
    int midiChannel READ midiChannel WRITE setMidiChannel NOTIFY
      midiChannelChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("Created by ProjectUiState")

public:
  explicit MidiInputSelection (QObject * parent = nullptr)
      : QObject (parent) { }
  ~MidiInputSelection () override = default;
  Q_DISABLE_COPY_MOVE (MidiInputSelection)

  QString       deviceIdentifier () const;
  void          setDeviceIdentifier (const QString &identifier);
  Q_SIGNAL void deviceIdentifierChanged ();

  int           midiChannel () const { return midi_channel_; }
  void          setMidiChannel (int ch);
  Q_SIGNAL void midiChannelChanged ();

  friend bool
  operator== (const MidiInputSelection &a, const MidiInputSelection &b);

private:
  friend void to_json (nlohmann::json &j, const MidiInputSelection &sel);
  friend void from_json (const nlohmann::json &j, MidiInputSelection &sel);

  utils::Utf8String device_identifier_;
  int midi_channel_ = 0; // 0 = omni (all channels), 1-16 = specific
};

} // namespace zrythm::dsp
