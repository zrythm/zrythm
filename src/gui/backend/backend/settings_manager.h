// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SETTINGS_SETTINGS_MANAGER_H__
#define __SETTINGS_SETTINGS_MANAGER_H__

#include "zrythm-config.h"

#include "utils/logger.h"
#include "utils/math.h"

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace zrythm::gui
{

#define DEFINE_SETTING_PROPERTY_WITHOUT_SETTER(ptype, name, default_value) \
\
  Q_PROPERTY ( \
    ptype name READ get_##name WRITE set_##name NOTIFY name##_changed) \
\
public: \
  [[nodiscard]] static ptype get_default_##name () \
  { \
    return default_value; \
  } \
  [[nodiscard]] ptype get_##name () const \
  { \
    return settings_.value (QStringLiteral (#name), default_value) \
      .value<ptype> (); \
  } \
  [[nodiscard]] static ptype name () \
  { \
    return get_instance ()->get_##name (); \
  } \
\
  Q_SIGNAL void name##_changed ();

#define DEFINE_SETTING_PROPERTY(ptype, name, default_value) \
\
  DEFINE_SETTING_PROPERTY_WITHOUT_SETTER (ptype, name, default_value) \
  void set_##name (ptype value) \
  { \
    if ( \
      settings_.value (QStringLiteral (#name), default_value).value<ptype> () \
      != value) \
      { \
        z_debug ("setting '{}' to '{}'", #name, value); \
        settings_.setValue (QStringLiteral (#name), value); \
        Q_EMIT name##_changed (); \
        settings_.sync (); \
      } \
  }

#define DEFINE_SETTING_PROPERTY_DOUBLE(ptype, name, default_value) \
\
  DEFINE_SETTING_PROPERTY_WITHOUT_SETTER (ptype, name, default_value) \
  void set_##name (ptype value) \
  { \
    if ( \
      !utils::math::floats_equal ( \
        settings_.value (QStringLiteral (#name), default_value).value<ptype> (), \
        value)) \
      { \
        z_debug ("setting '{}' to '{}'", #name, value); \
        settings_.setValue (QStringLiteral (#name), value); \
        Q_EMIT name##_changed (); \
        settings_.sync (); \
      } \
  }

class SettingsManager final : public QObject
{
  Q_OBJECT

  // FIXME: should be part of DirectoryManager, not an editable setting
  DEFINE_SETTING_PROPERTY (
    QString,
    zrythm_user_path,
    QStandardPaths::writableLocation (QStandardPaths::AppDataLocation))
  DEFINE_SETTING_PROPERTY (bool, first_run, true)
  DEFINE_SETTING_PROPERTY (bool, transportLoop, true)
  DEFINE_SETTING_PROPERTY (bool, transportReturnToCue, true)
  // note: in amplitude (0 to 2)
  DEFINE_SETTING_PROPERTY_DOUBLE (double, metronomeVolume, 1.0)
  DEFINE_SETTING_PROPERTY (bool, metronomeEnabled, -1)
  DEFINE_SETTING_PROPERTY (int, metronomeCountIn, 0) // none
  DEFINE_SETTING_PROPERTY (bool, punchModeEnabled, false)
  DEFINE_SETTING_PROPERTY (bool, startPlaybackOnMidiInput, false)
  DEFINE_SETTING_PROPERTY (int, recordingMode, 2)     // takes
  DEFINE_SETTING_PROPERTY (int, recordingPreroll, 0)  // none
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
    utils::io::to_qstring ((
      utils::io::to_fs_path (
        QStandardPaths::writableLocation (QStandardPaths::DocumentsLocation))
      / utils::io::to_fs_path (QCoreApplication::applicationName ()))))
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
  DEFINE_SETTING_PROPERTY_DOUBLE (float, monitorVolume, 1.0f)
  DEFINE_SETTING_PROPERTY_DOUBLE (float, monitorMuteVolume, 0.0f)
  DEFINE_SETTING_PROPERTY_DOUBLE (float, monitorListenVolume, 1.0f)
  DEFINE_SETTING_PROPERTY_DOUBLE (float, monitorDimVolume, 0.1f)
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
  DEFINE_SETTING_PROPERTY_DOUBLE (double, lastAudioFunctionPitchShiftRatio, 1.0)
  DEFINE_SETTING_PROPERTY (int, lastMidiFunction, -1)
  DEFINE_SETTING_PROPERTY (QString, fileBrowserInstrument, {})
  DEFINE_SETTING_PROPERTY (int, automationCurveAlgorithm, 1) // superellipse
  DEFINE_SETTING_PROPERTY_DOUBLE (
    double,
    timelineLastCreatedObjectLengthInTicks,
    3840.0)
  DEFINE_SETTING_PROPERTY_DOUBLE (
    double,
    editorLastCreatedObjectLengthInTicks,
    480.0)
  DEFINE_SETTING_PROPERTY (QString, uiLocale, {})

public:
  SettingsManager (QObject * parent = nullptr);

  static SettingsManager * get_instance ();

  /**
   * @brief Resets all settings to their default values.
   */
  Q_INVOKABLE void reset_and_sync ();

private:
  QSettings settings_;
};

} // namespace zrythm::gui::glue

#undef DEFINE_SETTING_PROPERTY

#endif // __SETTINGS_SETTINGS_MANAGER_H__
