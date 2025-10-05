// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/scene.h"

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

}
