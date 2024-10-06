// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/utils/directory_manager.h"
#include "common/utils/gtest_wrapper.h"

#ifndef _WIN32
#  include <sys/mman.h>
#endif

#include "common/dsp/recording_manager.h"
#include "common/plugins/plugin_manager.h"
#include "common/utils/dsp.h"
#include "common/utils/env.h"
#include "common/utils/exceptions.h"
#include "common/utils/io.h"
#include "common/utils/networking.h"
#include "common/utils/string.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/chord_preset_pack_manager.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <giomm.h>
#include <glibmm.h>

JUCE_IMPLEMENT_SINGLETON (Zrythm);

void
Zrythm::pre_init (const char * exe_path, bool have_ui, bool optimized_dsp)
{
  if (exe_path)
    exe_path_ = exe_path;

  have_ui_ = have_ui;
  use_optimized_dsp_ = optimized_dsp;
  if (use_optimized_dsp_)
    {
      lsp_dsp_context_ = std::make_unique<LspDspContextRAII> ();
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
  recording_manager_ = std::make_unique<RecordingManager> ();
  plugin_manager_ = std::make_unique<zrythm::plugins::PluginManager> ();
  chord_preset_pack_manager_ = std::make_unique<ChordPresetPackManager> (
    have_ui_ && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING);

  if (have_ui_)
    {
      event_manager_ = std::make_unique<EventManager> ();
    }
}

void
Zrythm::add_to_recent_projects (const std::string &_filepath)
{
  recent_projects_.insert (0, _filepath.c_str ());

  /* if we are at max projects, remove the last one */
  if (recent_projects_.size () > MAX_RECENT_PROJECTS)
    {
      recent_projects_.remove (MAX_RECENT_PROJECTS);
    }

  char ** tmp = recent_projects_.getNullTerminated ();
  g_settings_set_strv (S_GENERAL, "recent-projects", (const char * const *) tmp);
  g_strfreev (tmp);
}

void
Zrythm::remove_recent_project (const std::string &filepath)
{
  recent_projects_.removeString (filepath.c_str ());

  char ** tmp = recent_projects_.getNullTerminated ();
  g_settings_set_strv (S_GENERAL, "recent-projects", (const char * const *) tmp);
  g_strfreev (tmp);
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

#if HAVE_LSP_DSP
    "    +lsp-dsp-lib\n"
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

  QFile file ("/etc/os-release");
  if (file.open (QIODevice::ReadOnly | QIODevice::Text))
    {
      QTextStream in (&file);
      gstr += in.readAll ().toStdString () + "\n";
    }

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment ();
  gstr += fmt::format ("XDG_SESSION_TYPE={}\n", env.value ("XDG_SESSION_ID"));
  gstr += fmt::format (
    "XDG_CURRENT_DESKTOP={}\n", Glib::getenv ("XDG_CURRENT_DESKTOP"));
  gstr += fmt::format ("DESKTOP_SESSION={}\n", Glib::getenv ("DESKTOP_SESSION"));

  gstr += "\n";

#if HAVE_CARLA
  gstr += fmt::format ("Carla version: {}\n", Z_CARLA_VERSION_STRING);
#endif
  gstr += fmt::format (
    "GTK version: {}.{}.{}\n", gtk_get_major_version (),
    gtk_get_minor_version (), gtk_get_micro_version ());
  gstr += fmt::format (
    "libadwaita version: {}.{}.{}\n", adw_get_major_version (),
    adw_get_minor_version (), adw_get_micro_version ());
  gstr += fmt::format (
    "libpanel version: {}.{}.{}\n", panel_get_major_version (),
    panel_get_minor_version (), panel_get_micro_version ());

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

  return !string_contains_substr (PACKAGE_VERSION, "g");
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

  auto * dir_mgr = DirectoryManager::getInstance ();
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
      std::string dir = dir_mgr->get_dir (dir_type);
      z_return_if_fail (!dir.empty ());
      try
        {
          io_mkdir (dir);
        }
      catch (const ZrythmException &e)
        {
          throw ZrythmException ("Failed to init user dires and files");
        }
    }
}

void
Zrythm::init_templates ()
{
  z_info ("Initializing templates...");

  auto *      dir_mgr = DirectoryManager::getInstance ();
  std::string user_templates_dir =
    dir_mgr->get_dir (DirectoryManager::DirectoryType::USER_TEMPLATES);
  try
    {
      templates_ = io_get_files_in_dir (user_templates_dir);
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to init user templates from {}", user_templates_dir);
    }
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      std::string system_templates_dir =
        dir_mgr->get_dir (DirectoryManager::DirectoryType::SYSTEM_TEMPLATES);
      try
        {
          templates_.addArray (io_get_files_in_dir (system_templates_dir));
        }
      catch (const ZrythmException &e)
        {
          z_warning (
            "Failed to init system templates from {}", system_templates_dir);
        }
    }

  for (auto &tmpl : this->templates_)
    {
      z_info ("Template found: {}", tmpl.toRawUTF8 ());
      if (tmpl.contains ("demo_zsong01"))
        {
          demo_template_ = tmpl.toStdString ();
        }
    }

  z_info ("done");
}

Zrythm::~Zrythm ()
{
  z_info ("Destroying Zrythm instance");
  clearSingletonInstance ();
}