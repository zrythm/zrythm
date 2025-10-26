// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>

#include "gui/backend/backend/settings/plugin_configuration_manager.h"
#include "gui/backend/plugin_collections.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_descriptor_list.h"
#include "plugins/plugin_scan_manager.h"

namespace zrythm::gui::old_dsp::plugins
{

/**
 * The PluginManager is responsible for scanning and keeping track of available
 * Plugin's.
 */
class PluginManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::plugins::discovery::PluginDescriptorList * pluginDescriptors READ
      getPluginDescriptors CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::plugins::PluginScanManager * scanner READ getScanner CONSTANT FINAL)
  Q_PROPERTY (
    QString currentlyScanningPlugin READ getCurrentlyScanningPlugin NOTIFY
      currentlyScanningPluginChanged FINAL)
public:
  using Protocol = zrythm::plugins::Protocol;
  using PluginConfiguration = zrythm::plugins::PluginConfiguration;

  PluginManager (QObject * parent = nullptr);

  QString getCurrentlyScanningPlugin () const
  {
    return scanner_->getCurrentlyScanningPlugin ();
  }

  ::zrythm::plugins::PluginScanManager * getScanner () const
  {
    return scanner_.get ();
  }

  zrythm::plugins::discovery::PluginDescriptorList *
  getPluginDescriptors () const
  {
    return plugin_descriptors_.get ();
  }

  Q_INVOKABLE void beginScan ();

  Q_INVOKABLE void
  createPluginInstance (const zrythm::plugins::PluginDescriptor * descr);

  auto &get_format_manager () const { return format_manager_; }

  /**
   * Adds a new descriptor.
   */
  void add_descriptor (const zrythm::plugins::PluginDescriptor &descr);

  static PluginManager * get_active_instance ();

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
   * Returns if the plugin manager supports the given plugin protocol.
   */
  static bool
  supports_protocol (zrythm::plugins::Protocol::ProtocolType protocol);

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

  static fs::path get_known_plugins_xml_path ();
  void            serialize_known_plugins ();
  void            deserialize_known_plugins ();

private:
  /** Plugin categories. */
  std::vector<utils::Utf8String> plugin_categories_;

  /** Plugin authors. */
  std::vector<utils::Utf8String> plugin_authors_;

  std::shared_ptr<juce::AudioPluginFormatManager> format_manager_;

  /** Current known plugin list. */
  std::shared_ptr<juce::KnownPluginList> known_plugin_list_;

  /**
   * Wrapper over known_plugin_list_ that provides QML list functionality.
   */
  std::unique_ptr<zrythm::plugins::discovery::PluginDescriptorList>
    plugin_descriptors_;

  /** Plugin collections. */
  std::unique_ptr<PluginCollections> collections_;

  std::unique_ptr<::zrythm::plugins::PluginScanManager> scanner_;

  /** Whether the plugin manager has been set up already. */
  bool setup_ = false;

  /** Number of newly scanned (newly cached) plugins. */
  int num_new_plugins_ = 0;
};

} // namespace zrythm::gui::old_dsp::plugins
