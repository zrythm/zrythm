// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/utf8_string.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

using namespace std::string_view_literals;

namespace zrythm::dsp
{

/**
 * @brief Audio hardware input selection for a track.
 *
 * Stores which audio device and channels to use as input.
 * An empty device name means no input is selected.
 */
class AudioInputSelection : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    QString deviceName READ deviceName WRITE setDeviceName NOTIFY
      deviceNameChanged)
  Q_PROPERTY (
    int firstChannel READ firstChannel WRITE setFirstChannel NOTIFY
      firstChannelChanged)
  Q_PROPERTY (bool stereo READ stereo WRITE setStereo NOTIFY stereoChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("Created by ProjectUiState")

public:
  explicit AudioInputSelection (QObject * parent = nullptr) : QObject (parent)
  {
  }
  ~AudioInputSelection () override = default;
  Q_DISABLE_COPY_MOVE (AudioInputSelection)

  QString       deviceName () const;
  void          setDeviceName (const QString &name);
  Q_SIGNAL void deviceNameChanged ();

  int           firstChannel () const { return first_channel_; }
  void          setFirstChannel (int channel);
  Q_SIGNAL void firstChannelChanged ();

  bool          stereo () const { return stereo_; }
  void          setStereo (bool s);
  Q_SIGNAL void stereoChanged ();

  friend bool
  operator== (const AudioInputSelection &a, const AudioInputSelection &b);

  static constexpr auto kDeviceNameKey = "deviceName"sv;
  static constexpr auto kFirstChannelKey = "firstChannel"sv;
  static constexpr auto kStereoKey = "stereo"sv;

private:
  friend void to_json (nlohmann::json &j, const AudioInputSelection &sel);
  friend void from_json (const nlohmann::json &j, AudioInputSelection &sel);

  utils::Utf8String device_name_;
  uint8_t           first_channel_ = 0;
  bool              stereo_ = true;
};

} // namespace zrythm::dsp
