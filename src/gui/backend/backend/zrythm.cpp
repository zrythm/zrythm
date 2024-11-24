// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"
#include "gui/backend/zrythm_application.h"

#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"

#ifndef _WIN32
#  include <sys/mman.h>
#endif

# include "gui/dsp/recording_manager.h"
#include "gui/backend/plugin_manager.h"
#include "utils/dsp.h"
#include "utils/env.h"
#include "utils/exceptions.h"
#include "utils/io.h"
#include "utils/networking.h"
#include "utils/string.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/chord_preset_pack_manager.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"

#include "engine/audio_engine_application.h"

using namespace zrythm;

JUCE_IMPLEMENT_SINGLETON (Zrythm);

using namespace Qt::StringLiterals;

Zrythm::Zrythm ()
    : plugin_manager_ (std::make_unique<zrythm::gui::dsp::plugins::PluginManager> ())
{
  z_return_if_fail (!engine::AudioEngineApplication::is_audio_engine_process ());
  elapsed_timer_.start ();
}

void
Zrythm::pre_init (const char * exe_path, bool have_ui, bool optimized_dsp)
{
  if (exe_path)
    exe_path_ = exe_path;

  have_ui_ = have_ui;
  use_optimized_dsp_ = optimized_dsp;
  if (use_optimized_dsp_)
    {
      lsp_dsp_context_ = std::make_unique<DspContextRAII> ();
    }
  settings_ = std::make_unique<Settings> ();

  debug_ = (env_get_int ("ZRYTHM_DEBUG", 0) != 0);
  break_on_error_ = (env_get_int ("ZRYTHM_BREAK_ON_ERROR", 0) != 0);
  benchmarking_ = (env_get_int ("ZRYTHM_BENCHMARKING", 0) != 0);
}

void
Zrythm::init ()
{
  settings_->init ();
  recording_manager_ = new RecordingManager (this);
  chord_preset_pack_manager_ = std::make_unique<ChordPresetPackManager> (
    have_ui_ && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING);

  if (have_ui_)
    {
      // event_manager_ = std::make_unique<EventManager> ();
    }
}

std::string
Zrythm::get_version (bool with_v)
{
  constexpr const char * ver = PACKAGE_VERSION;

  if (with_v)
    {
      if (ver[0] == 'v')
        return ver;

      return fmt::format ("v{}", ver);
    }
  else
    {
      if (ver[0] == 'v')
        return &ver[1];

      return ver;
    }
}

/**
 * Returns the version and the capabilities.
 *
 * @param buf Buffer to write the string to.
 * @param include_system_info Whether to include
 *   additional system info (for bug reports).
 */
void
Zrythm::get_version_with_capabilities (char * buf, bool include_system_info)
{
  std::string ver = get_version (false);

  std::string gstr = fmt::format (
    "{} {}{} ({})\n"
    "  built with {} {} for {}{}\n"
#if HAVE_CARLA
    "    +carla\n"
#endif

#if HAVE_JACK2
    "    +jack2\n"
#elif defined(HAVE_JACK)
    "    +jack1\n"
#endif

#ifdef MANUAL_PATH
    "    +manual\n"
#endif
#if HAVE_PULSEAUDIO
    "    +pulse\n"
#endif
#if HAVE_RTMIDI
    "    +rtmidi\n"
#endif
#if HAVE_RTAUDIO
    "    +rtaudio\n"
#endif

    "",
    PROGRAM_NAME,

#if ZRYTHM_IS_TRIAL_VER
    "(trial) ",
#else
    "",
#endif

    ver,

    BUILD_TYPE,

    COMPILER, COMPILER_VERSION, HOST_MACHINE_SYSTEM,

#ifdef APPIMAGE_BUILD
    " (appimage)"
#elif ZRYTHM_IS_INSTALLER_VER
    " (installer)"
#else
    ""
#endif

  );

  if (include_system_info)
    {
      gstr += "\n";
      const auto system_nfo = get_system_info ();
      gstr += system_nfo;
    }

  strcpy (buf, gstr.c_str ());
}

std::string
Zrythm::get_system_info ()
{
  std::string gstr;

  QFile file (u"/etc/os-release"_s);
  if (file.open (QIODevice::ReadOnly | QIODevice::Text))
    {
      QTextStream in (&file);
      gstr += in.readAll ().toStdString () + "\n";
    }

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment ();
  gstr += fmt::format ("XDG_SESSION_TYPE={}\n", env.value (u"XDG_SESSION_ID"_s));
  gstr += fmt::format (
    "XDG_CURRENT_DESKTOP={}\n", env.value (u"XDG_CURRENT_DESKTOP"_s));
  gstr += fmt::format ("DESKTOP_SESSION={}\n", env.value (u"DESKTOP_SESSION"_s));

  gstr += "\n";

#if HAVE_CARLA
  gstr += fmt::format ("Carla version: {}\n", Z_CARLA_VERSION_STRING);
#endif
#if 0
  gstr += fmt::format (
    "GTK version: {}.{}.{}\n", gtk_get_major_version (),
    gtk_get_minor_version (), gtk_get_micro_version ());
  gstr += fmt::format (
    "libadwaita version: {}.{}.{}\n", adw_get_major_version (),
    adw_get_minor_version (), adw_get_micro_version ());
  gstr += fmt::format (
    "libpanel version: {}.{}.{}\n", panel_get_major_version (),
    panel_get_minor_version (), panel_get_micro_version ());
#endif

  gstr += fmt::format (
    "QT version: {}.{}.{}\n", QT_VERSION_MAJOR, QT_VERSION_MINOR,
    QT_VERSION_PATCH);

  return gstr;
}

/**
 * Returns whether the current Zrythm version is a
 * release version.
 *
 * @note This only does regex checking.
 */
bool
Zrythm::is_release (bool official)
{
#if !ZRYTHM_IS_INSTALLER_VER
  if (official)
    {
      return false;
    }
#endif

  return !utils::string::contains_substr (PACKAGE_VERSION, "g");
}

/**
 * @param callback A GAsyncReadyCallback to call when the
 *   request is satisfied.
 * @param callback_data Data to pass to @p callback.
 */
void
Zrythm::fetch_latest_release_ver_async (
  networking::URL::GetContentsAsyncCallback callback)
{
  networking::URL ("https://www.zrythm.org/zrythm-version.txt")
    .get_page_contents_async (8000, callback);
}

/**
 * Returns whether the given release string is the latest
 * release.
 */
bool
Zrythm::is_latest_release (const char * remote_latest_release)
{
  return string_is_equal (remote_latest_release, PACKAGE_VERSION);
}

void
Zrythm::init_user_dirs_and_files ()
{
  z_info ("initing dirs and files");

  auto & dir_mgr = dynamic_cast<gui::ZrythmApplication*>(qApp)->get_directory_manager ();
  for (
    auto dir_type :
    { DirectoryManager::DirectoryType::USER_TOP,
      DirectoryManager::DirectoryType::USER_PROJECTS,
      DirectoryManager::DirectoryType::USER_TEMPLATES,
      DirectoryManager::DirectoryType::USER_LOG,
      DirectoryManager::DirectoryType::USER_SCRIPTS,
      DirectoryManager::DirectoryType::USER_THEMES,
      DirectoryManager::DirectoryType::USER_THEMES_CSS,
      DirectoryManager::DirectoryType::USER_PROFILING,
      DirectoryManager::DirectoryType::USER_GDB,
      DirectoryManager::DirectoryType::USER_BACKTRACE })
    {
      std::string dir = dir_mgr.get_dir (dir_type);
      z_return_if_fail (!dir.empty ());
      try
        {
          utils::io::mkdir (dir);
        }
      catch (const ZrythmException &e)
        {
          throw ZrythmException ("Failed to init user dires and files");
        }
    }
}

Zrythm::~Zrythm ()
{
  z_info ("Destroying Zrythm instance");
  clearSingletonInstance ();
}
