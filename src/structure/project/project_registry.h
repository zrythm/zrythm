// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include <array>
#include <memory>
#include <string_view>

#include "utils/iobject_registry.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::plugins
{
class PluginFactory;
}
namespace zrythm::structure::tracks
{
class TrackFactory;
}
namespace zrythm::structure::arrangement
{
class ArrangerObjectFactory;
}

namespace zrythm::structure::project
{

class ProjectRegistry final : public QObject, public utils::IObjectRegistry
{
  Q_OBJECT
  Q_DISABLE_COPY_MOVE (ProjectRegistry)

public:
  /** Bucket keys used in the registry's JSON representation (also used by
   * clipboard payloads, which mirror this format). */
  static constexpr std::string_view kPortsKey = "ports";
  static constexpr std::string_view kParametersKey = "parameters";
  static constexpr std::string_view kPluginsKey = "plugins";
  static constexpr std::string_view kTracksKey = "tracks";
  static constexpr std::string_view kArrangerObjectsKey = "arrangerObjects";
  static constexpr std::string_view kFileAudioSourcesKey = "fileAudioSources";

  struct DeserializationDependencies
  {
    structure::tracks::TrackFactory               &track_factory;
    structure::arrangement::ArrangerObjectFactory &arranger_object_factory;
    plugins::PluginFactory                        &plugin_factory;
  };

  /**
   * @brief Category of a registered object (which bucket it lives in).
   */
  enum class ObjectCategory : std::uint8_t
  {
    Port,
    Param,
    Plugin,
    Track,
    ArrangerObject,
    FileAudioSource,
  };

  /** Bucket key for each object category: the single source of truth for
   * the category ↔ bucket mapping (also used by clipboard payloads, which
   * mirror the registry's JSON format). */
  static constexpr std::array kCategoryBucketKeys = {
    std::pair{ ObjectCategory::Port,            kPortsKey            },
    std::pair{ ObjectCategory::Param,           kParametersKey       },
    std::pair{ ObjectCategory::Plugin,          kPluginsKey          },
    std::pair{ ObjectCategory::Track,           kTracksKey           },
    std::pair{ ObjectCategory::ArrangerObject,  kArrangerObjectsKey  },
    std::pair{ ObjectCategory::FileAudioSource, kFileAudioSourcesKey },
  };

  ProjectRegistry (QObject * parent = nullptr);
  ~ProjectRegistry () override;

  void set_deserialization_dependencies (DeserializationDependencies deps);

  /**
   * @brief Serializes the object registered under @p id into @p j_out.
   *
   * The JSON has the same shape as entries in the registry's buckets
   * (variant format for types stored as variants).
   *
   * @return The object's category, or std::nullopt if no object with @p id
   * is registered.
   */
  std::optional<ObjectCategory>
  serialize_object_by_uuid (const QUuid &id, nlohmann::json &j_out) const;

  /**
   * @brief Deletes the object registered under @p id.
   *
   * @pre No live references target the object (asserted; violating it is
   * undefined in release builds). References held by the deleted object
   * are released as usual, which may recursively delete other objects
   * whose count drops to zero.
   *
   * Use delete_objects_in_any_order() for ids that may still be
   * referenced.
   */
  void delete_object_by_id (const QUuid &id);

  /**
   * @brief Returns whether at least one live reference currently targets
   * the object registered under @p id.
   */
  bool has_live_references (const QUuid &id) const;

  /**
   * @brief Deletes the registered objects among @p ids, insensitive to the
   * order the ids are given in.
   *
   * Objects that are still referenced are deferred and retried as other
   * deletions release their references (parent destruction releases
   * children naturally). Ids that remain referenced once no more
   * deletions are possible are left registered with a warning — deleting
   * them would be undefined.
   */
  void delete_objects_in_any_order (std::span<const QUuid> ids);

private:
  using ObjectVisitor = utils::IObjectRegistry::ObjectVisitor;

  void register_object_impl (utils::UuidIdentifiableBase &obj) override;
  void acquire_reference_impl (const QUuid &id) override;
  void release_reference_impl (const QUuid &id) override;

  [[gnu::hot]] utils::UuidIdentifiableBase *
  find_by_raw_uuid_impl (const QUuid &id) const override;

  bool contains_impl (const QUuid &id) const override;

  void
  for_each_matching_impl (const QMetaObject &meta_type, ObjectVisitor visitor)
    const override;

  // ============================================================================
  // Serialization
  // ============================================================================

  friend void to_json (nlohmann::json &j, const ProjectRegistry &registry);
  friend void from_json (const nlohmann::json &j, ProjectRegistry &registry);

private:
  struct Impl;
  bool                  destroying_ = false;
  std::unique_ptr<Impl> impl_;
};

} // namespace zrythm::structure::project
