// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "plugins/clap_plugin_format.h"
#include "plugins/lv2_plugin_format.h"
#include "plugins/lv2_world.h"
#include "plugins/plugin_scan_format.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/format_juce.h"
#include "utils/logger.h"

namespace zrythm::plugins
{

namespace
{

/**
 * @brief Adapter for the plugin formats hosted through
 * juce::AudioPluginFormat.
 *
 * JUCE descriptions never leave the adapter: scan results are converted to
 * Zrythm descriptors.
 */
class JucePluginScanFormat final : public PluginScanFormat
{
public:
  explicit JucePluginScanFormat (juce::AudioPluginFormat &format)
      : format_ (format)
  {
  }

  [[nodiscard]] Protocol::ProtocolType protocol () const override
  {
    return Protocol::from_juce_format_name (format_.getName ());
  }

  [[nodiscard]] utils::Utf8String format_name () const override
  {
    return utils::Utf8String::from_juce_string (format_.getName ());
  }

  [[nodiscard]] std::vector<utils::Utf8String>
  search_identifiers (const utils::FilePathList &search_paths, bool recursive)
    const override
  {
    return format_.searchPathsForPlugins (
             search_paths.get_as_juce_file_search_path (), recursive, true)
           | std::views::transform ([] (const juce::String &identifier) {
               return utils::Utf8String::from_juce_string (identifier);
             })
           | std::ranges::to<std::vector> ();
  }

  [[nodiscard]] std::
    expected<std::vector<std::unique_ptr<PluginDescriptor>>, utils::Utf8String>
    scan_identifier (const utils::Utf8String &identifier) override
  {
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format_.findAllTypesForFile (descriptions, identifier.to_juce_string ());

    std::vector<std::unique_ptr<PluginDescriptor>> descriptors;
    for (const auto * description : descriptions)
      {
        auto descriptor = PluginDescriptor::from_juce_description (*description);
        // JUCE descriptions carry no MIDI port information, so the plugin
        // is instantiated to query its MIDI capabilities
        auto instance =
          format_.createInstanceFromDescription (*description, 48000.0, 512);
        if (instance == nullptr)
          return std::unexpected (
            utils::Utf8String::from_utf8_encoded_string (
              fmt::format (
                "JUCE plugin instantiation failed for '{}'",
                descriptor->name_.view ())));

        descriptor->num_midi_ins_ = instance->acceptsMidi () ? 1 : 0;
        descriptor->num_midi_outs_ = instance->producesMidi () ? 1 : 0;
        descriptors.push_back (std::move (descriptor));
      }
    return descriptors;
  }

  [[nodiscard]] bool
  plugin_needs_rescanning (const PluginDescriptor &descriptor) const override
  {
    const auto juce_descriptor = descriptor.to_juce_description ();
    return format_.pluginNeedsRescanning (*juce_descriptor);
  }

  [[nodiscard]] bool
  plugin_exists (const PluginDescriptor &descriptor) const override
  {
    const auto juce_descriptor = descriptor.to_juce_description ();
    return format_.doesPluginStillExist (*juce_descriptor);
  }

  [[nodiscard]] bool
  scan_uses_message_thread (const utils::Utf8String &identifier) const override
  {
    juce::PluginDescription description;
    description.fileOrIdentifier = identifier.to_juce_string ();
    return !format_.requiresUnblockedMessageThreadDuringCreation (description);
  }

private:
  juce::AudioPluginFormat &format_;
};

bool
has_native_implementation (const juce::String &format_name)
{
  return format_name == "VST3" || format_name == "LV2";
}

} // namespace

struct PluginScanFormatManager::Impl
{
  std::shared_ptr<Lv2World>                       lv2_world;
  std::shared_ptr<juce::AudioPluginFormatManager> juce_format_manager;
  std::vector<std::unique_ptr<PluginScanFormat>>  formats;
};

PluginScanFormatManager::PluginScanFormatManager (
  std::shared_ptr<Lv2World> lv2_world)
    : impl_ (std::make_unique<Impl> ())
{
  impl_->lv2_world = std::move (lv2_world);
  impl_->juce_format_manager =
    std::make_shared<juce::AudioPluginFormatManager> ();
  juce::addHeadlessDefaultFormatsToManager (*impl_->juce_format_manager);

  impl_->formats.push_back (std::make_unique<Vst3PluginFormat> ());
  impl_->formats.push_back (std::make_unique<CLAPPluginFormat> ());
  if (impl_->lv2_world != nullptr)
    {
      impl_->formats.push_back (
        std::make_unique<Lv2PluginFormat> (impl_->lv2_world));
    }
  else
    {
      z_warning ("LV2 scanning unavailable: no LV2 world");
    }

  for (auto * format : impl_->juce_format_manager->getFormats ())
    {
      if (has_native_implementation (format->getName ()))
        continue;
      impl_->formats.push_back (
        std::make_unique<JucePluginScanFormat> (*format));
    }
}

PluginScanFormatManager::~PluginScanFormatManager () = default;

std::vector<PluginScanFormat *>
PluginScanFormatManager::formats () const
{
  return impl_->formats
         | std::views::transform (
           [] (const std::unique_ptr<PluginScanFormat> &format) {
             return format.get ();
           })
         | std::ranges::to<std::vector> ();
}

PluginScanFormat *
PluginScanFormatManager::find_format (const utils::Utf8String &format_name) const
{
  const auto found = std::ranges::find_if (
    impl_->formats,
    [&format_name] (const std::unique_ptr<PluginScanFormat> &format) {
      return format->format_name () == format_name;
    });
  return found == impl_->formats.end () ? nullptr : found->get ();
}

std::shared_ptr<juce::AudioPluginFormatManager>
PluginScanFormatManager::juce_format_manager () const
{
  return impl_->juce_format_manager;
}

} // namespace zrythm::plugins
