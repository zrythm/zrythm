// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/backend/plugin_manager.h"
#include "gui/backend/plugin_protocol_paths.h"
#include "plugins/CLAPPluginFormat.h"
#include "plugins/faust/faust_registry.h"
#if ZRYTHM_WITH_LILV
#  include "plugins/lv2_plugin_format.h"
#endif
#include "plugins/out_of_process_scanner.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/io_utils.h"
#include "utils/logger.h"

#include <QtConcurrent>

using namespace Qt::StringLiterals;

using namespace zrythm::gui::old_dsp::plugins;

bool
zrythm::gui::old_dsp::plugins::known_plugin_file_missing (
  const juce::PluginDescription &desc)
{
  // internal (Faust) plugins are not file-based
  if (desc.pluginFormatName == "Internal")
    return false;

  // only prune absolute file paths (other formats use identifiers or URIs)
  if (!juce::File::isAbsolutePath (desc.fileOrIdentifier))
    return false;

  return !juce::File (desc.fileOrIdentifier).exists ();
}

PluginManager::PluginManager (
  zrythm::plugins::ProtocolPluginPathsProvider plugin_paths_provider,
  QObject *                                    parent)
    : QObject (parent),
      format_manager_ (std::make_shared<juce::AudioPluginFormatManager> ()),
      known_plugin_list_ (std::make_shared<juce::KnownPluginList> ()),
      plugin_descriptors_ (new zrythm::plugins::discovery::PluginDescriptorList (
        known_plugin_list_,
        this)),
      collections_ (PluginCollections::read_or_new ())
{
  juce::addDefaultFormatsToManager (*format_manager_);
  format_manager_->addFormat (
    std::make_unique<zrythm::plugins::CLAPPluginFormat> ());
  format_manager_->addFormat (
    std::make_unique<zrythm::plugins::Vst3PluginFormat> ());
#if ZRYTHM_WITH_LILV
  format_manager_->addFormat (
    std::make_unique<zrythm::plugins::Lv2PluginFormat> ());
#endif
  known_plugin_list_->setCustomScanner (
    std::make_unique<::zrythm::plugins::discovery::OutOfProcessPluginScanner> ());
  scanner_ = std::make_unique<zrythm::plugins::PluginScanManager> (
    known_plugin_list_, format_manager_, plugin_paths_provider);

  add_internal_plugins_to_known_list ();
}

void
PluginManager::add_internal_plugins_to_known_list ()
{
  for (const auto &info : zrythm::plugins::faust::available_faust_plugins ())
    {
      auto juce_desc =
        zrythm::plugins::faust::make_faust_plugin_descriptor (info)
          ->to_juce_description ();

      const auto &existing = known_plugin_list_->getTypes ();
      const auto  it = std::ranges::find_if (existing, [&] (const auto &pd) {
        return pd.pluginFormatName == juce_desc->pluginFormatName
               && pd.fileOrIdentifier == juce_desc->fileOrIdentifier;
      });
      if (it == existing.end ())
        {
          known_plugin_list_->addType (*juce_desc);
        }
    }
}

void
PluginManager::add_category_and_author (
  const utils::Utf8String &category,
  const utils::Utf8String &author)
{
  if (!category.is_ascii ())
    {
      z_warning ("Ignoring non-ASCII plugin category name...");
    }
  if (!std::ranges::contains (plugin_categories_, category))
    {
      z_debug ("New category: {}", category);
      plugin_categories_.emplace_back (category);
    }

  if (!author.empty ())
    {
      if (!std::ranges::contains (plugin_authors_, author))
        {
          z_debug ("New author: {}", author);
          plugin_authors_.emplace_back (author);
        }
    }
}

void
PluginManager::add_descriptor (const zrythm::plugins::PluginDescriptor &descr)
{
#if 0
  z_return_if_fail (descr.protocol_ > Protocol::ProtocolType::Internal);
  plugin_descriptors_->addDescriptor (descr);
  add_category_and_author (descr.category_str_, descr.author_);
#endif
}

std::filesystem::path
PluginManager::get_known_plugins_xml_path ()
{
  QString local_app_data_path =
    QStandardPaths::writableLocation (QStandardPaths::AppLocalDataLocation);
  QDir dir (local_app_data_path);
  return utils::Utf8String::from_qstring (
    dir.absoluteFilePath (u"known_plugins.xml"_s));
}

void
PluginManager::serialize_known_plugins ()
{
  const auto known_plugins_xml_path = get_known_plugins_xml_path ();
  const auto known_plugins_xml_path_str =
    utils::Utf8String::from_path (known_plugins_xml_path);
  z_return_if_fail (known_plugin_list_);

  // create parent dir
  try
    {
      utils::io::mkdir (known_plugins_xml_path.parent_path ());
    }
  catch (const std::exception &e)
    {
      z_warning ("Failed to create directory for known plugins: {}", e.what ());
      return;
    }

  if (
    known_plugin_list_->createXml ()->writeTo (
      known_plugins_xml_path_str.to_juce_file ()))
    {
      z_debug ("Saved known plugins to {}", known_plugins_xml_path_str);
    }
  else
    {
      z_warning (
        "Failed to save known plugins to {}", known_plugins_xml_path_str);
    }
}

void
PluginManager::deserialize_known_plugins ()
{
  const auto known_plugins_xml_path = get_known_plugins_xml_path ();
  const auto known_plugins_xml_path_str =
    utils::Utf8String::from_path (known_plugins_xml_path);
  known_plugin_list_->clear ();
  const juce::File jfile (known_plugins_xml_path_str.to_juce_file ());
  if (jfile.existsAsFile ())
    {
      z_debug ("Loading known plugins from {}", known_plugins_xml_path_str);
      const auto xml_doc = juce::XmlDocument::parse (jfile);
      if (xml_doc)
        {
          known_plugin_list_->recreateFromXml (*xml_doc);
        }
      else
        {
          z_warning (
            "Failed to load known plugins from {}", known_plugins_xml_path_str);
        }

      // prune entries whose plugin files no longer exist
      const auto types = known_plugin_list_->getTypes ();
      int        num_pruned = 0;
      for (const auto &desc : types)
        {
          if (known_plugin_file_missing (desc))
            {
              known_plugin_list_->removeType (desc);
              ++num_pruned;
            }
        }
      if (num_pruned > 0)
        {
          z_info ("Pruned {} known plugins with missing files", num_pruned);
        }
    }
  else
    {
      z_info ("No known plugins file found at {}", known_plugins_xml_path_str);
    }

  add_internal_plugins_to_known_list ();
}

void
PluginManager::onScannerScanFinished ()
{
  // serialize
  serialize_known_plugins ();

  known_plugin_list_->sort (juce::KnownPluginList::sortAlphabetically, true);
  plugin_descriptors_->reset_model ();

  // relay the signal
  Q_EMIT scanFinished ();
}

void
PluginManager::beginScan ()
{
  if (qEnvironmentVariableIsSet ("ZRYTHM_SKIP_PLUGIN_SCAN"))
    {
      Q_EMIT scanFinished ();
      return;
    }

  // relay currently scanning plugin
  QObject::connect (
    scanner_.get (),
    &::zrythm::plugins::PluginScanManager::currentlyScanningPluginChanged, this,
    &PluginManager::currentlyScanningPluginChanged);

  // get notified by the scanner when it has finished scanning
  QObject::connect (
    scanner_.get (), &::zrythm::plugins::PluginScanManager::scanningFinished,
    this, &PluginManager::onScannerScanFinished);

  deserialize_known_plugins ();

  scanner_->beginScan ();
}

std::unique_ptr<zrythm::plugins::PluginDescriptor>
PluginManager::find_plugin_from_uri (const utils::Utf8String &uri) const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [&uri] (const zrythm::plugins::PluginDescriptor &descr) {
      return uri == descr.uri_;
    });
  if (it != plugin_descriptors_.end ())
    {
      return std::make_unique<PluginDescriptor> (*it);
    }
  else
    {
      z_debug ("descriptor for URI {} not found", uri);
      return nullptr;
    }
#endif
  return nullptr;
}

std::unique_ptr<zrythm::plugins::PluginDescriptor>
PluginManager::find_from_descriptor (
  const zrythm::plugins::PluginDescriptor &src_descr) const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [&src_descr] (const zrythm::plugins::PluginDescriptor &descr) {
      return src_descr.is_same_plugin (descr);
    });
  if (it != plugin_descriptors_.end ())
    {
      return std::make_unique<PluginDescriptor> (*it);
    }
  else
    {
      z_debug ("descriptor for {} not found", src_descr.name_);
      return nullptr;
    }
#endif
  return nullptr;
}

std::unique_ptr<zrythm::plugins::PluginDescriptor>
PluginManager::pick_instrument () const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [] (const auto &descr) { return descr.is_instrument (); });
  if (it != plugin_descriptors_.end ())
    {
      return std::make_unique<PluginDescriptor> (*it);
    }
  else
    {
      z_debug ("no instrument found");
      return nullptr;
    }
#endif
  return nullptr;
}

void
PluginManager::clear_plugins ()
{
  known_plugin_list_->clear ();
  plugin_categories_.clear ();
  plugin_authors_.clear ();

  add_internal_plugins_to_known_list ();
}
