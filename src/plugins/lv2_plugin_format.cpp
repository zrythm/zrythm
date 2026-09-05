// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// LV2 discovery implemented against lilv (metadata-only: bundle ttl is
// parsed, but plugin binaries are never loaded during scanning).

#include "zrythm-config.h"

#include <ranges>

#include "plugins/lv2_discovery.h"
#include "plugins/lv2_plugin_format.h"
#include "plugins/lv2_world.h"
#include "plugins/plugin_format_utils.h"
#include "utils/format_juce.h"
#include "utils/logger.h"

namespace zrythm::plugins
{

/**
 * @brief Canonicalized form of a bundle path.
 *
 * Search results, stored plugin identities and blacklist entries are all
 * matched by exact string comparison, so they must share this one form
 * even when a search path contains symlinks.
 */
static std::filesystem::path
canonical_bundle_path (const juce::String &path)
{
  auto bundle_path = utils::Utf8String::from_juce_string (path).to_path ();
  // canonicalize so that identities and lilv's bundle URIs match even when
  // a search path contains symlinks
  std::error_code ec;
  if (
    const auto canonical = std::filesystem::weakly_canonical (bundle_path, ec);
    !ec)
    {
      return canonical;
    }
  return bundle_path;
}

std::filesystem::path
Lv2PluginFormat::get_spec_bundles_dir ()
{
  // the executable lives in <prefix>/bin (both in the build tree's
  // products/bin and in an installed prefix), so two levels up is the
  // prefix
  const auto prefix =
    juce::File::getSpecialLocation (juce::File::currentExecutableFile)
      .getParentDirectory ()
      .getParentDirectory ();
  return std::filesystem::path{
    prefix.getChildFile (DATADIR_NAME "/zrythm/lv2-specs")
      .getFullPathName ()
      .toStdString ()
  };
}

Lv2PluginFormat::Lv2PluginFormat (std::shared_ptr<Lv2World> world)
    : world_ (std::move (world))
{
}

Lv2PluginFormat::~Lv2PluginFormat () = default;

void
Lv2PluginFormat::findAllTypesForFile (
  juce::OwnedArray<juce::PluginDescription> &results,
  const juce::String                        &fileOrIdentifier)
{
  if (!fileMightContainThisPluginType (fileOrIdentifier))
    return;

  const auto bundle_path = canonical_bundle_path (fileOrIdentifier);
  const auto plugin_infos = get_plugins_in_bundle (*world_, bundle_path);

  if (plugin_infos.empty ())
    {
      // valid outcome, e.g. for bundles that only contain presets
      z_debug ("Found no LV2 plugins in bundle '{}'", bundle_path);
      return;
    }

  const auto mod_time = effective_modification_time (
    juce::File (utils::Utf8String::from_path (bundle_path).to_juce_string ()));
  for (const auto &info : plugin_infos)
    {
      auto desc = std::make_unique<juce::PluginDescription> ();
      // Keyed by the bundle path (the scan identifier) so that
      // KnownPluginList can match existing entries and skip unchanged
      // bundles; the plugin URI is folded into uniqueId
      desc->fileOrIdentifier =
        utils::Utf8String::from_path (bundle_path).to_juce_string ();
      desc->lastFileModTime = mod_time;
      desc->lastInfoUpdateTime = juce::Time::getCurrentTime ();
      desc->pluginFormatName = getFormatName ();
      desc->name = info.name_.to_juce_string ();
      desc->manufacturerName = info.author_.to_juce_string ();
      desc->uniqueId = get_hash_for_range (info.uri_.view ());
      desc->deprecatedUid = desc->uniqueId;
      desc->version = info.version_.to_juce_string ();
      desc->isInstrument = info.is_instrument_;
      desc->category =
        info.category_str_.to_juce_string ().isEmpty ()
          ? (info.is_instrument_ ? "Instrument" : "Effect")
          : info.category_str_.to_juce_string ();
      // Unlike VST3, LV2 declares its port counts statically
      desc->numInputChannels = info.num_audio_ins_;
      desc->numOutputChannels = info.num_audio_outs_;
      results.add (std::move (desc));
    }
}

void
Lv2PluginFormat::createPluginInstance (
  const juce::PluginDescription &,
  double,
  int,
  PluginCreationCallback callback)
{
  // LV2 plugins are hosted natively (future Lv2Plugin), not via
  // juce::AudioPluginInstance
  callback (nullptr, "unsupported");
}

bool
Lv2PluginFormat::requiresUnblockedMessageThreadDuringCreation (
  const juce::PluginDescription &) const
{
  // Discovery is pure ttl parsing (no message-thread APIs), so the scanner
  // subprocess can scan LV2 bundles directly on its connection thread
  // instead of deferring to the message thread. JUCE's generic async
  // creation paths also consult this; it has no effect while instantiation
  // is unsupported.
  return true;
}

bool
Lv2PluginFormat::fileMightContainThisPluginType (
  const juce::String &fileOrIdentifier)
{
  const auto f = juce::File::createFileWithoutCheckingPath (fileOrIdentifier);
  return f.hasFileExtension ("lv2");
}

juce::String
Lv2PluginFormat::getNameOfPluginFromIdentifier (
  const juce::String &fileOrIdentifier)
{
  return juce::File (fileOrIdentifier).getFileNameWithoutExtension ();
}

bool
Lv2PluginFormat::pluginNeedsRescanning (const juce::PluginDescription &desc)
{
  // the bundle's effective modification time is recorded at scan time
  // (see findAllTypesForFile)
  return effective_modification_time (juce::File (desc.fileOrIdentifier))
         != desc.lastFileModTime;
}

bool
Lv2PluginFormat::doesPluginStillExist (const juce::PluginDescription &desc)
{
  return juce::File (desc.fileOrIdentifier).exists ();
}

juce::StringArray
Lv2PluginFormat::searchPathsForPlugins (
  const juce::FileSearchPath &directoriesToSearch,
  const bool                  recursive,
  bool)
{
  juce::StringArray results;

  for (const auto i : std::views::iota (0, directoriesToSearch.getNumPaths ()))
    {
      for (
        const auto &iter : juce::RangedDirectoryIterator (
          directoriesToSearch[i], false, "*", juce::File::findDirectories))
        {
          const auto f = iter.getFile ();
          if (fileMightContainThisPluginType (f.getFullPathName ()))
            {
              results.add (
                utils::Utf8String::from_path (
                  canonical_bundle_path (f.getFullPathName ()))
                  .to_juce_string ());
            }
          else if (recursive)
            {
              results.addArray (searchPathsForPlugins (
                juce::FileSearchPath{ f.getFullPathName () }, true, false));
            }
        }
    }

  return results;
}

juce::FileSearchPath
Lv2PluginFormat::getDefaultLocationsToSearch ()
{
  // Unused in our scan flow: search paths come from PluginProtocolPaths
  // (user settings + platform defaults), not from the format
  return {};
}

} // namespace zrythm::plugins
