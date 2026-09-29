// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

namespace zrythm::plugins
{

class Lv2World;

/**
 * @brief Native LV2 plugin discovery via lilv, replacing
 * juce::LV2PluginFormat.
 *
 * Implements the juce::AudioPluginFormat interface (so KnownPluginList and
 * the out-of-process scanner keep working) but discovers plugins via lilv.
 * Discovery is metadata-only: bundle ttl is parsed, but no plugin binary is
 * ever loaded. Instantiation via createPluginInstance() is unsupported:
 * hosting will be done by a dedicated Lv2Plugin.
 */
class Lv2PluginFormat final : public juce::AudioPluginFormat
{
public:
  /**
   * @param world The world used for bundle parsing, kept alive by the
   * format. Callers share one world per process.
   */
  explicit Lv2PluginFormat (std::shared_ptr<Lv2World> world);
  ~Lv2PluginFormat () override;

  /**
   * @brief Returns the directory the LV2 specification bundles are shipped
   * in: share/zrythm/lv2-specs under the installation prefix.
   */
  static std::filesystem::path get_spec_bundles_dir ();

  static juce::String getFormatName () { return "LV2"; }
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

private:
  std::shared_ptr<Lv2World> world_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Lv2PluginFormat)
};

} // namespace zrythm::plugins
