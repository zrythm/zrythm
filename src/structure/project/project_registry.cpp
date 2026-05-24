// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "dsp/file_audio_source.h"
#include "dsp/parameter.h"
#include "dsp/port_all.h"
#include "dsp/port_fwd.h"
#include "plugins/plugin.h"
#include "plugins/plugin_factory.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/arranger_object_fwd.h"
#include "structure/project/project_registry.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_fwd.h"
#include "utils/serialization.h"
#include "utils/traits.h"
#include "utils/variant_helpers.h"

#include <boost/unordered/unordered_flat_map.hpp>

namespace zrythm::structure::project
{

struct ProjectRegistry::Impl
{
  enum class Category : uint8_t
  {
    Port,
    Param,
    Plugin,
    Track,
    ArrangerObject,
    FileAudioSource,
  };

  boost::unordered::unordered_flat_map<QUuid, dsp::PortPtrVariant> ports_;
  boost::unordered::unordered_flat_map<QUuid, dsp::ProcessorParameter *> params_;
  boost::unordered::unordered_flat_map<QUuid, plugins::PluginPtrVariant> plugins_;
  boost::unordered::unordered_flat_map<QUuid, structure::tracks::TrackPtrVariant>
    tracks_;
  boost::unordered::
    unordered_flat_map<QUuid, structure::arrangement::ArrangerObjectPtrVariant>
      arranger_objects_;
  boost::unordered::unordered_flat_map<QUuid, dsp::FileAudioSource *>
    file_audio_sources_;

  boost::unordered::unordered_flat_map<QUuid, Category> uuid_to_category_;
  boost::unordered::unordered_flat_map<QUuid, int>      ref_counts_;

  std::optional<DeserializationDependencies> deserialization_dependencies_;
};

ProjectRegistry::ProjectRegistry (QObject * parent)
    : QObject (parent), impl_ (std::make_unique<Impl> ())
{
}

ProjectRegistry::~ProjectRegistry ()
{
  destroying_ = true;
}

void
ProjectRegistry::set_deserialization_dependencies (
  DeserializationDependencies deps)
{
  impl_->deserialization_dependencies_.emplace (std::move (deps));
}

void
ProjectRegistry::register_object_impl (utils::UuidIdentifiableBase &base)
{
  auto * qobj = static_cast<QObject *> (&base);
  assert (qobj != nullptr);
  const auto uuid = base.raw_uuid ();

  if (impl_->uuid_to_category_.contains (uuid))
    {
      throw std::runtime_error (
        fmt::format ("duplicate UUID: {}", uuid.toString ()));
    }

  if (auto * port = qobject_cast<dsp::Port *> (qobj))
    {
      auto var = utils::convert_to_variant_qobj<dsp::PortPtrVariant> (port);
      impl_->ports_.emplace (uuid, var);
      impl_->uuid_to_category_.emplace (uuid, Impl::Category::Port);
      qobj->setParent (this);
      return;
    }

  if (auto * param = qobject_cast<dsp::ProcessorParameter *> (qobj))
    {
      impl_->params_.emplace (uuid, param);
      impl_->uuid_to_category_.emplace (uuid, Impl::Category::Param);
      qobj->setParent (this);
      return;
    }

  if (auto * plugin = qobject_cast<plugins::Plugin *> (qobj))
    {
      auto var =
        utils::convert_to_variant_qobj<plugins::PluginPtrVariant> (plugin);
      impl_->plugins_.emplace (uuid, var);
      impl_->uuid_to_category_.emplace (uuid, Impl::Category::Plugin);
      qobj->setParent (this);
      return;
    }

  if (auto * track = qobject_cast<structure::tracks::Track *> (qobj))
    {
      auto var = utils::convert_to_variant_qobj<
        structure::tracks::TrackPtrVariant> (track);
      impl_->tracks_.emplace (uuid, var);
      impl_->uuid_to_category_.emplace (uuid, Impl::Category::Track);
      qobj->setParent (this);
      return;
    }

  if (auto * obj = qobject_cast<structure::arrangement::ArrangerObject *> (qobj))
    {
      auto var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj);
      impl_->arranger_objects_.emplace (uuid, var);
      impl_->uuid_to_category_.emplace (uuid, Impl::Category::ArrangerObject);
      qobj->setParent (this);
      return;
    }

  if (auto * fas = qobject_cast<dsp::FileAudioSource *> (qobj))
    {
      impl_->file_audio_sources_.emplace (uuid, fas);
      impl_->uuid_to_category_.emplace (uuid, Impl::Category::FileAudioSource);
      qobj->setParent (this);
      return;
    }

  throw std::runtime_error (
    fmt::format ("Unknown object type in ProjectRegistry: {}", uuid.toString ()));
}

void
ProjectRegistry::acquire_reference_impl (const QUuid &id)
{
  impl_->ref_counts_[id]++;
}

void
ProjectRegistry::release_reference_impl (const QUuid &id)
{
  if (destroying_)
    return;
  auto it = impl_->ref_counts_.find (id);
  if (it == impl_->ref_counts_.end () || it->second <= 0)
    {
      throw std::runtime_error (
        fmt::format (
          "release_reference: invalid ref count for UUID {}", id.toString ()));
    }
  if (--it->second <= 0)
    delete_object_by_id (id);
}

utils::UuidIdentifiableBase *
ProjectRegistry::find_by_raw_uuid_impl (const QUuid &id) const
{
  const auto cat_it = impl_->uuid_to_category_.find (id);
  if (cat_it == impl_->uuid_to_category_.end ())
    return nullptr;

  auto extract_base = [] (const auto &variant) -> utils::UuidIdentifiableBase * {
    return std::visit (
      [] (auto * ptr) -> utils::UuidIdentifiableBase * { return ptr; }, variant);
  };

  switch (cat_it->second)
    {
    case Impl::Category::Port:
      {
        auto it = impl_->ports_.find (id);
        return it != impl_->ports_.end () ? extract_base (it->second) : nullptr;
      }
    case Impl::Category::Param:
      {
        auto it = impl_->params_.find (id);
        return it != impl_->params_.end () ? it->second : nullptr;
      }
    case Impl::Category::Plugin:
      {
        auto it = impl_->plugins_.find (id);
        return it != impl_->plugins_.end () ? extract_base (it->second) : nullptr;
      }
    case Impl::Category::Track:
      {
        auto it = impl_->tracks_.find (id);
        return it != impl_->tracks_.end () ? extract_base (it->second) : nullptr;
      }
    case Impl::Category::ArrangerObject:
      {
        auto it = impl_->arranger_objects_.find (id);
        return it != impl_->arranger_objects_.end ()
                 ? extract_base (it->second)
                 : nullptr;
      }
    case Impl::Category::FileAudioSource:
      {
        auto it = impl_->file_audio_sources_.find (id);
        return it != impl_->file_audio_sources_.end () ? it->second : nullptr;
      }
    }
  return nullptr;
}

bool
ProjectRegistry::contains_impl (const QUuid &id) const
{
  return impl_->uuid_to_category_.contains (id);
}

// ============================================================================
// Type-filtered iteration
// ============================================================================

void
ProjectRegistry::for_each_matching_impl (
  const QMetaObject &meta_type,
  ObjectVisitor      visitor) const
{
  auto visit_if_matching = [&] (utils::UuidIdentifiableBase &obj) {
    if (obj.metaObject ()->inherits (&meta_type))
      visitor (obj);
  };

  if (meta_type.inherits (&dsp::FileAudioSource::staticMetaObject))
    {
      for (const auto &[uuid, ptr] : impl_->file_audio_sources_)
        visit_if_matching (*ptr);
      return;
    }
  if (
    meta_type.inherits (
      &structure::arrangement::ArrangerObject::staticMetaObject))
    {
      for (const auto &[uuid, var] : impl_->arranger_objects_)
        std::visit ([&] (auto * p) { visit_if_matching (*p); }, var);
      return;
    }
  if (meta_type.inherits (&structure::tracks::Track::staticMetaObject))
    {
      for (const auto &[uuid, var] : impl_->tracks_)
        std::visit ([&] (auto * p) { visit_if_matching (*p); }, var);
      return;
    }
  if (meta_type.inherits (&plugins::Plugin::staticMetaObject))
    {
      for (const auto &[uuid, var] : impl_->plugins_)
        std::visit ([&] (auto * p) { visit_if_matching (*p); }, var);
      return;
    }
  if (meta_type.inherits (&dsp::ProcessorParameter::staticMetaObject))
    {
      for (const auto &[uuid, ptr] : impl_->params_)
        visit_if_matching (*ptr);
      return;
    }
  if (meta_type.inherits (&dsp::Port::staticMetaObject))
    {
      for (const auto &[uuid, var] : impl_->ports_)
        std::visit ([&] (auto * p) { visit_if_matching (*p); }, var);
      return;
    }
}

// ============================================================================
// Private
// ============================================================================

void
ProjectRegistry::delete_object_by_id (const QUuid &id)
{
  const auto cat_it = impl_->uuid_to_category_.find (id);
  if (cat_it == impl_->uuid_to_category_.end ())
    return;

  QObject * raw = nullptr;

  auto extract = [] (const auto &var) -> QObject * {
    return std::visit ([] (auto * p) -> QObject * { return p; }, var);
  };

  switch (cat_it->second)
    {
    case Impl::Category::Port:
      {
        auto it = impl_->ports_.find (id);
        if (it != impl_->ports_.end ())
          {
            raw = extract (it->second);
            impl_->ports_.erase (it);
          }
        break;
      }
    case Impl::Category::Param:
      {
        auto it = impl_->params_.find (id);
        if (it != impl_->params_.end ())
          {
            raw = it->second;
            impl_->params_.erase (it);
          }
        break;
      }
    case Impl::Category::Plugin:
      {
        auto it = impl_->plugins_.find (id);
        if (it != impl_->plugins_.end ())
          {
            raw = extract (it->second);
            impl_->plugins_.erase (it);
          }
        break;
      }
    case Impl::Category::Track:
      {
        auto it = impl_->tracks_.find (id);
        if (it != impl_->tracks_.end ())
          {
            raw = extract (it->second);
            impl_->tracks_.erase (it);
          }
        break;
      }
    case Impl::Category::ArrangerObject:
      {
        auto it = impl_->arranger_objects_.find (id);
        if (it != impl_->arranger_objects_.end ())
          {
            raw = extract (it->second);
            impl_->arranger_objects_.erase (it);
          }
        break;
      }
    case Impl::Category::FileAudioSource:
      {
        auto it = impl_->file_audio_sources_.find (id);
        if (it != impl_->file_audio_sources_.end ())
          {
            raw = it->second;
            impl_->file_audio_sources_.erase (it);
          }
        break;
      }
    }

  impl_->uuid_to_category_.erase (id);
  impl_->ref_counts_.erase (id);
  delete raw;
}

// ============================================================================
// Serialization
// ============================================================================

static constexpr auto kPortsKey = "ports"sv;
static constexpr auto kParametersKey = "parameters"sv;
static constexpr auto kPluginsKey = "plugins"sv;
static constexpr auto kTracksKey = "tracks"sv;
static constexpr auto kArrangerObjectsKey = "arrangerObjects"sv;
static constexpr auto kFileAudioSourcesKey = "fileAudioSources"sv;

void
to_json (nlohmann::json &j, const ProjectRegistry &registry)
{
  j = nlohmann::json::object ();

  auto serialize_bucket_variant = [] (const auto &map) {
    nlohmann::json arr = nlohmann::json::array ();
    for (const auto &[uuid, var] : map)
      {
        arr.push_back (var);
      }
    return arr;
  };

  auto serialize_bucket_ptr = [] (const auto &map) {
    nlohmann::json arr = nlohmann::json::array ();
    for (const auto &[uuid, ptr] : map)
      {
        nlohmann::json obj_json;
        obj_json = *ptr;
        arr.push_back (obj_json);
      }
    return arr;
  };

  j[kPortsKey] = serialize_bucket_variant (registry.impl_->ports_);
  j[kParametersKey] = serialize_bucket_ptr (registry.impl_->params_);
  j[kPluginsKey] = serialize_bucket_variant (registry.impl_->plugins_);
  j[kTracksKey] = serialize_bucket_variant (registry.impl_->tracks_);
  j[kArrangerObjectsKey] =
    serialize_bucket_variant (registry.impl_->arranger_objects_);
  j[kFileAudioSourcesKey] =
    serialize_bucket_ptr (registry.impl_->file_audio_sources_);
}

// ============================================================================
// Deserialization builders
// ============================================================================

struct PortBuilder
{
  template <typename T> std::unique_ptr<T> build () const
  {
    if constexpr (std::is_same_v<T, dsp::AudioPort>)
      {
        return std::make_unique<T> (
          utils::Utf8String{}, dsp::PortFlow::Input,
          dsp::AudioPort::BusLayout::Mono, 1);
      }
    else if constexpr (std::is_same_v<T, dsp::MidiPort>)
      {
        return std::make_unique<T> (utils::Utf8String{}, dsp::PortFlow::Input);
      }
    else if constexpr (std::is_same_v<T, dsp::CVPort>)
      {
        return std::make_unique<T> (utils::Utf8String{}, dsp::PortFlow::Input);
      }
    else
      {
        static_assert (false, "Unsupported port type in PortBuilder");
      }
  }
};

struct ParamBuilder
{
  ProjectRegistry                         &reg;
  template <typename T> std::unique_ptr<T> build () const
  {
    return std::make_unique<T> (
      reg, dsp::ProcessorParameter::UniqueId{}, dsp::ParameterRange{},
      utils::Utf8String{});
  }
};

struct PluginBuilder
{
  plugins::PluginFactory                  &factory;
  template <typename T> std::unique_ptr<T> build () const
  {
    return factory.build_for_deserialization<T> ();
  }
};

struct TrackBuilder
{
  structure::tracks::TrackFactory         &factory;
  template <typename T> std::unique_ptr<T> build () const
  {
    return factory.get_builder<T> ().build_for_deserialization ();
  }
};

struct ArrangerObjectBuilder
{
  structure::arrangement::ArrangerObjectFactory &factory;
  template <typename T> std::unique_ptr<T>       build () const
  {
    return factory.get_builder<T> ().build_empty ();
  }
};

struct FileAudioSourceBuilder
{
  template <typename T> std::unique_ptr<T> build () const
  {
    return std::make_unique<T> ();
  }
};

// ============================================================================
// Two-phase deserialization helpers
// ============================================================================

/**
 * @brief Phase 1 helper: create and register all objects for a variant bucket.
 *
 * Sets UUID from JSON before registering so each object is registered under
 * its correct UUID. Appends (variant, json) pairs to @p deferred for Phase 2.
 */
template <
  typename VariantT,
  utils::RangeOf<std::pair<VariantT, nlohmann::json>> DeferredRange,
  typename BuilderT>
void
create_and_register_all (
  ProjectRegistry      &registry,
  const nlohmann::json &entries,
  const BuilderT       &builder,
  DeferredRange        &deferred)
{
  for (const auto &entry : entries)
    {
      VariantT var;
      utils::serialization::variant_create_object_only (entry, var, builder);

      std::visit (
        [&entry] (auto &&ptr) {
          from_json (entry, static_cast<utils::UuidIdentifiableBase &> (*ptr));
        },
        var);

      std::visit ([&] (auto * p) { registry.register_object (*p); }, var);
      deferred.emplace_back (var, entry);
    }
}

/**
 * @brief Phase 1 helper: create and register all objects for a pointer bucket.
 *
 * Releases ownership after registration (registry owns via QObject
 * parentship). Appends (raw pointer, json) pairs to @p deferred for Phase 2.
 */
template <
  typename T,
  utils::RangeOf<std::pair<T *, nlohmann::json>> DeferredRange,
  typename BuilderT>
void
create_and_register_ptr_all (
  ProjectRegistry      &registry,
  const nlohmann::json &entries,
  const BuilderT       &builder,
  DeferredRange        &deferred)
{
  for (const auto &entry : entries)
    {
      auto obj = builder.template build<T> ();
      from_json (entry, static_cast<utils::UuidIdentifiableBase &> (*obj));
      try
        {
          registry.register_object (*obj);
        }
      catch (...)
        {
          obj->setParent (nullptr);
          throw;
        }
      deferred.emplace_back (obj.release (), entry);
    }
}

/**
 * @brief Phase 2: deserialize data into all objects in a variant deferred
 * range.
 */
template <
  typename VariantT,
  utils::RangeOf<std::pair<VariantT, nlohmann::json>> DeferredRange>
void
deserialize_all (DeferredRange &deferred)
{
  for (auto &[object_var, json] : deferred)
    {
      utils::serialization::variant_deserialize_data (json, object_var);
    }
}

/**
 * @brief Phase 2: deserialize data into all objects in a pointer deferred
 * range.
 */
template <typename T, utils::RangeOf<std::pair<T *, nlohmann::json>> DeferredRange>
void
deserialize_ptr_all (DeferredRange &deferred)
{
  for (auto &[ptr, json] : deferred)
    {
      nlohmann::json value_json = json;
      value_json.erase (utils::serialization::kVariantTypeKey);
      value_json.get_to (*ptr);
    }
}

// ============================================================================
// Deserialization
// ============================================================================

void
from_json (const nlohmann::json &j, ProjectRegistry &registry)
{
  if (!registry.impl_->deserialization_dependencies_.has_value ())
    {
      throw std::runtime_error (
        "set_deserialization_dependencies() must be called before from_json");
    }
  const auto &deps = *registry.impl_->deserialization_dependencies_;

  // Deferred work storage for each bucket.
  std::vector<std::pair<dsp::PortPtrVariant, nlohmann::json>> deferred_ports;
  std::vector<std::pair<dsp::ProcessorParameter *, nlohmann::json>>
    deferred_params;
  std::vector<std::pair<plugins::PluginPtrVariant, nlohmann::json>>
    deferred_plugins;
  std::vector<std::pair<dsp::FileAudioSource *, nlohmann::json>>
    deferred_file_audio_sources;
  std::vector<
    std::pair<structure::arrangement::ArrangerObjectPtrVariant, nlohmann::json>>
    deferred_arranger_objects;
  std::vector<std::pair<structure::tracks::TrackPtrVariant, nlohmann::json>>
    deferred_tracks;

  // --- Phase 1: Create and register ALL objects from ALL buckets ---
  // All objects are created, assigned their UUIDs from JSON, and registered.
  // No data deserialization happens yet — objects exist but have no data.
  // This ensures every UUID is resolvable before any from_json reads
  // references.

  if (j.contains (kPortsKey))
    {
      deferred_ports.reserve (j[kPortsKey].size ());
      create_and_register_all<dsp::PortPtrVariant> (
        registry, j[kPortsKey], PortBuilder{}, deferred_ports);
    }

  if (j.contains (kParametersKey))
    {
      deferred_params.reserve (j[kParametersKey].size ());
      create_and_register_ptr_all<dsp::ProcessorParameter> (
        registry, j[kParametersKey], ParamBuilder{ registry }, deferred_params);
    }

  if (j.contains (kPluginsKey))
    {
      deferred_plugins.reserve (j[kPluginsKey].size ());
      create_and_register_all<plugins::PluginPtrVariant> (
        registry, j[kPluginsKey], PluginBuilder{ deps.plugin_factory },
        deferred_plugins);
    }

  if (j.contains (kFileAudioSourcesKey))
    {
      deferred_file_audio_sources.reserve (j[kFileAudioSourcesKey].size ());
      create_and_register_ptr_all<dsp::FileAudioSource> (
        registry, j[kFileAudioSourcesKey], FileAudioSourceBuilder{},
        deferred_file_audio_sources);
    }

  if (j.contains (kArrangerObjectsKey))
    {
      deferred_arranger_objects.reserve (j[kArrangerObjectsKey].size ());
      create_and_register_all<structure::arrangement::ArrangerObjectPtrVariant> (
        registry, j[kArrangerObjectsKey],
        ArrangerObjectBuilder{ deps.arranger_object_factory },
        deferred_arranger_objects);
    }

  if (j.contains (kTracksKey))
    {
      deferred_tracks.reserve (j[kTracksKey].size ());
      create_and_register_all<structure::tracks::TrackPtrVariant> (
        registry, j[kTracksKey], TrackBuilder{ deps.track_factory },
        deferred_tracks);
    }

  // --- Phase 2: Deserialize data into ALL objects from ALL buckets ---
  // Same order as Phase 1: ports → params → plugins → file audio sources →
  // arranger objects → tracks. Within each bucket, JSON array order is
  // preserved so children are deserialized before parents.

  deserialize_all<dsp::PortPtrVariant> (deferred_ports);
  deserialize_ptr_all<dsp::ProcessorParameter> (deferred_params);
  deserialize_all<plugins::PluginPtrVariant> (deferred_plugins);
  deserialize_ptr_all<dsp::FileAudioSource> (deferred_file_audio_sources);
  deserialize_all<structure::arrangement::ArrangerObjectPtrVariant> (
    deferred_arranger_objects);
  deserialize_all<structure::tracks::TrackPtrVariant> (deferred_tracks);
}

} // namespace zrythm::structure::project
