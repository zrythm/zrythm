// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/plugins/plugin_protocol_paths.h"
#include "common/utils/directory_manager.h"
#include "gui/backend/backend/settings_manager.h"

using namespace zrythm::plugins;
using namespace zrythm;

static void
add_expanded_paths (auto &arr, const QStringList &paths_from_settings)
{
  for (const auto &path : paths_from_settings)
    {
      auto expanded_cur_path = string_expand_env_vars (path.toStdString ());
      /* split because the env might contain multiple paths */
      auto expanded_paths = Glib::Regex::split_simple (
        G_SEARCHPATH_SEPARATOR_S, expanded_cur_path.c_str ());
      for (auto &expanded_path : expanded_paths)
        {
          arr->add_path (expanded_path.c_str ());
        }
    }
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_for_protocol (const Protocol::ProtocolType protocol)
{
  switch (protocol)
    {
    case Protocol::ProtocolType::VST:
      return get_vst2_paths ();
    case Protocol::ProtocolType::VST3:
      return get_vst3_paths ();
    case Protocol::ProtocolType::DSSI:
      return get_dssi_paths ();
    case Protocol::ProtocolType::LADSPA:
      return get_ladspa_paths ();
    case Protocol::ProtocolType::SFZ:
      return get_sf_paths (false);
    case Protocol::ProtocolType::SF2:
      return get_sf_paths (true);
    case Protocol::ProtocolType::CLAP:
      return get_clap_paths ();
    case Protocol::ProtocolType::JSFX:
      return get_jsfx_paths ();
    case Protocol::ProtocolType::LV2:
      return get_lv2_paths ();
    case Protocol::ProtocolType::AudioUnit:
      return get_au_paths ();
    default:
      z_return_val_if_reached (
        {}); // Return empty container for unknown protocol types
    }
}

std::string
PluginProtocolPaths::get_for_protocol_separated (
  const Protocol::ProtocolType protocol)
{
  auto paths = get_for_protocol (protocol);
  if (!paths->empty ())
    {
      std::vector<std::string> path_strings;
      std::ranges::transform (
        paths->getPaths (), std::back_inserter (path_strings),
        [] (const auto &path) { return path.toStdString (); });
      auto paths_separated =
        string_join (path_strings, G_SEARCHPATH_SEPARATOR_S);
      return paths_separated;
    }

  return "";
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_lv2_paths ()
{
  auto ret = std::make_unique<utils::FilePathList> ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      /* add test plugins if testing */
      auto tests_builddir = Glib::getenv ("G_TEST_BUILDDIR");
      auto root_builddir = Glib::getenv ("G_TEST_BUILD_ROOT_DIR");
      z_return_val_if_fail (!tests_builddir.empty (), nullptr);
      z_return_val_if_fail (!root_builddir.empty (), nullptr);

      auto test_lv2_plugins = fs::path (tests_builddir) / "lv2plugins";
      auto test_root_plugins = fs::path (root_builddir) / "data" / "plugins";
      ret->add_path (test_lv2_plugins.string ());
      ret->add_path (test_root_plugins.string ());

      QStringList paths_from_settings = { "${LV2_PATH}", "/usr/lib/lv2" };
      add_expanded_paths (ret, paths_from_settings);

      ret->print ("LV2 paths");

      return ret;
    }

  auto paths_from_settings =
    zrythm::gui::SettingsManager::get_instance ()->get_lv2_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#ifdef _WIN32
      ret->add_path ("C:\\Program Files\\Common Files\\LV2");
#elif defined(__APPLE__)
      ret->add_path ("/Library/Audio/Plug-ins/LV2");
#elif defined(FLATPAK_BUILD)
      ret->add_path ("/app/lib/lv2");
      ret->add_path ("/app/extensions/Plugins/lv2");
#else /* non-flatpak UNIX */
      {
        auto home_lv2 = fs::path (Glib::get_home_dir ()) / ".lv2";
        ret->add_path (home_lv2);
      }
      ret->add_path ("/usr/lib/lv2");
      ret->add_path ("/usr/local/lib/lv2");
#  if ZRYTHM_IS_INSTALLER_VER
      ret->add_path ("/usr/lib64/lv2");
      ret->add_path ("/usr/local/lib64/lv2");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret->add_path ("/usr/" LIBDIR_NAME "/lv2");
          ret->add_path ("/usr/local/" LIBDIR_NAME "/lv2");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  /* add special paths */
  auto * dir_mgr = DirectoryManager::getInstance ();
  auto   builtin_plugins_path = dir_mgr->get_dir (
    DirectoryManager::DirectoryType::SYSTEM_BUNDLED_PLUGINSDIR);
  auto special_plugins_path = dir_mgr->get_dir (
    DirectoryManager::DirectoryType::SYSTEM_SPECIAL_LV2_PLUGINS_DIR);
  ret->add_path (builtin_plugins_path);
  ret->add_path (special_plugins_path);

  ret->print ("LV2 paths");

  return ret;
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_vst2_paths ()
{
  auto ret = std::make_unique<utils::FilePathList> ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${VST_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret->print ("VST2 paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_vst2_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#ifdef _WIN32
      ret->add_path ("C:\\Program Files\\Common Files\\VST2");
      ret->add_path ("C:\\Program Files\\VSTPlugins");
      ret->add_path ("C:\\Program Files\\Steinberg\\VSTPlugins");
      ret->add_path ("C:\\Program Files\\Common Files\\VST2");
      ret->add_path ("C:\\Program Files\\Common Files\\Steinberg\\VST2");
#elif defined(__APPLE__)
      ret->add_path ("/Library/Audio/Plug-ins/VST");
#elif defined(FLATPAK_BUILD)
      ret->add_path ("/app/extensions/Plugins/vst");
#else /* non-flatpak UNIX */
      {
        auto home_vst = fs::path (Glib::get_home_dir ()) / ".vst";
        ret->add_path (home_vst);
      }
      ret->add_path ("/usr/lib/vst");
      ret->add_path ("/usr/local/lib/vst");
#  if ZRYTHM_IS_INSTALLER_VER
      ret->add_path ("/usr/lib64/vst");
      ret->add_path ("/usr/local/lib64/vst");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret->add_path ("/usr/" LIBDIR_NAME "/vst");
          ret->add_path ("/usr/local/" LIBDIR_NAME "/vst");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret->print ("VST2 paths");

  return ret;
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_vst3_paths ()
{
  auto ret = std::make_unique<utils::FilePathList> ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${VST3_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret->print ("VST3 paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_vst3_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#ifdef _WIN32
      ret->add_path ("C:\\Program Files\\Common Files\\VST3");
#elif defined(__APPLE__)
      ret->add_path ("/Library/Audio/Plug-ins/VST3");
#elif defined(FLATPAK_BUILD)
      ret->add_path ("/app/extensions/Plugins/vst3");
#else /* non-flatpak UNIX */
      {
        auto home_vst3 = fs::path (Glib::get_home_dir ()) / ".vst3";
        ret->add_path (home_vst3);
      }
      ret->add_path ("/usr/lib/vst3");
      ret->add_path ("/usr/local/lib/vst3");
#  if ZRYTHM_IS_INSTALLER_VER
      ret->add_path ("/usr/lib64/vst3");
      ret->add_path ("/usr/local/lib64/vst3");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret->add_path ("/usr/" LIBDIR_NAME "/vst3");
          ret->add_path ("/usr/local/" LIBDIR_NAME "/vst3");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret->print ("VST3 paths");

  return ret;
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_sf_paths (bool sf2)
{
  auto ret = std::make_unique<utils::FilePathList> ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      ret->add_path (G_SEARCHPATH_SEPARATOR_S);
      return ret;
    }

  auto paths_from_settings =
    sf2 ? gui::SettingsManager::get_instance ()->get_sf2_search_paths ()
        : gui::SettingsManager::get_instance ()->get_sfz_search_paths ();
  add_expanded_paths (ret, paths_from_settings);

  return ret;
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_dssi_paths ()
{
  auto ret = std::make_unique<utils::FilePathList> ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${DSSI_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret->print ("DSSI paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_dssi_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#if defined(FLATPAK_BUILD)
      ret.add ("/app/extensions/Plugins/dssi");
#else /* non-flatpak UNIX */
      {
        auto home_dssi = fs::path (Glib::get_home_dir ()) / ".dssi";
        ret->add_path (home_dssi.string ());
      }
      ret->add_path ("/usr/lib/dssi");
      ret->add_path ("/usr/local/lib/dssi");
#  if ZRYTHM_IS_INSTALLER_VER
      ret->add_path ("/usr/lib64/dssi");
      ret->add_path ("/usr/local/lib64/dssi");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret->add_path ("/usr/" LIBDIR_NAME "/dssi");
          ret->add_path ("/usr/local/" LIBDIR_NAME "/dssi");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret->print ("DSSI paths");

  return ret;
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_ladspa_paths ()
{
  auto ret = std::make_unique<utils::FilePathList> ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${LADSPA_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret->print ("LADSPA paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_ladspa_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#if defined(FLATPAK_BUILD)
      ret->add_path ("/app/extensions/Plugins/ladspa");
#else /* non-flatpak UNIX */
      ret->add_path ("/usr/lib/ladspa");
      ret->add_path ("/usr/local/lib/ladspa");
#  if ZRYTHM_IS_INSTALLER_VER
      ret->add_path ("/usr/lib64/ladspa");
      ret->add_path ("/usr/local/lib64/ladspa");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret->add_path ("/usr/" LIBDIR_NAME "/ladspa");
          ret->add_path ("/usr/local/" LIBDIR_NAME "/ladspa");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret->print ("LADSPA paths");

  return ret;
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_clap_paths ()
{
  auto ret = std::make_unique<utils::FilePathList> ();

#ifndef CARLA_HAVE_CLAP_SUPPORT
  return ret;
#endif

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${CLAP_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret->print ("CLAP paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_clap_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#ifdef _WIN32
      ret.add ("C:\\Program Files\\Common Files\\CLAP");
      ret.add ("C:\\Program Files (x86)\\Common Files\\CLAP");
#elif defined(__APPLE__)
      ret.add ("/Library/Audio/Plug-ins/CLAP");
#elif defined(FLATPAK_BUILD)
      ret.add ("/app/extensions/Plugins/clap");
#else /* non-flatpak UNIX */
      {
        auto home_clap = fs::path (Glib::get_home_dir ()) / ".clap";
        ret->add_path (home_clap);
      }
      ret->add_path ("/usr/lib/clap");
      ret->add_path ("/usr/local/lib/clap");
#  if ZRYTHM_IS_INSTALLER_VER
      ret->add_path ("/usr/lib64/clap");
      ret->add_path ("/usr/local/lib64/clap");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret->add_path ("/usr/" LIBDIR_NAME "/clap");
          ret->add_path ("/usr/local/" LIBDIR_NAME "/clap");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret->print ("CLAP paths");

  return ret;
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_jsfx_paths ()
{
  auto ret = std::make_unique<utils::FilePathList> ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${JSFX_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret->print ("JSFX paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_jsfx_search_paths ();
  if (!paths_from_settings.empty ())
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret->print ("JSFX paths");

  return ret;
}

std::unique_ptr<utils::FilePathList>
PluginProtocolPaths::get_au_paths ()
{
  auto ret = std::make_unique<utils::FilePathList> ();

  ret->add_path ("/Library/Audio/Plug-ins/Components");
  auto user_components =
    fs::path (Glib::get_home_dir ()) / "Library" / "Audio" / "Plug-ins"
    / "Components";
  ret->add_path (user_components.string ());

  ret->print ("AU paths");

  return ret;
}