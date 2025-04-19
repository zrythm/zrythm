// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_COMMON_PLUGINS_PLUGIN_MANAGER_H
#define ZRYTHM_COMMON_PLUGINS_PLUGIN_MANAGER_H

#include <filesystem>

#include "gui/backend/cached_plugin_descriptors.h"
#include "gui/backend/carla_discovery.h"
#include "gui/backend/plugin_collections.h"
#include "gui/backend/plugin_descriptor_list.h"
#include "gui/backend/plugin_scanner.h"
#include "gui/dsp/plugin_descriptor.h"
#include "utils/string_array.h"

namespace zrythm::gui::old_dsp::plugins
{

/**
 * The PluginManager is responsible for scanning and keeping track of available
 * Plugin's.
 */
class PluginManager final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    gui::PluginDescriptorList * pluginDescriptors READ getPluginDescriptors
      CONSTANT FINAL)
  Q_PROPERTY (plugins::PluginScanner * scanner READ getScanner CONSTANT FINAL)
  Q_PROPERTY (
    QString currentlyScanningPlugin READ getCurrentlyScanningPlugin NOTIFY
      currentlyScanningPluginChanged FINAL)
public:
  PluginManager (QObject * parent = nullptr);

  QString getCurrentlyScanningPlugin () const
  {
    return scanner_->getCurrentlyScanningPlugin ();
  }

  plugins::PluginScanner * getScanner () const { return scanner_.get (); }

  gui::PluginDescriptorList * getPluginDescriptors () const
  {
    return plugin_descriptors_.get ();
  }

  Q_INVOKABLE void beginScan ();

  /**
   * Adds a new descriptor.
   */
  void add_descriptor (const PluginDescriptor &descr);

  static PluginManager * get_active_instance ();

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
  static bool supports_protocol (Protocol::ProtocolType protocol);

  /**
   * Returns an instrument plugin, if any.
   */
  std::unique_ptr<PluginDescriptor> pick_instrument () const;

  void clear_plugins ();

  /**
   * @brief Returns the number of new plugins scanned this time (as opposed to
   * known ones).
   */
  int get_num_new_plugins () const { return num_new_plugins_; }

  Q_SIGNAL void scanFinished ();
  Q_SIGNAL void currentlyScanningPluginChanged (const QString &plugin);

  /**
   * @brief Called by the scanner when it has finished scanning.
   */
  Q_SLOT void onScanFinished ();

private:
  /**
   * @brief Adds the given category and author to the lists of plugin categories
   * and authors, if not already present.
   */
  void
  add_category_and_author (std::string_view category, std::string_view author);

  static fs::path get_known_plugins_xml_path ();
  void            serialize_known_plugins ();
  void            deserialize_known_plugins ();

public:
  /**
   * Scanned plugin descriptors.
   */
  std::unique_ptr<zrythm::gui::PluginDescriptorList> plugin_descriptors_;
  // std::vector<PluginDescriptor> plugin_descriptors_;

  /** Plugin categories. */
  std::vector<std::string> plugin_categories_;

  /** Plugin authors. */
  std::vector<std::string> plugin_authors_;

  /** Current known plugin list. */
  // std::unique_ptr<CachedPluginDescriptors> cached_plugin_descriptors_;
  std::shared_ptr<juce::KnownPluginList> known_plugin_list_;

  /** Plugin collections. */
  std::unique_ptr<PluginCollections> collections_;

  std::unique_ptr<PluginScanner> scanner_;

  /** Whether the plugin manager has been set up already. */
  bool setup_ = false;

  /** Number of newly scanned (newly cached) plugins. */
  int num_new_plugins_ = 0;
};

} // namespace zrythm::gui::old_dsp::plugins

#endif // ZRYTHM_COMMON_PLUGINS_PLUGIN_MANAGER_H
