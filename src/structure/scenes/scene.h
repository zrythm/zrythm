// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "structure/scenes/clip_slot.h"
#include "structure/tracks/track_collection.h"

#include <QtQmlIntegration>

namespace zrythm::structure::scenes
{

class Scene : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY (QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY (
    zrythm::structure::scenes::ClipSlotList * clipSlots READ clipSlots CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  Scene (
    arrangement::ArrangerObjectRegistry &object_registry,
    const tracks::TrackCollection       &track_collection,
    QObject *                            parent = nullptr)
      : QObject (parent),
        clip_slot_list_ (
          utils::make_qobject_unique<
            ClipSlotList> (object_registry, track_collection, this))
  {
  }

  ClipSlotList * clipSlots () const { return clip_slot_list_.get (); }

  // Name property
  QString       name () const { return name_; }
  void          setName (const QString &name);
  Q_SIGNAL void nameChanged (const QString &name);

  // Color property
  QColor        color () const { return color_; }
  void          setColor (const QColor &color);
  Q_SIGNAL void colorChanged (const QColor &color);

  // Access to clip slots
  auto &clip_slots () const { return clip_slot_list_->clip_slots (); }

private:
  QString                               name_;
  QColor                                color_;
  utils::QObjectUniquePtr<ClipSlotList> clip_slot_list_;
};

class SceneList : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum SceneListRoles
  {
    ScenePtrRole = Qt::UserRole + 1,
  };

  // Scene management
  void insert_scene (utils::QObjectUniquePtr<Scene> scene, int index);
  Q_INVOKABLE void removeScene (int index);
  Q_INVOKABLE void moveScene (int fromIndex, int toIndex);

  auto &scenes () const { return scenes_; }

  // ========================================================================
  // QML Interface
  // ========================================================================

  QHash<int, QByteArray> roleNames () const override
  {
    QHash<int, QByteArray> roles;
    roles[ScenePtrRole] = "scene";
    return roles;
  }
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    if (parent.isValid ())
      return 0;
    return static_cast<int> (scenes_.size ());
  }
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override
  {
    const auto index_int = index.row ();
    if (!index.isValid () || index_int >= static_cast<int> (scenes_.size ()))
      return {};

    if (role == ScenePtrRole)
      {
        return QVariant::fromValue (scenes_.at (index_int).get ());
      }

    return {};
  }
  // ========================================================================

private:
  std::vector<utils::QObjectUniquePtr<Scene>> scenes_;
};
}
