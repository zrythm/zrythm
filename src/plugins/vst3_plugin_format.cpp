// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// VST3 discovery implemented against the VST3 SDK's hosting helpers
// (public.sdk/source/vst/hosting/module.h: Module, PluginFactory, ClassInfo).

#include <ranges>

#include "plugins/plugin_format_utils.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/format_juce.h"
#include "utils/logger.h"

#include <QtSystemDetection>

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <public.sdk/source/vst/hosting/module.h>

namespace zrythm::plugins
{

static bool
class_info_is_instrument (const VST3::Hosting::ClassInfo &class_info)
{
  const auto &sub_categories = class_info.subCategories ();
  return std::ranges::any_of (sub_categories, [] (const auto &sub_category) {
    return sub_category.find (Steinberg::Vst::PlugType::kInstrument)
           != std::string::npos;
  });
}

void
Vst3PluginFormat::findAllTypesForFile (
  juce::OwnedArray<juce::PluginDescription> &results,
  const juce::String                        &fileOrIdentifier)
{
  if (!fileMightContainThisPluginType (fileOrIdentifier))
    return;

  std::string error;
  const auto  module =
    VST3::Hosting::Module::create (fileOrIdentifier.toStdString (), error);
  if (module == nullptr)
    {
      z_warning ("Failed to load VST3 module '{}': {}", fileOrIdentifier, error);
      return;
    }

  const auto  plugin_file = juce::File (fileOrIdentifier);
  const auto &factory = module->getFactory ();
  const auto  factory_vendor = factory.info ().vendor ();
  for (const auto &class_info : factory.classInfos ())
    {
      auto desc = std::make_unique<juce::PluginDescription> ();
      desc->fileOrIdentifier = fileOrIdentifier;
      // Recorded at scan time so pluginNeedsRescanning() can detect
      // on-disk changes
      desc->lastFileModTime = effective_modification_time (plugin_file);
      desc->lastInfoUpdateTime = juce::Time::getCurrentTime ();
      desc->pluginFormatName = getFormatName ();
      desc->name = class_info.name ();
      desc->manufacturerName =
        class_info.vendor ().empty () ? factory_vendor : class_info.vendor ();
      desc->version = class_info.version ();
      desc->uniqueId = get_hash_for_range (class_info.ID ().toString ());
      desc->deprecatedUid = desc->uniqueId;
      desc->isInstrument = class_info_is_instrument (class_info);
      desc->category = desc->isInstrument ? "Instrument" : "Effect";
      // Channel counts require instantiation; the actual bus layout is
      // queried by Vst3Plugin at load time
      desc->numInputChannels = 2;
      desc->numOutputChannels = 2;
      results.add (std::move (desc));
    }
}

void
Vst3PluginFormat::createPluginInstance (
  const juce::PluginDescription &,
  double,
  int,
  PluginCreationCallback callback)
{
  // VST3 plugins are hosted natively by Vst3Plugin, not via
  // juce::AudioPluginInstance
  callback (nullptr, "unsupported");
}

bool
Vst3PluginFormat::requiresUnblockedMessageThreadDuringCreation (
  const juce::PluginDescription &) const
{
  return false;
}

bool
Vst3PluginFormat::fileMightContainThisPluginType (
  const juce::String &fileOrIdentifier)
{
  const auto f = juce::File::createFileWithoutCheckingPath (fileOrIdentifier);

  // A .vst3 can be a bundle directory or a single binary
  return f.hasFileExtension ("vst3");
}

juce::String
Vst3PluginFormat::getNameOfPluginFromIdentifier (
  const juce::String &fileOrIdentifier)
{
  return juce::File (fileOrIdentifier).getFileNameWithoutExtension ();
}

bool
Vst3PluginFormat::pluginNeedsRescanning (
  const juce::PluginDescription &description)
{
  return effective_modification_time (juce::File (description.fileOrIdentifier))
         != description.lastFileModTime;
}

bool
Vst3PluginFormat::doesPluginStillExist (
  const juce::PluginDescription &description)
{
  return juce::File (description.fileOrIdentifier).exists ();
}

juce::StringArray
Vst3PluginFormat::searchPathsForPlugins (
  const juce::FileSearchPath &directoriesToSearch,
  const bool                  recursive,
  bool)
{
  juce::StringArray results;

  for (const auto i : std::views::iota (0, directoriesToSearch.getNumPaths ()))
    recursiveFileSearch (results, directoriesToSearch[i], recursive);

  return results;
}

void
Vst3PluginFormat::recursiveFileSearch (
  juce::StringArray &results,
  const juce::File  &directory,
  const bool         recursive)
{
  for (
    const auto &iter : juce::RangedDirectoryIterator (
      directory, false, "*", juce::File::findFilesAndDirectories))
    {
      const auto f = iter.getFile ();

      if (fileMightContainThisPluginType (f.getFullPathName ()))
        {
          results.add (f.getFullPathName ());
        }
      else if (recursive && f.isDirectory ())
        {
          recursiveFileSearch (results, f, true);
        }
    }
}

juce::FileSearchPath
Vst3PluginFormat::getDefaultLocationsToSearch ()
{
#ifdef Q_OS_WIN
  const auto localAppData =
    juce::File::getSpecialLocation (juce::File::windowsLocalAppData)
      .getFullPathName ();
  const auto programFiles =
    juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory)
      .getFullPathName ();
  return juce::FileSearchPath{
    localAppData + "\\Programs\\Common\\VST3;" + programFiles
    + "\\Common Files\\VST3"
  };
#elifdef Q_OS_APPLE
  return juce::FileSearchPath{
    "~/Library/Audio/Plug-Ins/VST3;/Library/Audio/Plug-Ins/VST3"
  };
#else
  return juce::FileSearchPath{ "~/.vst3/;/usr/lib/vst3/;/usr/local/lib/vst3/" };
#endif
}

Vst3PluginFormat::~Vst3PluginFormat () = default;

} // namespace zrythm::plugins
