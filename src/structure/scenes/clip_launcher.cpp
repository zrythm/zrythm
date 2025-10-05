// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/clip_launcher.h"

namespace zrythm::structure::scenes
{
ClipLauncher::ClipLauncher (
  arrangement::ArrangerObjectRegistry &object_registry,
  const tracks::TrackCollection       &track_collection,
  QObject *                            parent)
    : QObject (parent), scene_list_ (utils::make_qobject_unique<SceneList> ()),
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
}
