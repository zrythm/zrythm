// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __PLUGINS_PLUGIN_COLLECTIONS_H__
#define __PLUGINS_PLUGIN_COLLECTIONS_H__

#include <vector>

#include "plugins/plugin_descriptor.h"

#include <giomm.h>
#include <glibmm.h>

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * Plugin collection used in the plugin browser.
 */
class PluginCollection final : public ISerializable<PluginCollection>
{
public:
  std::string        get_name () const { return name_; }
  static std::string name_getter (void * data)
  {
    return static_cast<PluginCollection *> (data)->get_name ();
  }
  void        set_name (std::string_view name) { name_ = name; }
  static void name_setter (void * data, const std::string &name)
  {
    static_cast<PluginCollection *> (data)->set_name (name);
  }

  void clear () { descriptors_.clear (); }

  DECLARE_DEFINE_FIELDS_METHOD ();

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
  Glib::RefPtr<Gio::MenuModel> generate_context_menu () const;

public:
  /** Name of the collection. */
  std::string name_;

  /** Description of the collection (optional). */
  std::string description_;

  /** Plugin descriptors. */
  std::vector<PluginDescriptor> descriptors_;
};

/**
 * Serializable plugin collections.
 */
class PluginCollections : public ISerializable<PluginCollections>
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

  DECLARE_DEFINE_FIELDS_METHOD ();

std::string get_document_type () const override
{
  return "Zrythm Plugin Collections";
}
int get_format_major_version () const override { return 3; }
int get_format_minor_version () const override { return 0; }

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
  /**
   * @brief Deletes the cache file.
   *
   * @throw ZrythmException if an error occurred while deleting the file.
   */
  static void delete_file ();

  static fs::path get_file_path ();

public:
  /** Plugin collections. */
  std::vector<PluginCollection> collections_;
};

/**
 * @}
 */

#endif
