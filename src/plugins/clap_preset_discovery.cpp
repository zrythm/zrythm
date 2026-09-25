// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>
#include <unordered_map>
#include <utility>

#include "plugins/clap_preset_discovery.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"
#include "utils/views.h"

#include <QCollator>

namespace zrythm::plugins::clap_preset_discovery
{

namespace
{

/**
 * Cached crawl results keyed by "<library path><kSep><plugin id>";
 * main thread only (see collect_presets_cached).
 */
std::unordered_map<std::string, std::vector<Plugin::PresetEntry>> preset_cache;

/** Separator between the id's three fields. */
constexpr char kSep = '\x1f';

/** Maximum number of presets collected from a single plugin library. */
constexpr size_t kMaxPresetsPerLibrary = 10000;

/**
 * @brief Stops preset collection once a library's preset maximum is
 * reached.
 *
 * A provider pointing at a very large or broken directory would otherwise
 * grow the collection without bound; past the maximum, begin_preset()
 * refuses new presets so the provider stops reporting, and one warning is
 * logged per library.
 */
struct PresetCollectionLimit
{
  /** Presets collected from the library so far. */
  size_t collected_count = 0;

  /** Whether the limit warning was already logged for this library. */
  bool warning_logged = false;
};

std::string
ascii_lower (std::string_view str)
{
  std::string result (str);
  std::transform (
    result.begin (), result.end (), result.begin (),
    [] (unsigned char c) { return static_cast<char> (std::tolower (c)); });
  return result;
}

bool
ascii_iequals (std::string_view a, std::string_view b)
{
  return std::ranges::equal (a, b, [] (char ca, char cb) {
    return std::tolower (static_cast<unsigned char> (ca))
           == std::tolower (static_cast<unsigned char> (cb));
  });
}

/** Declarations gathered from one provider during its init(). */
struct IndexerCtx
{
  /** Lower-cased file extensions; an empty entry matches every file. */
  std::vector<std::string> extensions;

  /**
   * A declared location with its strings deep-copied: providers may reuse
   * the backing storage of the strings they pass to declare_location, so
   * only copies taken during the callback are safe to use afterwards.
   */
  struct DeclaredLocation
  {
    uint32_t    kind;
    std::string name;
    std::string location;
  };
  std::vector<DeclaredLocation> locations;
};

struct ReceiverCtx
{
  struct PendingPreset
  {
    std::string name;
    std::string load_key;

    /** Whether any plugin id was declared for the preset. */
    bool declares_plugin_ids = false;

    /** Whether one of the declared ids is the hosted plugin. */
    bool matches_plugin = false;
  };

  PendingPreset pending;
  bool          has_pending = false;

  /** Location fields the pending preset is being read from. */
  uint32_t    kind = CLAP_PRESET_DISCOVERY_LOCATION_FILE;
  std::string location;
  /** Path (or placeholder) used in log messages. */
  std::string source;
  QString     group;

  std::string_view plugin_id;

  /** Shared per-library limit (see PresetCollectionLimit). */
  PresetCollectionLimit * preset_limit = nullptr;

  std::vector<Plugin::PresetEntry> entries;
};

void
finalize_pending_preset (ReceiverCtx &ctx)
{
  if (!ctx.has_pending)
    return;
  ctx.has_pending = false;

  if (ctx.pending.declares_plugin_ids && !ctx.pending.matches_plugin)
    return;

  ctx.entries.push_back (
    Plugin::PresetEntry{
      utils::Utf8String::from_utf8_encoded_string (ctx.pending.name).to_qstring (),
      ctx.group,
      encode_preset_id (
        PresetLocation{ ctx.kind, ctx.location, ctx.pending.load_key }),
    });
  ++ctx.preset_limit->collected_count;
}

bool
file_matches_declared_extensions (
  const std::vector<std::string> &extensions,
  const std::filesystem::path    &path)
{
  auto file_extension =
    ascii_lower (utils::Utf8String::from_path (path.extension ()).str ());
  // std::filesystem keeps the leading dot; the CLAP filetype declaration
  // does not
  if (!file_extension.empty () && file_extension.front () == '.')
    {
      file_extension.erase (0, 1);
    }
  return std::ranges::any_of (extensions, [&file_extension] (const auto &ext) {
    return ext.empty () || ext == file_extension;
  });
}

/** Reads the metadata of one location (a single file or the plugin
 * itself) through the provider. */
void
read_metadata (
  const clap_preset_discovery_provider * provider,
  PresetLocation                         preset_location,
  const QString                         &group,
  std::string_view                       plugin_id,
  PresetCollectionLimit                 &preset_limit,
  std::vector<Plugin::PresetEntry>      &out)
{
  ReceiverCtx receiver_ctx;
  receiver_ctx.kind = preset_location.kind;
  receiver_ctx.location = std::move (preset_location.location);
  receiver_ctx.source =
    receiver_ctx.location.empty () ? "<embedded in plugin>" : receiver_ctx.location;
  receiver_ctx.group = group;
  receiver_ctx.plugin_id = plugin_id;
  receiver_ctx.preset_limit = &preset_limit;

  clap_preset_discovery_metadata_receiver_t receiver{};
  receiver.receiver_data = &receiver_ctx;
  receiver.on_error =
    +[] (
       const clap_preset_discovery_metadata_receiver * recv, int32_t,
       const char *                                    error_message) {
      auto &ctx = *static_cast<ReceiverCtx *> (recv->receiver_data);
      z_warning (
        "CLAP preset discovery: error reading '{}': {}", ctx.source,
        error_message != nullptr ? error_message : "");
    };
  receiver.begin_preset =
    +[] (
       const clap_preset_discovery_metadata_receiver * recv, const char * name,
       const char * load_key) -> bool {
    auto &ctx = *static_cast<ReceiverCtx *> (recv->receiver_data);
    finalize_pending_preset (ctx);
    auto &limit = *ctx.preset_limit;
    if (limit.collected_count >= kMaxPresetsPerLibrary)
      {
        if (!limit.warning_logged)
          {
            limit.warning_logged = true;
            z_warning (
              "CLAP preset discovery: refusing presets past {} collected "
              "from one library",
              kMaxPresetsPerLibrary);
          }
        return false;
      }
    ctx.pending = ReceiverCtx::PendingPreset{
      name != nullptr ? name : "", load_key != nullptr ? load_key : "", false,
      false
    };
    ctx.has_pending = true;
    return true;
  };
  receiver.add_plugin_id =
    +[] (
       const clap_preset_discovery_metadata_receiver * recv,
       const clap_universal_plugin_id_t *              id) {
      if (id == nullptr || id->abi == nullptr || id->id == nullptr)
        return;
      auto &ctx = *static_cast<ReceiverCtx *> (recv->receiver_data);
      ctx.pending.declares_plugin_ids = true;
      if (ascii_iequals (id->abi, "clap") && id->id == ctx.plugin_id)
        ctx.pending.matches_plugin = true;
    };
  receiver.set_soundpack_id =
    +[] (const clap_preset_discovery_metadata_receiver *, const char *) { };
  receiver.set_flags =
    +[] (const clap_preset_discovery_metadata_receiver *, uint32_t) { };
  receiver.add_creator =
    +[] (const clap_preset_discovery_metadata_receiver *, const char *) { };
  receiver.set_description =
    +[] (const clap_preset_discovery_metadata_receiver *, const char *) { };
  receiver.set_timestamps =
    +[] (
       const clap_preset_discovery_metadata_receiver *, clap_timestamp,
       clap_timestamp) { };
  receiver.add_feature =
    +[] (const clap_preset_discovery_metadata_receiver *, const char *) { };
  receiver.add_extra_info =
    +[] (
       const clap_preset_discovery_metadata_receiver *, const char *,
       const char *) { };

  const char * location_arg =
    receiver_ctx.location.empty () ? nullptr : receiver_ctx.location.c_str ();
  if (!provider->get_metadata (
        provider, preset_location.kind, location_arg, &receiver))
    {
      z_debug (
        "CLAP preset discovery: provider returned no metadata for '{}'",
        receiver_ctx.source);
    }

  finalize_pending_preset (receiver_ctx);
  out.insert (
    out.end (), std::make_move_iterator (receiver_ctx.entries.begin ()),
    std::make_move_iterator (receiver_ctx.entries.end ()));
}

/** Crawls one declared location and reads every matching preset file. */
void
process_declared_location (
  const clap_preset_discovery_provider * provider,
  const std::vector<std::string>        &extensions,
  const IndexerCtx::DeclaredLocation    &declared,
  std::string_view                       plugin_id,
  PresetCollectionLimit                 &preset_limit,
  std::vector<Plugin::PresetEntry>      &out)
{
  const QString group =
    utils::Utf8String::from_utf8_encoded_string (declared.name).to_qstring ();

  if (declared.kind == CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN)
    {
      read_metadata (
        provider, PresetLocation{ declared.kind, "", "" }, group, plugin_id,
        preset_limit, out);
      return;
    }

  if (extensions.empty ())
    {
      z_debug (
        "CLAP preset discovery: provider declared no filetypes; skipping "
        "location '{}'",
        declared.location);
      return;
    }

  const std::filesystem::path root{ declared.location };
  std::error_code             ec;

  if (std::filesystem::is_regular_file (root, ec))
    {
      if (file_matches_declared_extensions (extensions, root))
        {
          read_metadata (
            provider, PresetLocation{ declared.kind, declared.location, "" },
            group, plugin_id, preset_limit, out);
        }
      return;
    }

  if (!std::filesystem::is_directory (root, ec))
    {
      z_warning (
        "CLAP preset discovery: location '{}' does not exist",
        declared.location);
      return;
    }

  auto it = std::filesystem::recursive_directory_iterator (
    root, std::filesystem::directory_options::skip_permission_denied, ec);
  for (
    const auto end = std::filesystem::recursive_directory_iterator ();
    it != end; it.increment (ec))
    {
      if (ec)
        {
          z_warning (
            "CLAP preset discovery: failed to crawl '{}': {}",
            declared.location, ec.message ());
          break;
        }
      std::error_code entry_ec;
      if (!it->is_regular_file (entry_ec) || entry_ec)
        continue;
      const auto &path = it->path ();
      if (!file_matches_declared_extensions (extensions, path))
        continue;
      read_metadata (
        provider,
        PresetLocation{
          declared.kind, utils::Utf8String::from_path (path).str (), "" },
        group, plugin_id, preset_limit, out);
    }
}

} // namespace

QString
encode_preset_id (const PresetLocation &location)
{
  std::string encoded;
  encoded += location.kind == CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN ? 'P' : 'F';
  encoded += kSep;
  encoded += location.location;
  encoded += kSep;
  encoded += location.load_key;
  return utils::Utf8String::from_utf8_encoded_string (encoded).to_qstring ();
}

std::optional<PresetLocation>
decode_preset_id (const QString &id)
{
  const auto parts = id.split (QChar (kSep));
  if (parts.size () != 3 || (parts[0] != u'F' && parts[0] != u'P'))
    return std::nullopt;

  return PresetLocation{
    parts[0] == u'P'
      ? CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN
      : CLAP_PRESET_DISCOVERY_LOCATION_FILE,
    utils::Utf8String::from_qstring (parts[1]).str (),
    utils::Utf8String::from_qstring (parts[2]).str ()
  };
}

std::vector<Plugin::PresetEntry>
collect_presets (const clap_plugin_entry &entry, std::string_view plugin_id)
{
  std::vector<Plugin::PresetEntry> result;

  const auto * factory = static_cast<const clap_preset_discovery_factory *> (
    entry.get_factory (CLAP_PRESET_DISCOVERY_FACTORY_ID));
  if (factory == nullptr)
    {
      factory = static_cast<const clap_preset_discovery_factory *> (
        entry.get_factory (CLAP_PRESET_DISCOVERY_FACTORY_ID_COMPAT));
    }
  if (factory == nullptr)
    return result;

  PresetCollectionLimit preset_limit;

  for (const auto i : std::views::iota (0u, factory->count (factory)))
    {
      const auto * desc = factory->get_descriptor (factory, i);
      if (desc == nullptr || desc->id == nullptr)
        continue;
      if (!clap_version_is_compatible (desc->clap_version))
        {
          z_warning (
            "CLAP preset discovery: skipping provider '{}' with incompatible "
            "CLAP version {}.{}.{}",
            desc->id, desc->clap_version.major, desc->clap_version.minor,
            desc->clap_version.revision);
          continue;
        }
      IndexerCtx                      indexer_ctx;
      clap_preset_discovery_indexer_t indexer{};
      indexer.clap_version = CLAP_VERSION;
      indexer.name = "Zrythm";
      indexer.indexer_data = &indexer_ctx;
      indexer.declare_filetype =
        +[] (
           const clap_preset_discovery_indexer *    idx,
           const clap_preset_discovery_filetype_t * filetype) -> bool {
        if (filetype == nullptr)
          return false;
        auto &ctx = static_cast<IndexerCtx &> (
          *static_cast<IndexerCtx *> (idx->indexer_data));
        ctx.extensions.push_back (
          filetype->file_extension != nullptr
            ? ascii_lower (filetype->file_extension)
            : std::string{});
        return true;
      };
      indexer.declare_location =
        +[] (
           const clap_preset_discovery_indexer *    idx,
           const clap_preset_discovery_location_t * location) -> bool {
        if (location == nullptr || location->name == nullptr)
          return false;
        if (
          location->kind == CLAP_PRESET_DISCOVERY_LOCATION_FILE
          && location->location == nullptr)
          return false;
        auto &ctx = static_cast<IndexerCtx &> (
          *static_cast<IndexerCtx *> (idx->indexer_data));
        ctx.locations.push_back (
          IndexerCtx::DeclaredLocation{
            location->kind, location->name,
            location->location != nullptr ? location->location : "" });
        return true;
      };
      indexer.declare_soundpack =
        +[] (
           const clap_preset_discovery_indexer *     idx,
           const clap_preset_discovery_soundpack_t * soundpack) -> bool {
        return soundpack != nullptr;
      };
      indexer.get_extension =
        +[] (const clap_preset_discovery_indexer *, const char *)
        -> const void * { return nullptr; };

      const auto * provider = factory->create (factory, &indexer, desc->id);
      if (provider == nullptr)
        {
          z_warning (
            "CLAP preset discovery: failed to create provider '{}'", desc->id);
          continue;
        }

      if (provider->init (provider))
        {
          for (const auto &declared : indexer_ctx.locations)
            {
              process_declared_location (
                provider, indexer_ctx.extensions, declared, plugin_id,
                preset_limit, result);
            }
        }
      else
        {
          z_warning (
            "CLAP preset discovery: provider '{}' failed to initialize",
            desc->id);
        }

      provider->destroy (provider);
    }

  const auto id_of = [] (const Plugin::PresetEntry &preset) {
    const auto * id = std::get_if<QString> (&preset.id);
    return id != nullptr ? *id : QString{};
  };
  // Two providers may report the same preset (same encoded location);
  // keep one copy of each, preferring the first provider's report
  std::ranges::stable_sort (result, [&id_of] (const auto &a, const auto &b) {
    return id_of (a) < id_of (b);
  });
  const auto [first_duplicate, last_duplicate] =
    std::ranges::unique (result, [&id_of] (const auto &a, const auto &b) {
      return id_of (a) == id_of (b);
    });
  result.erase (first_duplicate, last_duplicate);
  const QCollator collator;
  std::ranges::stable_sort (
    result, [&id_of, &collator] (const auto &a, const auto &b) {
      const int name_comparison = collator.compare (a.name, b.name);
      if (name_comparison != 0)
        return name_comparison < 0;
      return id_of (a) < id_of (b);
    });
  return result;
}

std::vector<Plugin::PresetEntry>
collect_presets_cached (
  const std::filesystem::path &library_path,
  const clap_plugin_entry     &entry,
  std::string_view             plugin_id)
{
  // Keyed by library path and plugin id; the cached entries are value
  // copies, so nothing references the library or its providers
  // afterwards
  std::string key = utils::Utf8String::from_path (library_path).str ();
  key += kSep;
  key += plugin_id;

  const auto it = preset_cache.find (key);
  if (it != preset_cache.end ())
    return it->second;

  auto entries = collect_presets (entry, plugin_id);
  return preset_cache.emplace (std::move (key), std::move (entries))
    .first->second;
}

void
clear_preset_cache ()
{
  preset_cache.clear ();
}

} // namespace zrythm::plugins::clap_preset_discovery
