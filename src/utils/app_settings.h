// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/isettings_backend.h"
#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/utf8_string.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QtQmlIntegration/qqmlintegration.h>

using namespace Qt::StringLiterals;

#define DEFINE_SETTING_PROPERTY(ptype, name, default_value) \
\
  Q_PROPERTY (ptype name READ name WRITE set_##name NOTIFY name##Changed) \
\
public: \
  [[nodiscard]] static ptype get_default_##name () \
  { \
    return default_value; \
  } \
  [[nodiscard]] ptype name () const \
  { \
    return backend_->value (QStringLiteral (#name), default_value) \
      .value<ptype> (); \
  } \
  void set_##name (ptype value) \
  { \
    const auto current_value = \
      backend_->value (QStringLiteral (#name), default_value).value<ptype> (); \
    if (utils::values_equal_for_qproperty_type (current_value, value)) \
      return; \
    z_debug ("setting '{}' to '{}'", #name, value); \
    backend_->setValue (QStringLiteral (#name), value); \
    Q_EMIT name##Changed (value); \
  } \
  Q_SIGNAL void name##Changed (ptype value);

namespace zrythm::utils
{
class AppSettings : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

  // FIXME: should be part of DirectoryManager, not an editable setting
  DEFINE_SETTING_PROPERTY (
    QString,
    zrythm_user_path,
    QStandardPaths::writableLocation (QStandardPaths::AppDataLocation))
  DEFINE_SETTING_PROPERTY (bool, first_run, true)
  // DEFINE_SETTING_PROPERTY (bool, transportLoop, true)
  DEFINE_SETTING_PROPERTY (bool, transportReturnToCue, true)
  // note: in amplitude (0 to 2)
  DEFINE_SETTING_PROPERTY (double, metronomeVolume, 1.0)
  DEFINE_SETTING_PROPERTY (bool, metronomeEnabled, -1)
  DEFINE_SETTING_PROPERTY (int, metronomeCountIn, 0) // number of bars
  // DEFINE_SETTING_PROPERTY (bool, punchModeEnabled, false)
  DEFINE_SETTING_PROPERTY (bool, startPlaybackOnMidiInput, false)
  DEFINE_SETTING_PROPERTY (int, recordingMode, 2)     // takes
  DEFINE_SETTING_PROPERTY (int, recordingPreroll, 0)  // number of bars
  DEFINE_SETTING_PROPERTY (int, jackTransportType, 0) // timebase master
  DEFINE_SETTING_PROPERTY (QString, icon_theme, u"zrythm-dark"_s)
  DEFINE_SETTING_PROPERTY (QStringList, recent_projects, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, lv2_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, vst2_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, vst3_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, sf2_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, sfz_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, dssi_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, ladspa_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, clap_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, jsfx_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, au_search_paths, QStringList ())
  DEFINE_SETTING_PROPERTY (
    QString,
    new_project_directory,
    utils::Utf8String::from_path (
      (utils::Utf8String::from_qstring (
         QStandardPaths::writableLocation (QStandardPaths::DocumentsLocation))
         .to_path ()
       / utils::Utf8String::from_qstring (QCoreApplication::applicationName ())
           .to_path ()))
      .to_qstring ())
  DEFINE_SETTING_PROPERTY (bool, leftPanelVisible, true)
  DEFINE_SETTING_PROPERTY (bool, rightPanelVisible, true)
  DEFINE_SETTING_PROPERTY (bool, bottomPanelVisible, true)
  DEFINE_SETTING_PROPERTY (bool, trackAutoArm, true)
  DEFINE_SETTING_PROPERTY (int, audioBackend, 0) // dummy
  DEFINE_SETTING_PROPERTY (int, midiBackend, 0)  // dummy
  DEFINE_SETTING_PROPERTY (int, panLaw, 1)
  DEFINE_SETTING_PROPERTY (int, panAlgorithm, 2)
  DEFINE_SETTING_PROPERTY (QStringList, midiControllers, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, audioInputs, QStringList ())
  DEFINE_SETTING_PROPERTY (QStringList, fileBrowserBookmarks, QStringList ())
  DEFINE_SETTING_PROPERTY (QString, fileBrowserLastLocation, {})
  DEFINE_SETTING_PROPERTY (int, undoStackLength, 128)
  DEFINE_SETTING_PROPERTY (int, pianoRollHighlight, 3)    // both
  DEFINE_SETTING_PROPERTY (int, pianoRollMidiModifier, 0) // velocity
  /* these are all in amplitude (0.0 ~ 2.0) */
  DEFINE_SETTING_PROPERTY (float, monitorVolume, 1.0f)
  DEFINE_SETTING_PROPERTY (float, monitorMuteVolume, 0.0f)
  DEFINE_SETTING_PROPERTY (float, monitorListenVolume, 1.0f)
  DEFINE_SETTING_PROPERTY (float, monitorDimVolume, 0.1f)
  DEFINE_SETTING_PROPERTY (bool, monitorDimEnabled, false)
  DEFINE_SETTING_PROPERTY (bool, monitorMuteEnabled, false)
  DEFINE_SETTING_PROPERTY (bool, monitorMonoEnabled, false)
  DEFINE_SETTING_PROPERTY (bool, openPluginsOnInstantiation, true)
  // list of output devices to connect each channel to
  DEFINE_SETTING_PROPERTY (
    QStringList,
    monitorLeftOutputDeviceList,
    QStringList ())
  DEFINE_SETTING_PROPERTY (
    QStringList,
    monitorRightOutputDeviceList,
    QStringList ())
  DEFINE_SETTING_PROPERTY (bool, musicalMode, false)
  DEFINE_SETTING_PROPERTY (QString, rtAudioAudioDeviceName, {})
  DEFINE_SETTING_PROPERTY (int, sampleRate, 3)         // 48000
  DEFINE_SETTING_PROPERTY (int, audioBufferSize, 5)    // 512
  DEFINE_SETTING_PROPERTY (int, bounceTailLength, 100) // 100ms
  DEFINE_SETTING_PROPERTY (bool, bounceWithParents, 100)
  DEFINE_SETTING_PROPERTY (int, bounceStep, 2) // post-fader
  DEFINE_SETTING_PROPERTY (bool, disableAfterBounce, true)
  DEFINE_SETTING_PROPERTY (bool, autoSelectTracks, true)
  DEFINE_SETTING_PROPERTY (int, lastAutomationFunction, -1)
  DEFINE_SETTING_PROPERTY (int, lastAudioFunction, -1)
  DEFINE_SETTING_PROPERTY (double, lastAudioFunctionPitchShiftRatio, 1.0)
  DEFINE_SETTING_PROPERTY (int, lastMidiFunction, -1)
  DEFINE_SETTING_PROPERTY (QString, fileBrowserInstrument, {})
  DEFINE_SETTING_PROPERTY (int, automationCurveAlgorithm, 1) // superellipse
  DEFINE_SETTING_PROPERTY (double, timelineLastCreatedObjectLengthInTicks, 3840.0)
  DEFINE_SETTING_PROPERTY (double, editorLastCreatedObjectLengthInTicks, 480.0)
  DEFINE_SETTING_PROPERTY (QString, uiLocale, {})

public:
  explicit AppSettings (
    std::unique_ptr<ISettingsBackend> backend,
    QObject *                         parent = nullptr);

private:
  std::unique_ptr<ISettingsBackend> backend_;
};
}

#undef DEFINE_SETTING_PROPERTY
