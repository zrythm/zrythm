// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * SPDX-FileCopyrightText: Copyright 2022, Jatin Chowdhury
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2022, Jatin Chowdhury
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ---
 *
 */

#include "plugins/CLAPPluginFormat.h"

#include <QtSystemDetection>

#include <clap/clap.h>

namespace zrythm::plugins
{
using namespace juce;

static void
createPluginDescription (
  PluginDescription             &description,
  const File                    &pluginFile,
  const clap_plugin_descriptor * clapDesc,
  int                            numInputs,
  int                            numOutputs)
{
  description.fileOrIdentifier = pluginFile.getFullPathName ();
  description.lastFileModTime = pluginFile.getLastModificationTime ();
  description.lastInfoUpdateTime = Time::getCurrentTime ();

  description.numInputChannels = numInputs;
  description.numOutputChannels = numOutputs;

  description.manufacturerName = clapDesc->vendor;
  ;
  description.name = clapDesc->name;
  description.descriptiveName = clapDesc->description;
  description.pluginFormatName = "CLAP";

  description.uniqueId =
    CLAPPluginFormat::get_hash_for_range (std::string (clapDesc->id));

  description.version = clapDesc->version;
  juce::StringArray features_strings{ clapDesc->features };
  description.category =
    clapDesc->features[1] != nullptr
      ? clapDesc->features[1]
      : clapDesc->features[0]; // @TODO: better feature detection...

  description.isInstrument =
    features_strings.contains (CLAP_PLUGIN_FEATURE_INSTRUMENT, true);
}

//==============================================================================
struct DescriptionFactory
{
  explicit DescriptionFactory (const clap_plugin_factory * pluginFactory)
      : factory (pluginFactory)
  {
    jassert (pluginFactory != nullptr);
  }

  virtual ~DescriptionFactory () = default;

  Result findDescriptionsAndPerform (const File &pluginFile)
  {
    Result     result{ Result::ok () };
    const auto count = factory->get_plugin_count (factory);

    for (uint32_t i = 0; i < count; ++i)
      {
        const auto * const clapDesc =
          factory->get_plugin_descriptor (factory, i);
        PluginDescription desc;

        // TODO: inputs and outputs requires instantiating the plugin and having
        // a CLAP host filled in
        createPluginDescription (desc, pluginFile, clapDesc, 2, 2);

        result = performOnDescription (desc);

        if (result.failed ())
          break;
      }

    return result;
  }

  virtual Result performOnDescription (PluginDescription &) = 0;

private:
  const clap_plugin_factory * factory;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DescriptionFactory)
};

struct DescriptionLister : public DescriptionFactory
{
  explicit DescriptionLister (const clap_plugin_factory * pluginFactory)
      : DescriptionFactory (pluginFactory)
  {
  }

  Result performOnDescription (PluginDescription &desc) override
  {
    list.add (std::make_unique<PluginDescription> (desc));
    return Result::ok ();
  }

  OwnedArray<PluginDescription> list;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DescriptionLister)
};

void
CLAPPluginFormat::findAllTypesForFile (
  OwnedArray<PluginDescription> &results,
  const String                  &fileOrIdentifier)
{
  if (fileMightContainThisPluginType (fileOrIdentifier))
    {
      QLibrary lib;

      lib.setFileName (
        utils::Utf8String::from_juce_string (fileOrIdentifier).to_qstring ());
      lib.setLoadHints (
        QLibrary::ResolveAllSymbolsHint
#if !defined(__has_feature) || !__has_feature(address_sanitizer)
        | QLibrary::DeepBindHint
#endif
      );
      if (!lib.load ())
        {
          z_warning (
            "Failed to load plugin '{}': {}", fileOrIdentifier,
            lib.errorString ());
          return;
        }

      const auto * plugin_entry = reinterpret_cast<const clap_plugin_entry_t *> (
        lib.resolve ("clap_entry"));
      if (plugin_entry == nullptr)
        {
          z_warning (
            "Unable to resolve entry point 'clap_entry' in '{}'",
            fileOrIdentifier);
          return;
        }

      if (!plugin_entry->init (
            utils::Utf8String::from_juce_string (fileOrIdentifier).c_str ()))
        {
          z_warning ("clap_entry->init() failed for '{}'", fileOrIdentifier);
          return;
        }

      const auto * plugin_factory = static_cast<const clap_plugin_factory *> (
        plugin_entry->get_factory (CLAP_PLUGIN_FACTORY_ID));
      if (plugin_factory == nullptr)
        {
          z_warning ("failed to get plugin factory for {}", fileOrIdentifier);
          return;
        }

      DescriptionLister lister (plugin_factory);
      lister.findDescriptionsAndPerform (File (fileOrIdentifier));
      results.addCopiesOf (lister.list);
    }
}

void
CLAPPluginFormat::createPluginInstance (
  const PluginDescription &description,
  double,
  int,
  PluginCreationCallback callback)
{
  callback (nullptr, "unsupported");
}

bool
CLAPPluginFormat::requiresUnblockedMessageThreadDuringCreation (
  const PluginDescription &) const
{
  return false;
}

bool
CLAPPluginFormat::fileMightContainThisPluginType (const String &fileOrIdentifier)
{
  auto f = File::createFileWithoutCheckingPath (fileOrIdentifier);

  return f.hasFileExtension (".clap")
#ifdef Q_OS_MACOS
         || fileOrIdentifier.contains ("/Contents/MacOS")
#endif
    ;
}

String
CLAPPluginFormat::getNameOfPluginFromIdentifier (const String &fileOrIdentifier)
{
  // Impossible to tell because every CLAP is a type of shell...
  return fileOrIdentifier;
}

bool
CLAPPluginFormat::pluginNeedsRescanning (const PluginDescription &description)
{
  return File (description.fileOrIdentifier).getLastModificationTime ()
         != description.lastFileModTime;
}

bool
CLAPPluginFormat::doesPluginStillExist (const PluginDescription &description)
{
  return File (description.fileOrIdentifier).exists ();
}

StringArray
CLAPPluginFormat::searchPathsForPlugins (
  const FileSearchPath &directoriesToSearch,
  const bool            recursive,
  bool)
{
  StringArray results;

  for (int i = 0; i < directoriesToSearch.getNumPaths (); ++i)
    recursiveFileSearch (results, directoriesToSearch[i], recursive);

  return results;
}

void
CLAPPluginFormat::recursiveFileSearch (
  StringArray &results,
  const File  &directory,
  const bool   recursive)
{
  for (
    const auto &iter : RangedDirectoryIterator (
      directory, false, "*", File::findFilesAndDirectories))
    {
      auto f = iter.getFile ();
      bool isPlugin = false;

      if (
        fileMightContainThisPluginType (f.getFullPathName ())
        && !f.isDirectory ())
        {
          isPlugin = true;
          results.add (f.getFullPathName ());
        }

      if (recursive && (!isPlugin) && f.isDirectory ())
        recursiveFileSearch (results, f, true);
    }
}

FileSearchPath
CLAPPluginFormat::getDefaultLocationsToSearch ()
{
  // @TODO: check CLAP_PATH, as documented here:
  // https://github.com/free-audio/clap/blob/main/include/clap/entry.h

#ifdef Q_OS_WIN
  const auto localAppData =
    File::getSpecialLocation (File::windowsLocalAppData).getFullPathName ();
  const auto programFiles =
    File::getSpecialLocation (File::globalApplicationsDirectory)
      .getFullPathName ();
  return FileSearchPath{
    localAppData + "\\Programs\\Common\\CLAP;" + programFiles
    + "\\Common Files\\CLAP"
  };
#elifdef Q_OS_APPLE
  return FileSearchPath{
    "~/Library/Audio/Plug-Ins/CLAP;/Library/Audio/Plug-Ins/CLAP"
  };
#else
  return FileSearchPath{ "~/.clap/;/usr/lib/clap/" };
#endif
}

CLAPPluginFormat::~CLAPPluginFormat () = default;
}
