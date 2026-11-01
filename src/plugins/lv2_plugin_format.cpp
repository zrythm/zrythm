// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// LV2 discovery implemented against lilv (metadata-only: bundle ttl is
// parsed, but plugin binaries are never loaded during scanning).

#include "zrythm-config.h"

#include <ranges>

#include <fmt/std.h>

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
#if defined(__APPLE__)
  // inside a macOS app bundle, currentApplicationFile is the bundle and
  // the spec bundles live in its Resources directory
  const auto app_file =
    juce::File::getSpecialLocation (juce::File::currentApplicationFile);
  if (app_file.isDirectory ())
    {
      return utils::Utf8String::from_juce_string (
               app_file.getChildFile ("Contents/Resources/lv2-specs")
                 .getFullPathName ())
        .to_path ();
    }
#endif
  // the executable lives in <prefix>/bin (both in the build tree's
  // products/bin and in an installed prefix), so two levels up is the
  // prefix
  const auto prefix =
    juce::File::getSpecialLocation (juce::File::currentExecutableFile)
      .getParentDirectory ()
      .getParentDirectory ();
  return utils::Utf8String::from_juce_string (
           prefix.getChildFile (DATADIR_NAME "/zrythm/lv2-specs")
             .getFullPathName ())
    .to_path ();
}

Lv2PluginFormat::Lv2PluginFormat (std::shared_ptr<Lv2World> world)
    : world_ (std::move (world))
{
}

Lv2PluginFormat::~Lv2PluginFormat () = default;

Protocol::ProtocolType
Lv2PluginFormat::protocol () const
{
  return Protocol::ProtocolType::LV2;
}

utils::Utf8String
Lv2PluginFormat::format_name () const
{
  return u8"LV2";
}

bool
Lv2PluginFormat::file_might_contain_this_plugin_type (
  const utils::Utf8String &identifier) const
{
  return identifier.to_qstring ().endsWith (
    QStringLiteral (".lv2"), Qt::CaseInsensitive);
}

std::vector<utils::Utf8String>
Lv2PluginFormat::search_identifiers (
  const utils::FilePathList &search_paths,
  bool                       recursive) const
{
  std::vector<utils::Utf8String> results;
  std::error_code                ec;
  for (const auto &search_path : search_paths)
    {
      for (
        const auto &entry :
        std::filesystem::directory_iterator (search_path, ec))
        {
          const auto path = entry.path ();
          const auto path_string = utils::Utf8String::from_path (path);
          if (file_might_contain_this_plugin_type (path_string))
            {
              results.push_back (
                utils::Utf8String::from_path (
                  canonical_bundle_path (path_string.to_juce_string ())));
            }
          else if (recursive && entry.is_directory (ec))
            {
              utils::FilePathList nested_paths;
              nested_paths.add_path (path);
              const auto nested_results =
                search_identifiers (nested_paths, true);
              results.insert (
                results.end (), nested_results.begin (), nested_results.end ());
            }
        }
    }
  if (ec)
    {
      z_warning ("LV2: failed to search paths: {}", ec.message ());
    }
  return results;
}

std::expected<std::vector<std::unique_ptr<PluginDescriptor>>, utils::Utf8String>
Lv2PluginFormat::scan_identifier (const utils::Utf8String &identifier)
{
  if (!file_might_contain_this_plugin_type (identifier))
    return std::vector<std::unique_ptr<PluginDescriptor>>{};

  const auto bundle_path = canonical_bundle_path (identifier.to_juce_string ());
  const auto plugin_infos = get_plugins_in_bundle (*world_, bundle_path);
  if (plugin_infos.empty ())
    return std::vector<std::unique_ptr<PluginDescriptor>>{};

  const auto modification_time = effective_modification_time (
    juce::File (utils::Utf8String::from_path (bundle_path).to_juce_string ()));
  const auto info_update_time = juce::Time::getCurrentTime ().toMilliseconds ();

  std::vector<std::unique_ptr<PluginDescriptor>> descriptors;
  for (const auto &info : plugin_infos)
    {
      auto descriptor = std::make_unique<PluginDescriptor> ();
      // Keyed by the canonical bundle path (the scan identifier); the plugin
      // URI is folded into the unique ID
      descriptor->path_or_id_ = bundle_path;
      descriptor->last_file_mod_time_ms_ = modification_time.toMilliseconds ();
      descriptor->last_info_update_time_ms_ = info_update_time;
      descriptor->protocol_ = Protocol::ProtocolType::LV2;
      descriptor->name_ = info.name_;
      descriptor->author_ = info.author_;
      descriptor->version_ = info.version_;
      descriptor->unique_id_ = get_hash_for_range (info.uri_.view ());
      descriptor->juce_compat_deprecated_unique_id_ =
        static_cast<int> (descriptor->unique_id_);
      descriptor->category_ =
        info.is_instrument_ ? PluginCategory::Instrument : PluginCategory::None;
      descriptor->category_str_ =
        info.category_str_.empty ()
          ? (info.is_instrument_
               ? utils::Utf8String{ u8"Instrument" }
               : utils::Utf8String{ u8"Effect" })
          : info.category_str_;
      descriptor->num_audio_ins_ = info.num_audio_ins_;
      descriptor->num_audio_outs_ = info.num_audio_outs_;
      descriptor->num_midi_ins_ = info.num_midi_ins_;
      descriptor->num_midi_outs_ = info.num_midi_outs_;
      descriptor->num_ctrl_ins_ = info.num_ctrl_ins_;
      descriptor->num_ctrl_outs_ = info.num_ctrl_outs_;
      descriptor->num_cv_ins_ = info.num_cv_ins_;
      descriptor->num_cv_outs_ = info.num_cv_outs_;
      descriptor->has_custom_ui_ = info.has_custom_ui_;
      descriptors.push_back (std::move (descriptor));
    }

  return descriptors;
}

bool
Lv2PluginFormat::plugin_needs_rescanning (
  const PluginDescriptor &descriptor) const
{
  return effective_modification_time (
           juce::File (descriptor.source_identifier ().to_juce_string ()))
           .toMilliseconds ()
         != descriptor.last_file_mod_time_ms_;
}

bool
Lv2PluginFormat::plugin_exists (const PluginDescriptor &descriptor) const
{
  return std::filesystem::exists (descriptor.source_identifier ().to_path ());
}

bool
Lv2PluginFormat::scan_uses_message_thread (const utils::Utf8String &) const
{
  return false;
}

} // namespace zrythm::plugins
