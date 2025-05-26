// SPDX-FileCopyrightText: © 2020-2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "plugins/plugin_descriptor.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

namespace zrythm::gui::old_dsp::plugins
{

/**
 * Plugin collection used in the plugin browser.
 */
class PluginCollection final : public ICloneable<PluginCollection>
{
public:
  using PluginDescriptor = zrythm::plugins::PluginDescriptor;

  std::string get_name () const { return name_; }

  void set_name (std::string_view name) { name_ = name; }

  void clear () { descriptors_.clear (); }

  /**
   * Returns whether the collection contains the given descriptor.
   */
  bool contains_descriptor (const PluginDescriptor &descr) const;

  /**
   * Appends a descriptor to the collection.
   */
  void add_descriptor (const PluginDescriptor &descr);

  /**
   * Removes the descriptor matching the given one from the collection.
   */
  void remove_descriptor (const PluginDescriptor &descr);

  /**
   * Generates a context menu for the collection.
   */
  // Glib::RefPtr<Gio::MenuModel> generate_context_menu () const;

  void
  init_after_cloning (const PluginCollection &other, ObjectCloneType clone_type)
    override;

private:
  static constexpr auto kNameKey = "name"sv;
  static constexpr auto kDescriptionKey = "description"sv;
  static constexpr auto kDescriptorsKey = "descriptors"sv;
  friend void           to_json (nlohmann::json &j, const PluginCollection &p)
  {
    j = nlohmann::json{
      { kNameKey,        p.name_        },
      { kDescriptionKey, p.description_ },
      { kDescriptorsKey, p.descriptors_ },
    };
  }
  friend void from_json (const nlohmann::json &j, PluginCollection &p)
  {
    j.at (kNameKey).get_to (p.name_);
    j.at (kDescriptionKey).get_to (p.description_);
    j.at (kDescriptorsKey).get_to (p.descriptors_);
  }

public:
  /** Name of the collection. */
  std::string name_;

  /** Description of the collection (optional). */
  std::string description_;

  /** Plugin descriptors. */
  std::vector<std::unique_ptr<PluginDescriptor>> descriptors_;
};

/**
 * Serializable plugin collections.
 */
class PluginCollections
{
public:
  /**
   * @brief Returns a new instance with the cache pre-filled from the cache
   * file, or an empty instance if there is no file or an error occurred.
   *
   * @return std::unique_ptr<PluginCollections>
   */
  static std::unique_ptr<PluginCollections> read_or_new ();

  /**
   * @brief Serializes the cached descriptors to the standard file.
   *
   * @throw ZrythmException if an error occurred while serializing.
   */
  void serialize_to_file () const;

  /**
   * @brief Wrapper over @ref serialize_to_file that ignores exceptions.
   *
   */
  void serialize_to_file_no_throw () const
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

  std::string get_document_type () const { return "Zrythm Plugin Collections"; }
  int         get_format_major_version () const { return 3; }
  int         get_format_minor_version () const { return 0; }

  /**
   * Appends a collection.
   *
   * @param serialize Whether to serialize the updated
   *   cache now.
   */
  void add (const PluginCollection &collection, bool serialize);

  /**
   * Finds a collection by name.
   */
  const PluginCollection * find_from_name (std::string_view name) const;

  /**
   * Removes the given collection.
   *
   * @param serialize Whether to serialize the updated
   *   cache now.
   */
  void remove (PluginCollection &collection, bool serialize);

private:
  static constexpr auto kCollectionsKey = "collections"sv;
  friend void           to_json (nlohmann::json &j, const PluginCollections &pc)
  {
    j[kCollectionsKey] = pc.collections_;
  }
  friend void from_json (const nlohmann::json &j, PluginCollections &pc)
  {
    j.at (kCollectionsKey).get_to (pc.collections_);
  }

  /**
   * @brief Deletes the cache file.
   *
   * @throw ZrythmException if an error occurred while deleting the file.
   */
  static void delete_file ();

  static fs::path get_file_path ();

public:
  /** Plugin collections. */
  std::vector<std::unique_ptr<PluginCollection>> collections_;
};

} // namespace zrythm::gui::old_dsp::plugins

/**
 * @}
 */
