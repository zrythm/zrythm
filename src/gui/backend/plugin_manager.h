// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>

#include "gui/backend/plugin_collections.h"
#include "plugins/plugin_catalog.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_scan_manager.h"
#include "utils/qt.h"

namespace zrythm::plugins
{
class Lv2World;
class PluginScanFormatManager;
}

namespace zrythm::gui::old_dsp::plugins
{

/**
 * @brief Returns whether a descriptor refers to a plugin file that no longer
 * exists.
 *
 * Only file-based formats whose identifier is an absolute path are considered;
 * internal and other non-file-based identifiers always return false.
 */
bool
known_plugin_file_missing (const zrythm::plugins::PluginDescriptor &descriptor);

/**
 * The PluginManager is responsible for scanning and keeping track of available
 * Plugin's.
 */
class PluginManager : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::plugins::discovery::PluginCatalog * pluginDescriptors READ
      getPluginDescriptors CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::plugins::PluginScanManager * scanner READ getScanner CONSTANT FINAL)
  Q_PROPERTY (
    QString currentlyScanningPlugin READ getCurrentlyScanningPlugin NOTIFY
      currentlyScanningPluginChanged FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using Protocol = zrythm::plugins::Protocol;
  using PluginConfiguration = zrythm::plugins::PluginConfiguration;

  PluginManager (
    zrythm::plugins::ProtocolPluginPathsProvider plugin_paths_provider,
    QObject *                                    parent = nullptr);

  QString getCurrentlyScanningPlugin () const
  {
    return scanner_->getCurrentlyScanningPlugin ();
  }

  ::zrythm::plugins::PluginScanManager * getScanner () const
  {
    return scanner_.get ();
  }

  zrythm::plugins::discovery::PluginCatalog * getPluginDescriptors () const
  {
    return catalog_.get ();
  }

  Q_INVOKABLE void beginScan ();

  std::shared_ptr<juce::AudioPluginFormatManager> get_format_manager () const
  {
    return scan_formats_->juce_format_manager ();
  }

  /**
   * @brief Returns the process-wide LV2 world owned by this manager,
   * shared by the scan format and instantiated LV2 plugins.
   */
  const std::shared_ptr<zrythm::plugins::Lv2World> &get_lv2_world () const
  {
    return lv2_world_;
  }

  /**
   * Returns a PluginDescriptor for the given URI.
   */
  std::unique_ptr<zrythm::plugins::PluginDescriptor>
  find_plugin_from_uri (const utils::Utf8String &uri) const;

  /**
   * Finds and returns a PluginDescriptor matching the given descriptor.
   */
  std::unique_ptr<zrythm::plugins::PluginDescriptor> find_from_descriptor (
    const zrythm::plugins::PluginDescriptor &src_descr) const;

  /**
   * Returns an instrument plugin, if any.
   */
  std::unique_ptr<zrythm::plugins::PluginDescriptor> pick_instrument () const;

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
  Q_SLOT void onScannerScanFinished ();

private:
  /**
   * @brief Adds the given category and author to the lists of plugin categories
   * and authors, if not already present.
   */
  void add_category_and_author (
    const utils::Utf8String &category,
    const utils::Utf8String &author);

  static std::filesystem::path get_known_plugins_json_path ();
  void                         serialize_known_plugins ();
  void                         deserialize_known_plugins ();

  /**
   * @brief Adds descriptors for bundled internal (Faust) plugins to the
   * catalog, if not already present.
   */
  void add_internal_plugins_to_catalog ();

private:
  /** Plugin categories. */
  std::vector<utils::Utf8String> plugin_categories_;

  /** Plugin authors. */
  std::vector<utils::Utf8String> plugin_authors_;

  std::shared_ptr<zrythm::plugins::PluginScanFormatManager> scan_formats_;

  std::shared_ptr<zrythm::plugins::Lv2World> lv2_world_;

  /** Catalog of scanned plugin descriptors exposed to QML. */
  std::shared_ptr<zrythm::plugins::discovery::PluginCatalog> catalog_;

  /** Plugin collections. */
  std::unique_ptr<PluginCollections> collections_;

  std::unique_ptr<::zrythm::plugins::PluginScanManager> scanner_;

  /** Whether the plugin manager has been set up already. */
  bool setup_ = false;

  /** Number of newly scanned (newly cached) plugins. */
  int num_new_plugins_ = 0;
};

} // namespace zrythm::gui::old_dsp::plugins
