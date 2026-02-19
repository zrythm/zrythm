// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/scene.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::scenes
{

// Scene implementation
void
Scene::setName (const QString &name)
{
  if (name_ != name)
    {
      name_ = name;
      Q_EMIT nameChanged (name_);
    }
}

void
Scene::setColor (const QColor &color)
{
  if (color_ != color)
    {
      color_ = color;
      Q_EMIT colorChanged (color_);
    }
}

// SceneList implementation
SceneList::SceneList (
  arrangement::ArrangerObjectRegistry &object_registry,
  const tracks::TrackCollection       &track_collection,
  QObject *                            parent)
    : QAbstractListModel (parent), object_registry_ (object_registry),
      track_collection_ (track_collection)
{
}

void
SceneList::insert_scene (utils::QObjectUniquePtr<Scene> scene, int index)
{
  beginInsertRows ({}, index, index);
  scenes_.emplace_back (std::move (scene));
  endInsertRows ();
}

void
SceneList::removeScene (int index)
{
  if (index < 0 || index >= static_cast<int> (scenes_.size ()))
    return;

  beginRemoveRows ({}, index, index);
  scenes_.erase (scenes_.begin () + index);
  endRemoveRows ();
}

void
SceneList::moveScene (int fromIndex, int toIndex)
{
  if (
    fromIndex < 0 || fromIndex >= static_cast<int> (scenes_.size ())
    || toIndex < 0 || toIndex >= static_cast<int> (scenes_.size ())
    || fromIndex == toIndex)
    return;

  beginMoveRows ({}, fromIndex, fromIndex, {}, toIndex);
  if (fromIndex < toIndex)
    {
      // Moving down
      auto scene = std::move (scenes_[fromIndex]);
      scenes_.erase (scenes_.begin () + fromIndex);
      scenes_.insert (scenes_.begin () + toIndex, std::move (scene));
    }
  else
    {
      // Moving up
      auto scene = std::move (scenes_[fromIndex]);
      scenes_.erase (scenes_.begin () + fromIndex);
      scenes_.insert (scenes_.begin () + toIndex, std::move (scene));
    }
  endMoveRows ();
}

// ============================================================================
// Serialization
// ============================================================================

void
to_json (nlohmann::json &j, const Scene &scene)
{
  j[Scene::kNameKey] = scene.name_.toStdString ();
  if (scene.color_.isValid ())
    {
      j[Scene::kColorKey] = scene.color_.name (QColor::HexRgb).toStdString ();
    }
  j[Scene::kClipSlotsKey] = *scene.clip_slot_list_;
}

void
from_json (const nlohmann::json &j, Scene &scene)
{
  if (j.contains (Scene::kNameKey))
    {
      scene.name_ =
        QString::fromStdString (j.at (Scene::kNameKey).get<std::string> ());
    }
  if (j.contains (Scene::kColorKey))
    {
      scene.color_ = QColor (
        QString::fromStdString (j.at (Scene::kColorKey).get<std::string> ()));
    }
  if (j.contains (Scene::kClipSlotsKey))
    {
      j.at (Scene::kClipSlotsKey).get_to (*scene.clip_slot_list_);
    }
}

void
to_json (nlohmann::json &j, const SceneList &list)
{
  j = nlohmann::json::array ();
  for (const auto &scene : list.scenes_)
    {
      j.push_back (*scene);
    }
}

void
from_json (const nlohmann::json &j, SceneList &list)
{
  if (j.is_array ())
    {
      for (const auto &scene_json : j)
        {
          auto scene = utils::make_qobject_unique<Scene> (
            list.object_registry_, list.track_collection_, &list);
          from_json (scene_json, *scene);
          list.scenes_.push_back (std::move (scene));
        }
    }
}

}
