// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SETTINGS_PLUGIN_SETTINGS_H__
#define __SETTINGS_PLUGIN_SETTINGS_H__

#include "zrythm-config.h"

#include <memory>

#include "dsp/plugin_slot.h"
#include "dsp/port_identifier.h"
#include "gui/dsp/plugin_descriptor.h"

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

/**
 * A setting for a specific plugin descriptor.
 */
class PluginSetting final
    : public zrythm::utils::serialization::ISerializable<PluginSetting>,
      public ICloneable<PluginSetting>
{
public:
  PluginSetting () = default;

  /**
   * Creates a plugin setting with the recommended settings for the given plugin
   * descriptor based on the current setup.
   */
  PluginSetting (const zrythm::gui::old_dsp::plugins::PluginDescriptor &descr);

  enum class HostingType
  {
    /**
     * @brief Plugin hosted via Carla Host.
     */
    Carla,

    // below unimplemented

    /**
     * @brief Plugin hosted via JUCE.
     */
    JUCE,

    /**
     * @brief Plugin hosted via custom CLAP implementation.
     */
    CLAP
  };

public:
  void validate () { validate (false); }

  std::string get_document_type () const override { return "PluginSetting"; }
  int         get_format_major_version () const override { return 1; }
  int         get_format_minor_version () const override { return 0; }

  /**
   * Makes sure the setting is valid in the current run and changes any fields
   * to make it conform.
   *
   * For example, if the setting is set to not open with carla but the
   * descriptor is for a VST plugin, this will set \ref
   * PluginSetting.open_with_carla to true.
   */
  void validate (bool print_result);

  void print () const;

  DECLARE_DEFINE_FIELDS_METHOD ();

  /**
   * Creates necessary tracks at the end of the tracklist.
   *
   * This may happen asynchronously so the caller should not expect the
   * setting to be activated on return.
   */
  void activate () const;

  /**
   * @note This also serializes all plugin settings.
   */
  void increment_num_instantiations ();

  void
  init_after_cloning (const PluginSetting &other, ObjectCloneType clone_type)
    override;

#if 0
  static void free_closure (void * self, GClosure * closure)
  {
    delete static_cast<PluginSetting *> (self);
  }
#endif

  zrythm::gui::old_dsp::plugins::PluginDescriptor * get_descriptor () const
  {
    return descr_.get ();
  }

  auto get_name () const { return get_descriptor ()->name_; }

  /**
   * Finishes activating a plugin setting.
   */
  void activate_finish (bool autoroute_multiout, bool has_stereo_outputs) const;

private:
  void copy_fields_from (const PluginSetting &other);

public:
  /** The descriptor of the plugin this setting is for. */
  std::unique_ptr<zrythm::gui::old_dsp::plugins::PluginDescriptor> descr_;

  HostingType hosting_type_ = HostingType::Carla;

  /** Whether to instantiate this plugin with carla. */
  bool open_with_carla_ = false;

  /** Whether to force a generic UI. */
  bool force_generic_ui_ = false;

  /** Requested carla bridge mode. */
  zrythm::gui::old_dsp::plugins::CarlaBridgeMode bridge_mode_ =
    (zrythm::gui::old_dsp::plugins::CarlaBridgeMode) 0;

  /** Last datetime instantiated (number of microseconds since January 1, 1970
   * UTC). */
  std::int64_t last_instantiated_time_ = 0;

  /** Number of times this plugin has been instantiated. */
  int num_instantiations_ = 0;
};

class PluginSettings
  final : public zrythm::utils::serialization::ISerializable<PluginSettings>
{
public:
  /**
   * Reads the file and fills up the object, or creates an empty object if the
   * file could not be read.
   */
  static std::unique_ptr<PluginSettings> read_or_new ();

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

  DECLARE_DEFINE_FIELDS_METHOD ();

  int         get_format_major_version () const override { return 7; }
  int         get_format_minor_version () const override { return 0; }
  std::string get_document_type () const override { return "PluginSettings"; }

  /**
   * Finds a setting for the given plugin descriptor.
   *
   * @return The found setting or NULL.
   */
  PluginSetting *
  find (const zrythm::gui::old_dsp::plugins::PluginDescriptor &descr);

  /**
   * Replaces a setting or appends a setting to the cache.
   *
   * This clones the setting before adding it.
   *
   * @param serialize Whether to serialize the updated cache now.
   */
  void set (const PluginSetting &setting, bool serialize);

private:
  static fs::path get_file_path ();

  /**
   * @brief Deletes the cache file.
   *
   * @throw ZrythmException if an error occurred while deleting the file.
   */
  static void delete_file ();

public:
  /** Settings. */
  std::vector<std::unique_ptr<PluginSetting>> settings_;
};

/**
 * @}
 */

#endif
