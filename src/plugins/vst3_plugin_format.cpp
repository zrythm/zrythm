// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// VST3 discovery implemented against the VST3 SDK's hosting helpers
// (public.sdk/source/vst/hosting/module.h: Module, PluginFactory, ClassInfo).

#include <limits>
#include <ranges>
#include <string>
#include <vector>

#include <fmt/std.h>

#include "plugins/plugin_format_utils.h"
#include "plugins/plugin_scan_format.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/format_juce.h"
#include "utils/logger.h"

#include <QtSystemDetection>

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <public.sdk/source/vst/hosting/hostclasses.h>
#include <public.sdk/source/vst/hosting/module.h>

namespace zrythm::plugins
{

namespace
{

bool
class_info_is_instrument (const VST3::Hosting::ClassInfo &class_info)
{
  const auto &sub_categories = class_info.subCategories ();
  return std::ranges::any_of (sub_categories, [] (const auto &sub_category) {
    return sub_category.find (Steinberg::Vst::PlugType::kInstrument)
           != std::string::npos;
  });
}

std::int64_t
current_time_ms ()
{
  return juce::Time::getCurrentTime ().toMilliseconds ();
}

std::expected<int, utils::Utf8String>
audio_channel_count (
  Steinberg::Vst::IComponent  &component,
  Steinberg::Vst::BusDirection direction)
{
  using namespace Steinberg;

  constexpr auto max_int = std::numeric_limits<int>::max ();
  const int      bus_count =
    component.getBusCount (Vst::MediaTypes::kAudio, direction);
  if (bus_count < 0)
    return std::unexpected (
      utils::Utf8String::from_utf8_encoded_string (
        "VST3 plugin reported a negative audio bus count"));

  int channel_count = 0;
  for (const int bus_index : std::views::iota (0, bus_count))
    {
      Vst::BusInfo bus_info{};
      if (
        component.getBusInfo (
          Vst::MediaTypes::kAudio, direction, bus_index, bus_info)
        != kResultOk)
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("VST3 plugin did not report audio bus {}", bus_index)));

      if (
        bus_info.channelCount < 0
        || bus_info.channelCount > max_int - channel_count)
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            "VST3 plugin reported an invalid audio channel count"));
      channel_count += static_cast<int> (bus_info.channelCount);
    }
  return channel_count;
}

std::expected<int, utils::Utf8String>
event_bus_count (
  Steinberg::Vst::IComponent  &component,
  Steinberg::Vst::BusDirection direction)
{
  const int count =
    component.getBusCount (Steinberg::Vst::MediaTypes::kEvent, direction);
  if (count < 0)
    return std::unexpected (
      utils::Utf8String::from_utf8_encoded_string (
        "VST3 plugin reported a negative event bus count"));
  return count;
}
} // namespace

Vst3PluginFormat::~Vst3PluginFormat () = default;

Protocol::ProtocolType
Vst3PluginFormat::protocol () const
{
  return Protocol::ProtocolType::VST3;
}

utils::Utf8String
Vst3PluginFormat::format_name () const
{
  return u8"VST3";
}

bool
Vst3PluginFormat::file_might_contain_this_plugin_type (
  const utils::Utf8String &identifier) const
{
  return identifier.to_qstring ().endsWith (
    QStringLiteral (".vst3"), Qt::CaseInsensitive);
}

std::vector<utils::Utf8String>
Vst3PluginFormat::search_identifiers (
  const utils::FilePathList &search_paths,
  bool                       recursive) const
{
  std::vector<utils::Utf8String> results;
  for (const auto &path : search_paths)
    recursive_file_search (results, path, recursive);
  return results;
}

void
Vst3PluginFormat::recursive_file_search (
  std::vector<utils::Utf8String> &results,
  const std::filesystem::path    &directory,
  bool                            recursive) const
{
  std::error_code ec;
  for (const auto &entry : std::filesystem::directory_iterator (directory, ec))
    {
      const auto path = entry.path ();
      const auto path_string = utils::Utf8String::from_path (path);
      // a .vst3 identifier can be a single binary or a bundle directory
      if (file_might_contain_this_plugin_type (path_string))
        {
          results.push_back (path_string);
        }
      else if (recursive && entry.is_directory (ec))
        {
          recursive_file_search (results, path, true);
        }
    }
  if (ec)
    {
      z_warning ("VST3: failed to search '{}': {}", directory, ec.message ());
    }
}

std::expected<std::vector<std::unique_ptr<PluginDescriptor>>, utils::Utf8String>
Vst3PluginFormat::scan_identifier (const utils::Utf8String &identifier)
{
  if (!file_might_contain_this_plugin_type (identifier))
    return std::vector<std::unique_ptr<PluginDescriptor>>{};

  std::string load_error;
  const auto  module = VST3::Hosting::Module::create (
    std::string{ identifier.view () }, load_error);
  if (module == nullptr)
    return std::unexpected (
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("Failed to load VST3 module: {}", load_error)));

  std::vector<std::unique_ptr<PluginDescriptor>> descriptors;
  const auto &factory = module->getFactory ();
  const auto  factory_vendor = factory.info ().vendor ();
  const auto  plugin_file = juce::File (identifier.to_juce_string ());
  const auto  modification_time = effective_modification_time (plugin_file);

  for (const auto &class_info : factory.classInfos ())
    {
      // A factory also lists non-plugin classes (e.g. the edit controller
      // class of dual-component plugins); only audio module classes are
      // plugins the host can instantiate
      if (class_info.category () != kVstAudioEffectClass)
        continue;

      Steinberg::Vst::HostApplication host_application;
      auto                            component =
        factory.createInstance<Steinberg::Vst::IComponent> (class_info.ID ());
      if (component == nullptr)
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format (
              "Failed to create VST3 component for '{}'", class_info.name ())));

      if (component->initialize (&host_application) != Steinberg::kResultOk)
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format (
              "VST3 component initialization failed for '{}'",
              class_info.name ())));

      const auto num_event_ins =
        event_bus_count (*component, Steinberg::Vst::kInput);
      const auto num_event_outs =
        event_bus_count (*component, Steinberg::Vst::kOutput);
      const auto num_audio_ins =
        audio_channel_count (*component, Steinberg::Vst::kInput);
      const auto num_audio_outs =
        audio_channel_count (*component, Steinberg::Vst::kOutput);
      if (
        !num_event_ins.has_value () || !num_event_outs.has_value ()
        || !num_audio_ins.has_value () || !num_audio_outs.has_value ())
        {
          component->terminate ();
          return std::unexpected (
            !num_event_ins    ? num_event_ins.error ()
            : !num_event_outs ? num_event_outs.error ()
            : !num_audio_ins
              ? num_audio_ins.error ()
              : num_audio_outs.error ());
        }

      auto descriptor = std::make_unique<PluginDescriptor> ();
      descriptor->path_or_id_ = identifier.to_path ();
      descriptor->last_file_mod_time_ms_ = modification_time.toMilliseconds ();
      descriptor->last_info_update_time_ms_ = current_time_ms ();
      descriptor->protocol_ = Protocol::ProtocolType::VST3;
      descriptor->name_ =
        utils::Utf8String::from_utf8_encoded_string (class_info.name ());
      descriptor->author_ = utils::Utf8String::from_utf8_encoded_string (
        class_info.vendor ().empty () ? factory_vendor : class_info.vendor ());
      descriptor->version_ =
        utils::Utf8String::from_utf8_encoded_string (class_info.version ());
      descriptor->unique_id_ = get_hash_for_range (class_info.ID ().toString ());
      descriptor->juce_compat_deprecated_unique_id_ =
        static_cast<int> (descriptor->unique_id_);
      descriptor->category_ =
        class_info_is_instrument (class_info)
          ? PluginCategory::Instrument
          : PluginCategory::None;
      descriptor->category_str_ =
        descriptor->category_ == PluginCategory::Instrument
          ? u8"Instrument"
          : u8"Effect";
      descriptor->num_audio_ins_ = *num_audio_ins;
      descriptor->num_audio_outs_ = *num_audio_outs;
      descriptor->num_midi_ins_ = *num_event_ins;
      descriptor->num_midi_outs_ = *num_event_outs;
      descriptors.push_back (std::move (descriptor));

      component->terminate ();
    }

  return descriptors;
}

bool
Vst3PluginFormat::plugin_needs_rescanning (
  const PluginDescriptor &descriptor) const
{
  const auto identifier = descriptor.source_identifier ();
  return effective_modification_time (juce::File (identifier.to_juce_string ()))
           .toMilliseconds ()
         != descriptor.last_file_mod_time_ms_;
}

bool
Vst3PluginFormat::plugin_exists (const PluginDescriptor &descriptor) const
{
  return std::filesystem::exists (descriptor.source_identifier ().to_path ());
}

bool
Vst3PluginFormat::scan_uses_message_thread (const utils::Utf8String &) const
{
  return true;
}

} // namespace zrythm::plugins
