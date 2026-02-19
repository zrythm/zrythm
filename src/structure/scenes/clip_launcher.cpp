// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/clip_launcher.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::scenes
{
ClipLauncher::ClipLauncher (
  arrangement::ArrangerObjectRegistry &object_registry,
  const tracks::TrackCollection       &track_collection,
  QObject *                            parent)
    : QObject (parent),
      scene_list_ (
        utils::make_qobject_unique<
          SceneList> (object_registry, track_collection, this)),
      object_registry_ (object_registry), track_collection_ (track_collection)
{
}

Scene *
ClipLauncher::addScene ()
{
  scene_list_->insert_scene (
    utils::make_qobject_unique<Scene> (object_registry_, track_collection_),
    scene_list_->rowCount ());
  return scene_list_->scenes ().back ().get ();
}

// ============================================================================
// Serialization
// ============================================================================

void
to_json (nlohmann::json &j, const ClipLauncher &launcher)
{
  j[ClipLauncher::kScenesKey] = *launcher.scene_list_;
}

void
from_json (const nlohmann::json &j, ClipLauncher &launcher)
{
  if (j.contains (ClipLauncher::kScenesKey))
    {
      j.at (ClipLauncher::kScenesKey).get_to (*launcher.scene_list_);
    }
}

}
