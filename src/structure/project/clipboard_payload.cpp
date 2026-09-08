// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cctype>
#include <ranges>

#include "utils/format_qt.h"

#include "dsp/parameter.h"
#include "structure/arrangement/arranger_object_fwd.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/project/clipboard_json_schema.h"
#include "structure/project/clipboard_payload.h"
#include "structure/tracks/channel_send.h"
#include "utils/compression.h"
#include "utils/exceptions.h"
#include "utils/logger.h"
#include "utils/serialization.h"

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <nlohmann/json-schema.hpp>

using namespace std::string_view_literals;

namespace zrythm::structure::project
{

using ObjectCategory = ProjectRegistry::ObjectCategory;

namespace
{

using IdMap = boost::unordered_flat_map<QUuid, QUuid>;
using IdSet = boost::unordered_flat_set<QUuid>;

/** Index of type T (pointer) in a variant of pointers.
 *
 * The serialized "variantType" field of each registry entry stored in a
 * variant is this index, so reordering such a variant silently corrupts
 * every stored payload — such a reorder must bump
 * ClipboardPayload::kFormatVersion (and the project schema's variant
 * handling). */
template <typename T, typename Variant> struct ptr_variant_index;
template <typename T, typename... Ts>
struct ptr_variant_index<T, std::variant<Ts...>>
{
  static constexpr std::size_t value = [] {
    std::size_t i = 0;
    std::size_t found = sizeof...(Ts);
    (void) ((std::is_same_v<T *, Ts> ? (found = i, true) : (++i, false)) || ...);
    return found;
  }();
  static_assert (value < sizeof...(Ts), "Type not found in variant");
};

/** Strict check for the QUuid::toString(QUuid::WithoutBraces) format
 * (8-4-4-4-12 hex digits). Used to avoid mistaking arbitrary strings for
 * UUID references. */
bool
is_uuid_string (std::string_view str)
{
  if (str.size () != 36)
    return false;
  for (const auto i : std::views::iota (0uz, str.size ()))
    {
      if (i == 8 || i == 13 || i == 18 || i == 23)
        {
          if (str[i] != '-')
            return false;
        }
      else if (!std::isxdigit (static_cast<unsigned char> (str[i])))
        return false;
    }
  return true;
}

QUuid
to_uuid (std::string_view str)
{
  return QUuid::fromString (
    QString::fromUtf8 (str.data (), static_cast<qsizetype> (str.size ())));
}

std::string
to_std_string (const QUuid &id)
{
  return id.toString (QUuid::WithoutBraces).toStdString ();
}

/** UUID of a bucket entry (its "id" member). */
QUuid
entry_id (const nlohmann::json &entry)
{
  return to_uuid (entry.at ("id").get<std::string> ());
}

/**
 * @brief Reference keys that may point outside the owned subgraph.
 *
 * References under these keys are not followed when collecting the closure:
 * if the target is part of the copied roots it is included on its own and
 * remapped consistently; otherwise the reference is kept (same-project
 * paste) or cleared by filtered_for_target() (cross-project paste).
 */
bool
is_boundary_key (std::string_view key)
{
  return key == tracks::ChannelSend::kDestinationPortKey
         || key == dsp::ProcessorParameter::kModulationSourcePortIdKey;
}

/** Collects UUID references found anywhere in @p j that resolve via @p
 * resolves, except under boundary keys unless @p include_boundary_refs.
 *
 * Only string values are harvested: UUIDs appearing as object KEYS (e.g.
 * routing maps keyed by port id) are not collected. This is safe while
 * every such key also appears as a value somewhere in the closure
 * (rewrite_uuid_strings renames mapped keys, so keys never dangle) — new
 * metadata that references objects ONLY as keys must add a value-form
 * reference or extend this walk. */
template <typename Resolver>
void
collect_referenced_uuids (
  const nlohmann::json &j,
  Resolver              resolves,
  std::vector<QUuid>   &out,
  bool                  include_boundary_refs = false)
{
  auto try_add = [&] (const nlohmann::json &value) {
    const auto &str = value.get_ref<const std::string &> ();
    if (!is_uuid_string (str))
      return;
    auto id = to_uuid (str);
    if (resolves (id))
      out.push_back (id);
  };

  if (j.is_object ())
    {
      for (const auto &[key, value] : j.items ())
        {
          if (value.is_string ())
            {
              if (include_boundary_refs || !is_boundary_key (key))
                try_add (value);
            }
          else
            {
              collect_referenced_uuids (
                value, resolves, out, include_boundary_refs);
            }
        }
    }
  else if (j.is_array ())
    {
      for (const auto &el : j)
        {
          if (el.is_string ())
            try_add (el);
          else
            collect_referenced_uuids (el, resolves, out, include_boundary_refs);
        }
    }
}

/** Whether @p j contains a UUID string value (outside boundary keys)
 * that @p ids does not contain. Traversal matches
 * collect_referenced_uuids(): object keys are not references. */
template <typename IdSetT>
bool
has_dangling_reference (const nlohmann::json &j, const IdSetT &ids)
{
  auto value_ok = [&] (const nlohmann::json &value) {
    const auto &str = value.get_ref<const std::string &> ();
    return !is_uuid_string (str) || ids.contains (to_uuid (str));
  };

  if (j.is_object ())
    {
      for (const auto &[key, value] : j.items ())
        {
          if (value.is_string ())
            {
              if (!is_boundary_key (key) && !value_ok (value))
                return true;
            }
          else if (has_dangling_reference (value, ids))
            return true;
        }
    }
  else if (j.is_array ())
    {
      for (const auto &el : j)
        {
          if (el.is_string ())
            {
              if (!value_ok (el))
                return true;
            }
          else if (has_dangling_reference (el, ids))
            return true;
        }
    }
  return false;
}

/** Replaces every UUID string found in @p j (values and object keys) per
 * @p id_map. */
void
rewrite_uuid_strings (nlohmann::json &j, const IdMap &id_map)
{
  auto rewrite = [&] (nlohmann::json &value) {
    const auto &str = value.get_ref<const std::string &> ();
    if (!is_uuid_string (str))
      return;
    if (const auto it = id_map.find (to_uuid (str)); it != id_map.end ())
      value = to_std_string (it->second);
  };

  if (j.is_object ())
    {
      // rename keys that are mapped UUIDs (e.g. routing maps)
      if (std::ranges::any_of (j.items (), [&] (const auto &item) {
            return is_uuid_string (item.key ())
                   && id_map.contains (to_uuid (item.key ()));
          }))
        {
          auto new_obj = nlohmann::json::object ();
          for (auto it = j.begin (); it != j.end (); ++it)
            {
              auto key = it.key ();
              if (is_uuid_string (key))
                {
                  if (
                    const auto map_it = id_map.find (to_uuid (key));
                    map_it != id_map.end ())
                    key = to_std_string (map_it->second);
                }
              new_obj[std::move (key)] = std::move (it.value ());
            }
          j = std::move (new_obj);
        }

      for (auto &value : j)
        {
          if (value.is_string ())
            rewrite (value);
          else
            rewrite_uuid_strings (value, id_map);
        }
    }
  else if (j.is_array ())
    {
      for (auto &el : j)
        {
          if (el.is_string ())
            rewrite (el);
          else
            rewrite_uuid_strings (el, id_map);
        }
    }
}

/** Removes every reference to a UUID in @p drop_set from @p j: matching
 * array elements are removed and matching object members (or members with
 * matching keys) are erased. */
void
drop_uuid_references (nlohmann::json &j, const IdSet &drop_set)
{
  auto is_dropped = [&] (const nlohmann::json &value) {
    const auto &str = value.get_ref<const std::string &> ();
    return is_uuid_string (str) && drop_set.contains (to_uuid (str));
  };

  if (j.is_object ())
    {
      for (auto it = j.begin (); it != j.end ();)
        {
          const bool key_dropped =
            is_uuid_string (it.key ())
            && drop_set.contains (to_uuid (it.key ()));
          const bool value_dropped =
            it.value ().is_string () && is_dropped (it.value ());
          if (key_dropped || value_dropped)
            it = j.erase (it);
          else
            {
              drop_uuid_references (it.value (), drop_set);
              ++it;
            }
        }
    }
  else if (j.is_array ())
    {
      // rebuild instead of erasing in place: nlohmann erasure shifts the
      // whole tail, which is quadratic over large arrays
      nlohmann::json kept = nlohmann::json::array ();
      for (auto &el : j)
        {
          if (el.is_string () && is_dropped (el))
            continue;
          drop_uuid_references (el, drop_set);
          kept.push_back (std::move (el));
        }
      j = std::move (kept);
    }
}

/** Clears boundary references whose targets don't resolve (see
 * is_boundary_key()): send destinations are erased (their deserialization
 * is guarded), modulation sources are set to null (their deserialization is
 * unguarded). */
template <typename ResolvableFunc>
void
clear_unresolvable_external_refs (
  nlohmann::json &j,
  ResolvableFunc  resolvable,
  std::size_t    &severed_count)
{
  if (j.is_object ())
    {
      for (auto it = j.begin (); it != j.end ();)
        {
          if (is_boundary_key (it.key ()) && it.value ().is_string ())
            {
              const auto &str = it.value ().get_ref<const std::string &> ();
              if (is_uuid_string (str) && !resolvable (to_uuid (str)))
                {
                  ++severed_count;
                  if (
                    it.key ()
                    == dsp::ProcessorParameter::kModulationSourcePortIdKey)
                    {
                      it.value () = nullptr;
                      ++it;
                    }
                  else
                    {
                      it = j.erase (it);
                    }
                  continue;
                }
            }
          clear_unresolvable_external_refs (
            it.value (), resolvable, severed_count);
          ++it;
        }
    }
  else if (j.is_array ())
    {
      for (auto &el : j)
        clear_unresolvable_external_refs (el, resolvable, severed_count);
    }
}

constexpr auto kPayloadTypeKey = "payloadType";
constexpr auto kSourceProjectIdKey = "sourceProjectId";
constexpr auto kMetadataKey = "metadata";
constexpr auto kRegistryKey = "registry";
constexpr auto kRootsKey = "roots";
constexpr auto kFormatVersionKey = "formatVersion";

// Payload type names as serialized in the payloadType field (also part of
// the schema's enum — kept in sync manually)
constexpr auto kPayloadTypeArrangerObjects = "arrangerObjects"sv;
constexpr auto kPayloadTypeTracks = "tracks"sv;
constexpr auto kPayloadTypePlugins = "plugins"sv;

/** Upper bound on decoded clipboard payload size: clipboard text comes
 * from other processes, so larger frames are rejected before any
 * allocation. Decode runs on the UI thread, so this bounds the worst
 * case decode work (together with ClipboardPayload::
 * kMaxClipboardTextLength bounding the input). */
constexpr size_t kMaxDecodedPayloadSize = 64ULL * 1024 * 1024;

/** Maximum JSON nesting depth accepted when parsing clipboard text.
 * Deeply nested JSON would overflow the stack during parsing and the
 * recursive walks over the parsed document. */
constexpr int kMaxJsonDepth = 512;

nlohmann::json
parse_json_depth_capped (std::string_view text)
{
  const auto depth_callback =
    [] (
      int depth, nlohmann::json::parse_event_t event,
      const nlohmann::json & /*parsed*/) {
      if (
        depth > kMaxJsonDepth
        && (event == nlohmann::json::parse_event_t::object_start || event == nlohmann::json::parse_event_t::array_start))
        {
          throw ZrythmException (
            fmt::format ("JSON nesting deeper than {}", kMaxJsonDepth));
        }
      return true;
    };
  return nlohmann::json::parse (text, depth_callback);
}

/** Lazy-initialized validator for the embedded clipboard schema.
 * Validation is what makes decoded payloads trustworthy: the schema
 * pins the document structure and requires every bucket entry to carry
 * a UUID "id". */
class ClipboardSchemaValidator
{
public:
  static ClipboardSchemaValidator &get_instance ()
  {
    static ClipboardSchemaValidator instance;
    return instance;
  }

  void validate (const nlohmann::json &j) const { validator_.validate (j); }

private:
  ClipboardSchemaValidator ()
      : validator_ (
          nlohmann::json::parse (kClipboardSchemaJsonStr),
          nullptr,
          nlohmann::json_schema::default_string_format_check,
          nullptr)
  {
  }

  nlohmann::json_schema::json_validator validator_;
};

} // namespace

struct ClipboardPayload::JsonState
{
  nlohmann::json metadata_ = nlohmann::json::object ();
  nlohmann::json registry_json_ = nlohmann::json::object ();
};

ClipboardPayload::ClipboardPayload () : json_ (std::make_unique<JsonState> ())
{
}

ClipboardPayload::ClipboardPayload (const ClipboardPayload &other)
    : ClipboardPayload ()
{
  *json_ = *other.json_;
  type_ = other.type_;
  source_project_id_ = other.source_project_id_;
  roots_ = other.roots_;
}

ClipboardPayload::ClipboardPayload (ClipboardPayload &&other) noexcept = default;

ClipboardPayload &
ClipboardPayload::operator= (const ClipboardPayload &other)
{
  if (this != &other)
    {
      *json_ = *other.json_;
      type_ = other.type_;
      source_project_id_ = other.source_project_id_;
      roots_ = other.roots_;
    }
  return *this;
}

ClipboardPayload &
ClipboardPayload::operator= (ClipboardPayload &&other) noexcept = default;

ClipboardPayload::~ClipboardPayload () = default;

const nlohmann::json &
ClipboardPayload::metadata () const
{
  return json_->metadata_;
}

const nlohmann::json &
ClipboardPayload::registry_json () const
{
  return json_->registry_json_;
}

ClipboardPayload
ClipboardPayload::create (
  const ProjectRegistry    &registry,
  Type                      type,
  const std::vector<QUuid> &roots,
  const QString            &source_project_id)
{
  return create (
    registry, type, roots, source_project_id, nlohmann::json::object ());
}

ClipboardPayload
ClipboardPayload::create (
  const ProjectRegistry    &registry,
  Type                      type,
  const std::vector<QUuid> &roots,
  const QString            &source_project_id,
  nlohmann::json            metadata)
{
  ClipboardPayload payload;
  payload.type_ = type;
  payload.source_project_id_ = source_project_id;
  payload.json_->metadata_ = std::move (metadata);
  payload.roots_ = roots;

  std::unordered_map<ObjectCategory, nlohmann::json> buckets;
  IdSet                                              visited;
  std::vector<QUuid>                                 worklist = roots;

  while (!worklist.empty ())
    {
      const auto id = worklist.back ();
      worklist.pop_back ();
      if (!visited.insert (id).second)
        continue;

      nlohmann::json obj_json;
      const auto category = registry.serialize_object_by_uuid (id, obj_json);
      if (!category.has_value ())
        {
          z_warning ("Clipboard: object {} not found in registry", id);
          continue;
        }
      buckets[*category].push_back (std::move (obj_json));
      collect_referenced_uuids (
        buckets[*category].back (),
        [&registry] (const QUuid &ref_id) { return registry.contains (ref_id); },
        worklist);
    }

  payload.json_->registry_json_ = nlohmann::json::object ();
  for (const auto &[category, key] : ProjectRegistry::kCategoryBucketKeys)
    {
      payload.json_->registry_json_[key] =
        buckets.contains (category)
          ? std::move (buckets[category])
          : nlohmann::json::array ();
    }

  return payload;
}

bool
ClipboardPayload::references_resolve_internally () const
{
  IdSet ids;
  for (const auto &[bucket_key, bucket] : json_->registry_json_.items ())
    {
      for (const auto &entry : bucket)
        ids.insert (entry_id (entry));
    }

  if (
    has_dangling_reference (json_->registry_json_, ids)
    || has_dangling_reference (json_->metadata_, ids))
    return false;

  return std::ranges::all_of (roots_, [&] (const QUuid &root) {
    return ids.contains (root);
  });
}

QString
ClipboardPayload::encode_to_clipboard_text () const
{
  const nlohmann::json j = *this;
  return QString::fromUtf8 (kTextPrefix.data (), kTextPrefix.size ())
         + QString::fromUtf8 (
           utils::compression::compress_to_base64_str (
             QByteArray::fromStdString (j.dump ())));
}

std::optional<ClipboardPayload>
ClipboardPayload::decode_from_clipboard_text (const QString &text)
{
  const auto prefix =
    QString::fromUtf8 (kTextPrefix.data (), kTextPrefix.size ());
  if (!text.startsWith (prefix))
    return std::nullopt;

  // Reject oversized text before any copying or decoding work
  if (text.size () > kMaxClipboardTextLength)
    {
      z_warning (
        "Clipboard: text longer than {} characters ignored",
        kMaxClipboardTextLength);
      return std::nullopt;
    }

  try
    {
      const auto json_text = utils::compression::decompress_string_from_base64 (
        QStringView{ text }.sliced (prefix.size ()).toUtf8 (),
        kMaxDecodedPayloadSize);
      const auto j = parse_json_depth_capped (json_text.c_str ());
      if (
        j.value (utils::serialization::kDocumentTypeKey, std::string{})
        != kDocumentType)
        return std::nullopt;
      const auto format_version = j.value (kFormatVersionKey, 0);
      if (format_version != kFormatVersion)
        {
          z_warning ("Clipboard: unsupported format version {}", format_version);
          return std::nullopt;
        }
      ClipboardSchemaValidator::get_instance ().validate (j);
      auto payload = j.get<ClipboardPayload> ();
      // The schema proves shape, not closure: reject payloads whose
      // references point at objects they do not carry, before they can
      // become null-resolving references in the target registry
      if (!payload.references_resolve_internally ())
        {
          z_warning (
            "Clipboard: payload references objects it does not contain; "
            "refusing to decode");
          return std::nullopt;
        }
      return payload;
    }
  catch (const std::exception &e)
    {
      z_warning ("Clipboard: failed to decode clipboard text: {}", e.what ());
      return std::nullopt;
    }
}

ClipboardPayload
ClipboardPayload::with_regenerated_uuids (ClipboardPayload payload)
{
  IdMap id_map;
  for (const auto &[bucket_key, bucket] : payload.json_->registry_json_.items ())
    {
      // file audio sources are shared assets and keep their identity
      if (bucket_key == ProjectRegistry::kFileAudioSourcesKey)
        continue;
      for (const auto &entry : bucket)
        id_map.emplace (entry_id (entry), QUuid::createUuid ());
    }

  rewrite_uuid_strings (payload.json_->registry_json_, id_map);
  rewrite_uuid_strings (payload.json_->metadata_, id_map);
  for (auto &root : payload.roots_)
    {
      if (const auto it = id_map.find (root); it != id_map.end ())
        root = it->second;
    }
  return payload;
}

FilterResult
ClipboardPayload::filtered_for_target (
  const ProjectRegistry &target_registry) const
{
  FilterResult      result;
  ClipboardPayload &result_payload = result.payload.emplace (*this);

  // File audio sources not present in the target registry cannot produce
  // audio: their frames live in a pool entry the target does not have
  // (always the case cross-project; in the same project it means the
  // source was purged after the copy). Drop them and the audio content
  // that depends on them, so the loss is surfaced instead of pasting
  // silent clips.
  IdSet drop_set;
  for (
    const auto &entry : result_payload.json_->registry_json_.at (
      ProjectRegistry::kFileAudioSourcesKey))
    {
      const auto id = entry_id (entry);
      if (!target_registry.contains (id))
        drop_set.insert (id);
    }
  static constexpr auto kAudioClipIndex = ptr_variant_index<
    arrangement::AudioClip, arrangement::ArrangerObjectPtrVariant>::value;
  static constexpr auto kAudioSourceObjectIndex = ptr_variant_index<
    arrangement::AudioSourceObject, arrangement::ArrangerObjectPtrVariant>::value;
  auto &arranger_bucket = result_payload.json_->registry_json_.at (
    ProjectRegistry::kArrangerObjectsKey);
  for (const auto &entry : arranger_bucket)
    {
      const auto type_index =
        entry.at (utils::serialization::kVariantTypeKey).get<std::size_t> ();
      if (type_index == kAudioSourceObjectIndex)
        {
          if (
            drop_set.contains (
              entry.at (arrangement::AudioSourceObject::kFileAudioSourceKey)
                .get<QUuid> ()))
            {
              drop_set.insert (entry_id (entry));
              ++result.dropped_audio_objects;
            }
        }
    }
  for (const auto &entry : arranger_bucket)
    {
      const auto type_index =
        entry.at (utils::serialization::kVariantTypeKey).get<std::size_t> ();
      if (type_index != kAudioClipIndex)
        continue;
      const auto &sources = entry.at (arrangement::AudioClip::kAudioSourcesKey);
      // A clip with no source objects carries no audio to lose and is
      // kept; a clip whose sources all went stale is dropped
      const bool has_kept_source =
        sources.empty ()
        || std::ranges::any_of (sources, [&] (const nlohmann::json &id_json) {
             return !drop_set.contains (id_json.get<QUuid> ());
           });
      if (!has_kept_source)
        {
          drop_set.insert (entry_id (entry));
          ++result.dropped_audio_objects;
        }
    }

  if (!drop_set.empty ())
    {
      // Remove dropped objects from the buckets, then drop every
      // reference to them (buckets are rebuilt instead of erased in
      // place: nlohmann erasure shifts the whole tail)
      for (
        auto &[bucket_key, bucket] :
        result_payload.json_->registry_json_.items ())
        {
          nlohmann::json kept = nlohmann::json::array ();
          for (auto &entry : bucket)
            {
              if (!drop_set.contains (entry_id (entry)))
                kept.push_back (std::move (entry));
            }
          bucket = std::move (kept);
        }
      drop_uuid_references (result_payload.json_->registry_json_, drop_set);
      drop_uuid_references (result_payload.json_->metadata_, drop_set);
      std::erase_if (result_payload.roots_, [&] (const QUuid &id) {
        return drop_set.contains (id);
      });
      // The anchor is deliberately left as copied: pasted remainders keep
      // their original relative offsets from the earliest copied object,
      // so what survives still lands around the paste position as a group
      // (with the dropped earliest roots simply absent)
    }

  // Clear external references that don't resolve in the target. In the
  // same project this severs references whose targets were deleted after
  // the copy was made.
  IdSet resolvable;
  for (
    const auto &[bucket_key, bucket] :
    result_payload.json_->registry_json_.items ())
    for (const auto &entry : bucket)
      resolvable.insert (entry_id (entry));
  const auto is_resolvable = [&] (const QUuid &id) {
    return resolvable.contains (id) || target_registry.contains (id);
  };
  clear_unresolvable_external_refs (
    result_payload.json_->registry_json_, is_resolvable,
    result.severed_references);
  clear_unresolvable_external_refs (
    result_payload.json_->metadata_, is_resolvable, result.severed_references);

  if (result_payload.roots_.empty ())
    return FilterResult{};
  return result;
}

std::vector<QUuid>
ClipboardPayload::ids_needed_by_roots (const std::vector<QUuid> &roots) const
{
  // Index the payload entries by ID
  boost::unordered_flat_map<QUuid, const nlohmann::json *> entries_by_id;
  for (const auto &[bucket_key, bucket] : json_->registry_json_.items ())
    {
      for (const auto &entry : bucket)
        entries_by_id.emplace (entry_id (entry), &entry);
    }
  const auto is_payload_present = [&entries_by_id] (const QUuid &id) {
    return entries_by_id.contains (id);
  };

  IdSet              needed;
  std::vector<QUuid> worklist = roots;
  while (!worklist.empty ())
    {
      const auto id = worklist.back ();
      worklist.pop_back ();
      if (!needed.insert (id).second)
        continue;

      // Boundary references are followed here on purpose: a kept entry
      // referencing a non-pasted root (e.g. a parameter modulated by a
      // skipped plugin's port) must not be left dangling
      const auto entry_it = entries_by_id.find (id);
      if (entry_it == entries_by_id.end ())
        continue;
      collect_referenced_uuids (
        *entry_it->second, is_payload_present, worklist,
        /*include_boundary_refs=*/true);
    }

  return needed | std::ranges::to<std::vector> ();
}

std::vector<QUuid>
ClipboardPayload::import_into (ProjectRegistry &registry) const
{
  // FileAudioSources already present in the target are shared assets and
  // are skipped instead of imported again. A source missing from the
  // target is imported without audio data, preserving its identity.
  std::vector<QUuid> imported_ids;
  bool               has_shared_file_audio_sources = false;
  for (
    const auto &entry :
    json_->registry_json_.at (ProjectRegistry::kFileAudioSourcesKey))
    {
      const auto id = entry_id (entry);
      if (registry.contains (id))
        has_shared_file_audio_sources = true;
      else
        {
          z_warning (
            "Clipboard: file audio source {} not found in target project; "
            "imported audio will be silent",
            id);
          imported_ids.push_back (id);
        }
    }
  for (const auto &[bucket_key, bucket] : json_->registry_json_.items ())
    {
      if (bucket_key == ProjectRegistry::kFileAudioSourcesKey)
        continue;
      for (const auto &entry : bucket)
        imported_ids.push_back (entry_id (entry));
    }

  // Refuse IDs that already exist in the target before anything is
  // imported: the error cleanup below must never be able to destroy
  // objects that were not imported by this call
  for (const auto &id : imported_ids)
    {
      if (registry.contains (id))
        {
          throw ZrythmException (
            fmt::format (
              "Clipboard: object {} is already registered in the target "
              "project; regenerate UUIDs before importing",
              id.toString ()));
        }
    }

  // Only pay for a copy of the registry JSON when shared file audio
  // sources must actually be withheld from the import
  if (!has_shared_file_audio_sources)
    {
      try
        {
          from_json (json_->registry_json_, registry);
        }
      catch (...)
        {
          cleanup_failed_import (registry, imported_ids);
          throw;
        }
      return imported_ids;
    }

  auto  registry_json = json_->registry_json_;
  auto &fas_bucket = registry_json.at (ProjectRegistry::kFileAudioSourcesKey);
  for (auto it = fas_bucket.begin (); it != fas_bucket.end ();)
    {
      const auto id = entry_id (*it);
      if (registry.contains (id))
        {
          it = fas_bucket.erase (it);
        }
      else
        {
          ++it;
        }
    }
  try
    {
      from_json (registry_json, registry);
    }
  catch (...)
    {
      cleanup_failed_import (registry, imported_ids);
      throw;
    }
  return imported_ids;
}

void
ClipboardPayload::cleanup_failed_import (
  ProjectRegistry       &registry,
  std::span<const QUuid> imported_ids) const
{
  // Clean up exactly the objects this import call created: shared file
  // audio sources withheld from the import still belong to the target
  // project (possibly kept registered for undo) and must survive
  registry.delete_objects_in_any_order (imported_ids);
}

void
to_json (nlohmann::json &j, const ClipboardPayload &payload)
{
  j[utils::serialization::kDocumentTypeKey] = ClipboardPayload::kDocumentType;
  j[kFormatVersionKey] = ClipboardPayload::kFormatVersion;
  switch (payload.type_)
    {
    case ClipboardPayload::Type::ArrangerObjects:
      j[kPayloadTypeKey] = kPayloadTypeArrangerObjects;
      break;
    case ClipboardPayload::Type::Tracks:
      j[kPayloadTypeKey] = kPayloadTypeTracks;
      break;
    case ClipboardPayload::Type::Plugins:
      j[kPayloadTypeKey] = kPayloadTypePlugins;
      break;
    }
  j[kSourceProjectIdKey] = payload.source_project_id_;
  j[kMetadataKey] = payload.json_->metadata_;
  j[kRegistryKey] = payload.json_->registry_json_;
  j[kRootsKey] = payload.roots_;
}

void
from_json (const nlohmann::json &j, ClipboardPayload &payload)
{
  const auto type_str = j.at (kPayloadTypeKey).get<std::string> ();
  if (type_str == kPayloadTypeArrangerObjects)
    payload.type_ = ClipboardPayload::Type::ArrangerObjects;
  else if (type_str == kPayloadTypeTracks)
    payload.type_ = ClipboardPayload::Type::Tracks;
  else if (type_str == kPayloadTypePlugins)
    payload.type_ = ClipboardPayload::Type::Plugins;
  else
    throw std::runtime_error (
      fmt::format ("Invalid clipboard payload type '{}'", type_str));

  j.at (kSourceProjectIdKey).get_to (payload.source_project_id_);
  j.at (kMetadataKey).get_to (payload.json_->metadata_);
  j.at (kRegistryKey).get_to (payload.json_->registry_json_);
  j.at (kRootsKey).get_to (payload.roots_);
}

} // namespace zrythm::structure::project
