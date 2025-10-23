// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <juce_wrapper.h>

namespace zrythm::plugins
{

class CLAPPluginFormat final : public juce::AudioPluginFormat
{
public:
  CLAPPluginFormat () = default;
  ~CLAPPluginFormat () override;

  /**
   * @brief Used for hashing CLAP IDs
   *
   */
  static auto get_hash_for_range (auto &&range) -> int
  {
    return static_cast<int> (std::ranges::fold_left (
      range, uint32_t{ 0 }, [] (uint32_t acc, auto &&item) {
        return (acc * 31) + static_cast<uint32_t> (item);
      }));
  };

  static juce::String getFormatName () { return "CLAP"; }
  juce::String        getName () const override { return getFormatName (); }
  bool                canScanForPlugins () const override { return true; }
  bool                isTrivialToScan () const override { return false; }

  void findAllTypesForFile (
    juce::OwnedArray<juce::PluginDescription> &,
    const juce::String &fileOrIdentifier) override;
  bool
  fileMightContainThisPluginType (const juce::String &fileOrIdentifier) override;
  juce::String
  getNameOfPluginFromIdentifier (const juce::String &fileOrIdentifier) override;
  bool pluginNeedsRescanning (const juce::PluginDescription &) override;
  juce::StringArray
  searchPathsForPlugins (const juce::FileSearchPath &, bool recursive, bool)
    override;
  bool doesPluginStillExist (const juce::PluginDescription &) override;
  juce::FileSearchPath getDefaultLocationsToSearch () override;

private:
  void createPluginInstance (
    const juce::PluginDescription &desc,
    double                         initialSampleRate,
    int                            initialBufferSize,
    PluginCreationCallback) override;
  bool requiresUnblockedMessageThreadDuringCreation (
    const juce::PluginDescription &desc) const override;
  void recursiveFileSearch (
    juce::StringArray &results,
    const juce::File  &directory,
    bool               recursive);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CLAPPluginFormat)
};
}
