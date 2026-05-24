// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include <memory>

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
  struct DeserializationDependencies
  {
    structure::tracks::TrackFactory               &track_factory;
    structure::arrangement::ArrangerObjectFactory &arranger_object_factory;
    plugins::PluginFactory                        &plugin_factory;
  };

  ProjectRegistry (QObject * parent = nullptr);
  ~ProjectRegistry () override;

  void set_deserialization_dependencies (DeserializationDependencies deps);

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

  void delete_object_by_id (const QUuid &id);

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
