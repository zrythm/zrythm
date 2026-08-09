// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

namespace zrythm::plugins
{

/**
 * @brief Native VST3 plugin discovery, replacing juce::VST3PluginFormat.
 *
 * Implements the juce::AudioPluginFormat interface (so KnownPluginList and
 * the out-of-process scanner keep working) but discovers plugins directly via
 * the VST3 SDK's VST3::Hosting::Module. Instantiation via
 * createPluginInstance() is unsupported: hosting is done by Vst3Plugin.
 */
class Vst3PluginFormat final : public juce::AudioPluginFormat
{
public:
  Vst3PluginFormat () = default;
  ~Vst3PluginFormat () override;

  /**
   * @brief Returns a stable ID for a VST3 class ID (TUID) string.
   *
   * Used for PluginDescription::uniqueId, and by Vst3Plugin to find the
   * matching class inside a module.
   */
  static auto get_hash_for_range (auto &&range) -> int
  {
    return static_cast<int> (std::ranges::fold_left (
      range, uint32_t{ 0 }, [] (uint32_t acc, auto &&item) {
        return (acc * 31) + static_cast<uint32_t> (item);
      }));
  };

  static juce::String getFormatName () { return "VST3"; }
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

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Vst3PluginFormat)
};

} // namespace zrythm::plugins
