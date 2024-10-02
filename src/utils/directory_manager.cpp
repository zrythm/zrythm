// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "settings/settings_manager.h"
#include "utils/directory_manager.h"
#include "utils/io.h"

JUCE_IMPLEMENT_SINGLETON (DirectoryManager)

fs::path
DirectoryManager::get_prefix ()
{
  // return parent path of the executable's owner path (1 level above "bin")
  return fs::path (qApp->applicationDirPath ().toStdString ())
    .parent_path ()
    .string ();
#if 0
#  if defined(_WIN32) && ZRYTHM_IS_INSTALLER_VER
  return io_get_registry_string_val ("InstallPath");
#  elif defined(__APPLE__) && ZRYTHM_IS_INSTALLER_VER
  char bundle_path[PATH_MAX];
  int  ret = io_get_bundle_path (bundle_path);
  z_return_val_if_fail (ret == 0, nullptr);
  return io_path_get_parent_dir (bundle_path);
#  elif defined(APPIMAGE_BUILD)
  z_return_val_if_fail (zrythm_app->appimage_runtime_path, nullptr);
  return g_build_filename (zrythm_app->appimage_runtime_path, "usr", nullptr);
#  else
  return ZRYTHM_PREFIX;
#  endif
#endif
}

void
DirectoryManager::remove_testing_dir ()
{
  if (testing_dir_.empty ())
    return;

  io_rmdir (testing_dir_, true);
  testing_dir_.clear ();
}

const fs::path &
DirectoryManager::get_testing_dir ()
{
  if (testing_dir_.empty ())
    {
      char * new_testing_dir =
        g_dir_make_tmp ("zrythm_test_dir_XXXXXX", nullptr);
      testing_dir_ = new_testing_dir;
      g_free (new_testing_dir);
    }
  return testing_dir_;
}

fs::path
DirectoryManager::get_user_dir (bool force_default)
{
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      return get_testing_dir ();
    }

  fs::path dir =
    SettingsManager::get_instance ()->get_zrythm_user_path ().toStdString ();
  if (force_default || dir.empty ())
    {
      dir =
        SettingsManager::get_instance ()
          ->get_default_zrythm_user_path ()
          .toStdString ();
    }

  return dir;
}

fs::path
DirectoryManager::get_default_user_dir ()
{
  return get_user_dir (true);
}

fs::path
DirectoryManager::get_dir (DirectoryManager::DirectoryType type)
{
  /* handle system dirs */
  if (type < DirectoryManager::DirectoryType::USER_TOP)
    {
      fs::path prefix = get_prefix ();

      switch (type)
        {
        case DirectoryManager::DirectoryType::SYSTEM_PREFIX:
          return prefix;
        case DirectoryManager::DirectoryType::SYSTEM_BINDIR:
          return prefix / BINDIR_NAME;
        case DirectoryManager::DirectoryType::SYSTEM_PARENT_DATADIR:
          return prefix / DATADIR_NAME;
        case DirectoryManager::DirectoryType::SYSTEM_PARENT_LIBDIR:
          return prefix / LIBDIR_NAME;
        case DirectoryManager::DirectoryType::SYSTEM_ZRYTHM_LIBDIR:
          {
            fs::path parent_path =
              get_dir (DirectoryManager::DirectoryType::SYSTEM_PARENT_LIBDIR);
            return parent_path / "zrythm";
          }
        case DirectoryManager::DirectoryType::SYSTEM_BUNDLED_PLUGINSDIR:
          {
            fs::path parent_path =
              get_dir (DirectoryManager::DirectoryType::SYSTEM_ZRYTHM_LIBDIR);
            return parent_path / "lv2";
          }
        case DirectoryManager::DirectoryType::SYSTEM_LOCALEDIR:
          return prefix / DATADIR_NAME / "locale";
        case DirectoryManager::DirectoryType::SYSTEM_SOURCEVIEW_LANGUAGE_SPECS_DIR:
          return prefix / DATADIR_NAME / "gtksourceview-5" / "language-specs";
        case DirectoryManager::DirectoryType::
          SYSTEM_BUNDLED_SOURCEVIEW_LANGUAGE_SPECS_DIR:
          return prefix / DATADIR_NAME / "zrythm" / "gtksourceview-5"
                 / "language-specs";
        case DirectoryManager::DirectoryType::SYSTEM_ZRYTHM_DATADIR:
          return prefix / DATADIR_NAME / "zrythm";
        case DirectoryManager::DirectoryType::SYSTEM_SAMPLESDIR:
          return prefix / DATADIR_NAME / "zrythm" / "samples";
        case DirectoryManager::DirectoryType::SYSTEM_SCRIPTSDIR:
          return prefix / DATADIR_NAME / "zrythm" / "scripts";
        case DirectoryManager::DirectoryType::SYSTEM_THEMESDIR:
          return prefix / DATADIR_NAME / "zrythm" / "themes";
        case DirectoryManager::DirectoryType::SYSTEM_THEMES_CSS_DIR:
          {
            fs::path parent_path =
              get_dir (DirectoryManager::DirectoryType::SYSTEM_THEMESDIR);
            return parent_path / "css";
          }
        case DirectoryManager::DirectoryType::SYSTEM_THEMES_ICONS_DIR:
          {
            fs::path parent_path =
              get_dir (DirectoryManager::DirectoryType::SYSTEM_THEMESDIR);
            return parent_path / "icons";
          }
        case DirectoryManager::DirectoryType::SYSTEM_SPECIAL_LV2_PLUGINS_DIR:
          return prefix / DATADIR_NAME / "zrythm" / "lv2";
        case DirectoryManager::DirectoryType::SYSTEM_FONTSDIR:
          return prefix / DATADIR_NAME / "fonts" / "zrythm";
        case DirectoryManager::DirectoryType::SYSTEM_TEMPLATES:
          return prefix / DATADIR_NAME / "zrythm" / "templates";
        default:
          break;
        }
    }
  /* handle user dirs */
  else
    {
      fs::path user_dir = get_user_dir (false);

      switch (type)
        {
        case DirectoryManager::DirectoryType::USER_TOP:
          return user_dir;
        case DirectoryManager::DirectoryType::USER_PROJECTS:
          return user_dir / ZRYTHM_PROJECTS_DIR;
        case DirectoryManager::DirectoryType::USER_TEMPLATES:
          return user_dir / "templates";
        case DirectoryManager::DirectoryType::USER_LOG:
          return user_dir / "log";
        case DirectoryManager::DirectoryType::USER_SCRIPTS:
          return user_dir / "scripts";
        case DirectoryManager::DirectoryType::USER_THEMES:
          return user_dir / "themes";
        case DirectoryManager::DirectoryType::USER_THEMES_CSS:
          return user_dir / "themes" / "css";
        case DirectoryManager::DirectoryType::USER_THEMES_ICONS:
          return user_dir / "themes" / "icons";
        case DirectoryManager::DirectoryType::USER_PROFILING:
          return user_dir / "profiling";
        case DirectoryManager::DirectoryType::USER_GDB:
          return user_dir / "gdb";
        case DirectoryManager::DirectoryType::USER_BACKTRACE:
          return user_dir / "backtraces";
        default:
          break;
        }
    }

  z_return_val_if_reached ("");
}

DirectoryManager::~DirectoryManager ()
{
  z_info ("Destroying DirectoryManager");
  remove_testing_dir ();
  clearSingletonInstance ();
}