// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include <memory>

#include "plugins/plugin_configuration.h"

#include <boost/describe.hpp>

namespace zrythm::gui::old_dsp::plugins
{
class Plugin;
}

using namespace zrythm;

/**
 * @addtogroup settings
 *
 * @{
 */

class PluginConfigurationManager final
{
public:
  using PluginConfiguration = zrythm::plugins::PluginConfiguration;

  /**
   * @brief Creates a configuration for the given descriptor based on an
   * existing configuration if it exists, or otherwise creates a default
   * configuration for the descriptor.
   *
   * @param descr
   * @return std::unique_ptr<PluginConfiguration>
   */
  std::unique_ptr<PluginConfiguration>
  create_configuration_for_descriptor (const plugins::PluginDescriptor &descr);

  /**
   * Reads the file and fills up the object, or creates an empty object if the
   * file could not be read.
   */
  static std::unique_ptr<PluginConfigurationManager> read_or_new ();

  /**
   * Serializes the current settings.
   *
   * @throw ZrythmException on error.
   */
  void serialize_to_file ();

  /**
   * @brief Wrapper over @ref serialize_to_file that ignores exceptions.
   *
   */
  void serialize_to_file_no_throw ()
  {
    try
      {
        serialize_to_file ();
      }
    catch (const ZrythmException &e)
      {
        z_warning (e.what ());
      }
  }

  int         get_format_major_version () const { return 7; }
  int         get_format_minor_version () const { return 0; }
  std::string get_document_type () const
  {
    return "PluginConfigurationCollection";
  }

  /**
   * Finds a setting for the given plugin descriptor.
   *
   * @return The found setting or NULL.
   */
  PluginConfiguration * find (const zrythm::plugins::PluginDescriptor &descr);

  /**
   * Replaces a setting or appends a setting to the cache.
   *
   * This clones the setting before adding it.
   *
   * @param serialize Whether to serialize the updated cache now.
   */
  void set (const PluginConfiguration &setting, bool serialize);

  /**
   * @note This also serializes all plugin settings.
   */
  void increment_num_instantiations_for_plugin (
    const plugins::PluginDescriptor &descr);

  /**
   * @brief Creates necessary tracks at the end of the tracklist.
   *
   * @param config
   * @param autoroute_multiout
   * @param has_stereo_outputs
   *
   * This creates and performs undoable actions.
   *
   * TODO: maybe move this to a PluginInstantiator/PluginFactory class?
   */
  void activate_plugin_configuration (
    const PluginConfiguration &config,
    bool                       autoroute_multiout,
    bool                       has_stereo_outputs);

  /**
   * @brief Asks the user a few things via dialogs then calls
   * activate_plugin_configuration().
   */
  void activate_plugin_configuration_async (const PluginConfiguration &config);

private:
  static constexpr auto kSettingsKey = "settings"sv;
  friend void to_json (nlohmann::json &j, const PluginConfigurationManager &p)
  {
    j[kSettingsKey] = p.settings_;
  }
  friend void from_json (const nlohmann::json &j, PluginConfigurationManager &p)
  {
    j.at (kSettingsKey).get_to (p.settings_);
  }

  static fs::path get_file_path ();

  /**
   * @brief Deletes the cache file.
   *
   * @throw ZrythmException if an error occurred while deleting the file.
   */
  static void delete_file ();

private:
  /** Settings. */
  std::vector<std::unique_ptr<PluginConfiguration>> settings_;

  /** Number of times the plugin has been instantiated. */
  using NumInstantiations = int;

  /** Last datetime instantiated (number of microseconds since January 1, 1970
   * UTC). */
  using LastInstantiatedTimeAsMSecsSinceEpoch = std::int64_t;

  /**
   * @brief Number of instantiations and last instantiated time for each
   * descriptor.
   */
  std::unordered_map<
    plugins::PluginDescriptor,
    std::pair<NumInstantiations, LastInstantiatedTimeAsMSecsSinceEpoch>>
    instantiations_;
};

/**
 * @}
 */
