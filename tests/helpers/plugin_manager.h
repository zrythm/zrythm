// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Plugin manager helper.
 */

#ifndef __TEST_HELPERS_PLUGIN_MANAGER_H__
#define __TEST_HELPERS_PLUGIN_MANAGER_H__

#include "zrythm-test-config.h"

#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"

#include "tests/helpers/zrythm_helper.h"

#include "common/dsp/tracklist.h"
#include "common/plugins/plugin_manager.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/io.h"

/**
 * @addtogroup tests
 *
 * @{
 */

/**
 * Get a plugin setting clone from the given URI in the given bundle.
 */
PluginSetting
test_plugin_manager_get_plugin_setting (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla);

/**
 * Creates @ref num_tracks tracks for the given
 * plugin.
 *
 * @param num_tracks Number of tracks to create.
 *
 * @return The index of the last track created.
 */
int
test_plugin_manager_create_tracks_from_plugin (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla,
  int          num_tracks = 1);

/**
 * Get a plugin setting clone from the given URI in the given
 * bundle.
 *
 * If non-LV2, the path should be passed to pl_bundle.
 */
PluginSetting
test_plugin_manager_get_plugin_setting (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla)
{
  {
    auto   basename = Glib::path_get_basename (pl_bundle);
    char * tmpdir = g_dir_make_tmp ("zrythm_vst_XXXXXX", nullptr);
    auto   dest_path = Glib::build_filename (tmpdir, basename);
    if (g_str_has_suffix (pl_bundle, "vst3"))
      {
        EXPECT_NO_THROW (io_copy_dir (dest_path, pl_bundle, true, true));
      }
    else if (pl_uri)
      {
        EXPECT_NO_THROW (io_copy_dir (dest_path, &pl_bundle[7], true, true));
      }
    else
      {
        GFile * pl_bundle_file = NULL;
        pl_bundle_file = g_file_new_for_path (pl_bundle);
        GFile * pl_bundle_file_in_tmp = g_file_new_for_path (dest_path.c_str ());
        GError * err = NULL;
        EXPECT_TRUE (g_file_copy (
          pl_bundle_file, pl_bundle_file_in_tmp, G_FILE_COPY_NONE, nullptr,
          nullptr, nullptr, &err));
        // g_object_unref (pl_bundle_file);
        // g_object_unref (pl_bundle_file_in_tmp);
      }
    g_setenv ("VST3_PATH", tmpdir, true);
    g_setenv ("LV2_PATH", tmpdir, true);
    g_setenv ("VST_PATH", tmpdir, true);
    g_free (tmpdir);
  }

  bool scan_finished = false;
  PLUGIN_MANAGER->clear_plugins ();
  PLUGIN_MANAGER->begin_scan (1.0, nullptr, [&scan_finished] () {
    scan_finished = true;
  });
  while (!scan_finished)
    {
      g_main_context_iteration (nullptr, true);
    }
  EXPECT_NONEMPTY (PLUGIN_MANAGER->plugin_descriptors_);

  std::optional<PluginDescriptor> descr;
  for (const auto &cur_descr : PLUGIN_MANAGER->plugin_descriptors_)
    {
      if (pl_uri)
        {
          if (cur_descr.uri_ == pl_uri)
            {
              descr = cur_descr;
            }
        }
      else if (cur_descr.protocol_ != PluginProtocol::LV2)
        {
          auto basename = Glib::path_get_basename (pl_bundle);
          auto descr_basename =
            Glib::path_get_basename (cur_descr.path_.string ());
          if (descr_basename == basename)
            {
              descr = cur_descr;
            }
        }
    }
  EXPECT_HAS_VALUE (descr);

  PluginSetting setting (descr.value ());

  /* always open with carla */
  setting.open_with_carla_ = true;
#if 0
  /* open with carla if requested */
  setting->open_with_carla_ = with_carla;
#endif

  setting.validate ();

  /* run the logger to avoid too many messages being queued */
  // TODO?
  // log_idle_cb (LOG);

  return setting;
}

/**
 * Creates @ref num_tracks tracks for the given
 * plugin.
 *
 * @param num_tracks Number of tracks to create.
 *
 * @return The index of the last track created.
 */
int
test_plugin_manager_create_tracks_from_plugin (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla,
  int          num_tracks)
{
  PluginSetting setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);

  Track::Type track_type = Track::Type::AudioBus;
  if (is_instrument)
    {
      /* fix the descriptor (for some reason lilv reports it as Plugin instead
       * of Instrument if you don't do lilv_world_load_all) */
      setting.descr_.category_ = ZPluginCategory::INSTRUMENT;
      setting.descr_.category_str_ =
        PluginDescriptor::category_to_string (setting.descr_.category_);
      track_type = Track::Type::Instrument;
    }

  /* create a track from the plugin */
  EXPECT_NO_THROW (Track::create_with_action (
    track_type, &setting, nullptr, nullptr, TRACKLIST->get_num_tracks (),
    num_tracks, -1, nullptr));

  return TRACKLIST->get_num_tracks () - 1;
}

/**
 * @}
 */

#endif
