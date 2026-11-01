// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
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

#include <algorithm>
#include <cctype>
#include <limits>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/clap_plugin_format.h"
#include "plugins/plugin_format_utils.h"
#include "plugins/plugin_library.h"
#include "plugins/plugin_scan_format.h"
#include "utils/format_juce.h"
#include "utils/logger.h"

#include <QtSystemDetection>

#include <clap/clap.h>
#include <clap/ext/audio-ports.h>
#include <clap/ext/note-ports.h>

namespace zrythm::plugins
{

namespace
{

const void *
clap_host_get_extension (const clap_host *, const char *)
{
  return nullptr;
}

void
clap_host_request_restart (const clap_host *)
{
}

void
clap_host_request_process (const clap_host *)
{
}

void
clap_host_request_callback (const clap_host *)
{
}

clap_host
make_scan_host ()
{
  return clap_host{
    .clap_version = CLAP_VERSION,
    .host_data = nullptr,
    .name = "Zrythm",
    .vendor = "The Zrythm contributors",
    .url = "https://www.zrythm.org",
    .version = "2.0",
    .get_extension = clap_host_get_extension,
    .request_restart = clap_host_request_restart,
    .request_process = clap_host_request_process,
    .request_callback = clap_host_request_callback,
  };
}

std::vector<std::string_view>
clap_feature_strings (const char * const * features)
{
  std::vector<std::string_view> result;
  for (; features != nullptr && *features != nullptr; ++features)
    result.emplace_back (*features);
  return result;
}

bool
features_contain (
  const std::vector<std::string_view> &features,
  std::string_view                     expected)
{
  return std::ranges::any_of (features, [expected] (std::string_view feature) {
    return std::ranges::equal (feature, expected, [] (char a, char b) {
      return std::tolower (static_cast<unsigned char> (a))
             == std::tolower (static_cast<unsigned char> (b));
    });
  });
}

std::expected<int, utils::Utf8String>
audio_channel_count (const clap_plugin * plugin, bool is_input)
{
  const auto * extension = static_cast<const clap_plugin_audio_ports *> (
    plugin->get_extension (plugin, CLAP_EXT_AUDIO_PORTS));
  if (extension == nullptr)
    return 0;

  constexpr auto max_int = std::numeric_limits<int>::max ();
  int            channel_count = 0;
  const auto     port_count = extension->count (plugin, is_input);
  for (const uint32_t port_index : std::views::iota (0u, port_count))
    {
      clap_audio_port_info_t info{};
      if (!extension->get (plugin, port_index, is_input, &info))
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("CLAP plugin did not report audio port {}", port_index)));

      if (info.channel_count > static_cast<uint32_t> (max_int - channel_count))
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            "CLAP plugin reported an invalid audio channel count"));
      channel_count += static_cast<int> (info.channel_count);
    }
  return channel_count;
}

std::expected<int, utils::Utf8String>
note_port_count (const clap_plugin * plugin, bool is_input)
{
  const auto * extension = static_cast<const clap_plugin_note_ports *> (
    plugin->get_extension (plugin, CLAP_EXT_NOTE_PORTS));
  if (extension == nullptr)
    return 0;

  const auto port_count = extension->count (plugin, is_input);
  if (port_count > static_cast<uint32_t> (std::numeric_limits<int>::max ()))
    return std::unexpected (
      utils::Utf8String::from_utf8_encoded_string (
        "CLAP plugin reported an invalid note port count"));
  return static_cast<int> (port_count);
}

std::int64_t
current_time_ms ()
{
  return juce::Time::getCurrentTime ().toMilliseconds ();
}
} // namespace

CLAPPluginFormat::~CLAPPluginFormat () = default;

Protocol::ProtocolType
CLAPPluginFormat::protocol () const
{
  return Protocol::ProtocolType::CLAP;
}

utils::Utf8String
CLAPPluginFormat::format_name () const
{
  return u8"CLAP";
}

bool
CLAPPluginFormat::file_might_contain_this_plugin_type (
  const utils::Utf8String &identifier) const
{
  return identifier.to_qstring ().endsWith (
    QStringLiteral (".clap"), Qt::CaseInsensitive);
}

std::vector<utils::Utf8String>
CLAPPluginFormat::search_identifiers (
  const utils::FilePathList &search_paths,
  bool                       recursive) const
{
  std::vector<utils::Utf8String> results;
  for (const auto &path : search_paths)
    recursive_file_search (results, path, recursive);
  return results;
}

void
CLAPPluginFormat::recursive_file_search (
  std::vector<utils::Utf8String> &results,
  const std::filesystem::path    &directory,
  bool                            recursive) const
{
  std::error_code ec;
  for (const auto &entry : std::filesystem::directory_iterator (directory, ec))
    {
      const auto path = entry.path ();
      const auto path_string = utils::Utf8String::from_path (path);
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
      z_warning ("CLAP: failed to search '{}': {}", directory, ec.message ());
    }
}

std::expected<std::vector<std::unique_ptr<PluginDescriptor>>, utils::Utf8String>
CLAPPluginFormat::scan_identifier (const utils::Utf8String &identifier)
{
  if (!file_might_contain_this_plugin_type (identifier))
    return std::vector<std::unique_ptr<PluginDescriptor>>{};

  PluginLibrary library;
  if (!library.load (identifier))
    return std::unexpected (
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format (
          "Failed to load CLAP library: {}", library.error_string ().view ())));

  const auto * entry = reinterpret_cast<const clap_plugin_entry *> (
    library.resolve ("clap_entry"));
  if (entry == nullptr)
    return std::unexpected (
      utils::Utf8String::from_utf8_encoded_string (
        "Failed to resolve clap_entry"));

  if (!entry->init (identifier.c_str ()))
    return std::unexpected (
      utils::Utf8String::from_utf8_encoded_string ("clap_entry->init failed"));

  const auto * factory = static_cast<const clap_plugin_factory *> (
    entry->get_factory (CLAP_PLUGIN_FACTORY_ID));
  if (factory == nullptr)
    {
      entry->deinit ();
      return std::unexpected (
        utils::Utf8String::from_utf8_encoded_string (
          "CLAP plugin factory unavailable"));
    }

  std::vector<std::unique_ptr<PluginDescriptor>> descriptors;
  const auto                                     modification_time =
    effective_modification_time (juce::File (identifier.to_juce_string ()));
  const auto host = make_scan_host ();
  const auto plugin_count = factory->get_plugin_count (factory);

  for (const uint32_t plugin_index : std::views::iota (0u, plugin_count))
    {
      const auto * const plugin_info =
        factory->get_plugin_descriptor (factory, plugin_index);
      if (
        plugin_info == nullptr || plugin_info->id == nullptr
        || plugin_info->name == nullptr)
        {
          entry->deinit ();
          return std::unexpected (
            utils::Utf8String::from_utf8_encoded_string (
              fmt::format (
                "CLAP plugin descriptor unavailable for plugin index {}",
                plugin_index)));
        }

      const clap_plugin * plugin =
        factory->create_plugin (factory, &host, plugin_info->id);
      if (plugin == nullptr)
        {
          entry->deinit ();
          return std::unexpected (
            utils::Utf8String::from_utf8_encoded_string (
              fmt::format (
                "Failed to create CLAP plugin '{}'", plugin_info->name)));
        }

      if (!plugin->init (plugin))
        {
          plugin->destroy (plugin);
          entry->deinit ();
          return std::unexpected (
            utils::Utf8String::from_utf8_encoded_string (
              fmt::format (
                "CLAP plugin initialization failed for '{}'",
                plugin_info->name)));
        }

      const auto features = clap_feature_strings (plugin_info->features);
      const bool is_instrument =
        features_contain (features, CLAP_PLUGIN_FEATURE_INSTRUMENT);

      auto descriptor = std::make_unique<PluginDescriptor> ();
      descriptor->path_or_id_ = identifier.to_path ();
      descriptor->last_file_mod_time_ms_ = modification_time.toMilliseconds ();
      descriptor->last_info_update_time_ms_ = current_time_ms ();
      descriptor->protocol_ = Protocol::ProtocolType::CLAP;
      descriptor->name_ =
        utils::Utf8String::from_utf8_encoded_string (plugin_info->name);
      descriptor->author_ = utils::Utf8String::from_utf8_encoded_string (
        plugin_info->vendor != nullptr ? plugin_info->vendor : "");
      descriptor
        ->descriptive_name_ = utils::Utf8String::from_utf8_encoded_string (
        plugin_info->description != nullptr ? plugin_info->description : "");
      descriptor->version_ = utils::Utf8String::from_utf8_encoded_string (
        plugin_info->version != nullptr ? plugin_info->version : "");
      descriptor->unique_id_ =
        get_hash_for_range (std::string_view{ plugin_info->id });
      descriptor->category_ =
        is_instrument ? PluginCategory::Instrument : PluginCategory::None;
      descriptor->category_str_ =
        features.size () > 1
          ? utils::Utf8String::from_utf8_encoded_string (features[1])
          : utils::Utf8String::from_utf8_encoded_string (
              features.empty () ? "" : features[0]);
      const auto num_audio_ins = audio_channel_count (plugin, true);
      const auto num_audio_outs = audio_channel_count (plugin, false);
      const auto num_midi_ins = note_port_count (plugin, true);
      const auto num_midi_outs = note_port_count (plugin, false);
      if (
        !num_audio_ins.has_value () || !num_audio_outs.has_value ()
        || !num_midi_ins.has_value () || !num_midi_outs.has_value ())
        {
          plugin->destroy (plugin);
          entry->deinit ();
          return std::unexpected (
            !num_audio_ins    ? num_audio_ins.error ()
            : !num_audio_outs ? num_audio_outs.error ()
            : !num_midi_ins
              ? num_midi_ins.error ()
              : num_midi_outs.error ());
        }

      descriptor->num_audio_ins_ = *num_audio_ins;
      descriptor->num_audio_outs_ = *num_audio_outs;
      descriptor->num_midi_ins_ = *num_midi_ins;
      descriptor->num_midi_outs_ = *num_midi_outs;
      descriptors.push_back (std::move (descriptor));

      plugin->destroy (plugin);
    }

  entry->deinit ();
  return descriptors;
}

bool
CLAPPluginFormat::plugin_needs_rescanning (
  const PluginDescriptor &descriptor) const
{
  return effective_modification_time (
           juce::File (descriptor.source_identifier ().to_juce_string ()))
           .toMilliseconds ()
         != descriptor.last_file_mod_time_ms_;
}

bool
CLAPPluginFormat::plugin_exists (const PluginDescriptor &descriptor) const
{
  return std::filesystem::exists (descriptor.source_identifier ().to_path ());
}

bool
CLAPPluginFormat::scan_uses_message_thread (const utils::Utf8String &) const
{
  return true;
}

} // namespace zrythm::plugins
