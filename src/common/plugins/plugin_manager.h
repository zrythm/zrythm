// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Plugin manager.
 */

#ifndef __PLUGINS_PLUGIN_MANAGER_H__
#define __PLUGINS_PLUGIN_MANAGER_H__

#include <filesystem>

#include "common/plugins/cached_plugin_descriptors.h"
#include "common/plugins/carla_discovery.h"
#include "common/plugins/collections.h"
#include "common/plugins/plugin_descriptor.h"
#include "common/utils/types.h"

class ZCarlaDiscovery;
class StringArray;

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_MANAGER (gZrythm->plugin_manager_)

/**
 * The PluginManager is responsible for scanning and keeping track of available
 * Plugin's.
 */
class PluginManager
{
public:
  PluginManager ()
      : cached_plugin_descriptors_ (CachedPluginDescriptors::read_or_new ()),
        collections_ (PluginCollections::read_or_new ()),
        carla_discovery_ (std::make_unique<ZCarlaDiscovery> (*this))
  {
  }

  static std::vector<fs::path>
  get_paths_for_protocol (const PluginProtocol protocol);

  static std::string
  get_paths_for_protocol_separated (const PluginProtocol protocol);

  /**
   * Searches in the known paths for this plugin protocol for the given relative
   * path of the plugin and returns the absolute path.
   */
  fs::path find_plugin_from_rel_path (
    const PluginProtocol   protocol,
    const std::string_view rel_path) const;

  void
  begin_scan (const double max_progress, double * progress, GenericCallback cb);

  /**
   * Adds a new descriptor.
   */
  void add_descriptor (const PluginDescriptor &descr);

  /**
   * Updates the text in the greeter.
   */
  void set_currently_scanning_plugin (const char * filename, const char * sha1);

  /**
   * Returns a PluginDescriptor for the given URI.
   */
  std::unique_ptr<PluginDescriptor>
  find_plugin_from_uri (std::string_view uri) const;

  /**
   * Finds and returns a PluginDescriptor matching the given descriptor.
   */
  std::unique_ptr<PluginDescriptor>
  find_from_descriptor (const PluginDescriptor &src_descr) const;

  /**
   * Returns if the plugin manager supports the given plugin protocol.
   */
  static bool supports_protocol (PluginProtocol protocol);

  /**
   * Returns an instrument plugin, if any.
   */
  std::unique_ptr<PluginDescriptor> pick_instrument () const;

  void clear_plugins ();

private:
  /**
   * @brief Adds the given category and author to the lists of plugin categories
   * and authors, if not already present.
   */
  void
  add_category_and_author (std::string_view category, std::string_view author);

  /**
   * @brief Source func.
   */
  bool call_carla_discovery_idle ();

  static StringArray get_lv2_paths ();
  static StringArray get_vst2_paths ();
  static StringArray get_vst3_paths ();
  static StringArray get_sf_paths (bool sf2);
  static StringArray get_dssi_paths ();
  static StringArray get_ladspa_paths ();
  static StringArray get_clap_paths ();
  static StringArray get_jsfx_paths ();
  static StringArray get_au_paths ();

public:
  /**
   * Scanned plugin descriptors.
   */
  std::vector<PluginDescriptor> plugin_descriptors_;

  /** Plugin categories. */
  std::vector<std::string> plugin_categories_;

  /** Plugin authors. */
  std::vector<std::string> plugin_authors_;

  /** Cached descriptors */
  std::unique_ptr<CachedPluginDescriptors> cached_plugin_descriptors_;

  /** Plugin collections. */
  std::unique_ptr<PluginCollections> collections_;

  std::unique_ptr<ZCarlaDiscovery> carla_discovery_;

  GenericCallback scan_done_cb_;

  /** Whether the plugin manager has been set up already. */
  bool setup_ = false;

  /** Number of newly scanned (newly cached) plugins. */
  int num_new_plugins_ = 0;
};

/**
 * @}
 */

#endif
