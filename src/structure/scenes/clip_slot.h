// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/track_collection.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::scenes
{

class ClipSlot : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObject * region READ region WRITE
      setRegion NOTIFY regionChanged)
  Q_PROPERTY (ClipState state READ state WRITE setState NOTIFY stateChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")
  QML_EXTENDED_NAMESPACE (zrythm::structure::tracks)

public:
  enum class ClipState
  {
    Stopped,
    PlayQueued,
    Playing,
    StopQueued,
  };
  Q_ENUM (ClipState)

  ClipSlot (
    arrangement::ArrangerObjectRegistry &object_registry,
    QObject *                            parent = nullptr);

  arrangement::ArrangerObject * region () const
  {
    if (region_ref_.has_value ())
      {
        return region_ref_->get_object_base ();
      }
    return nullptr;
  }
  void          setRegion (arrangement::ArrangerObject * region);
  Q_SIGNAL void regionChanged (arrangement::ArrangerObject * region);

  // Region management
  Q_INVOKABLE void clearRegion ()
  {
    if (!region_ref_.has_value ())
      {
        return;
      }

    region_ref_.reset ();
    Q_EMIT regionChanged (nullptr);
  }

  ClipState     state () const { return state_.load (); }
  void          setState (ClipState state);
  Q_SIGNAL void stateChanged (ClipState state);

private:
  static constexpr auto kRegionIdKey = "regionId"sv;
  friend void           to_json (nlohmann::json &j, const ClipSlot &slot);
  friend void           from_json (const nlohmann::json &j, ClipSlot &slot);

private:
  std::optional<arrangement::ArrangerObjectUuidReference> region_ref_;
  arrangement::ArrangerObjectRegistry                    &object_registry_;
  std::atomic<ClipState> state_{ ClipState::Stopped };
};

class ClipSlotList : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ClipSlotList (
    arrangement::ArrangerObjectRegistry &object_registry,
    const tracks::TrackCollection       &track_collection,
    QObject *                            parent = nullptr);
  Z_DISABLE_COPY_MOVE (ClipSlotList)

  enum ClipSlotListRoles
  {
    ClipSlotPtrRole = Qt::UserRole + 1,
  };
  Q_ENUM (ClipSlotListRoles)

  // ========================================================================
  // QML Interface
  // ========================================================================

  Q_INVOKABLE ClipSlot * clipSlotForTrack (const tracks::Track * track) const
  {
    return clip_slots_
      .at (track_collection_.get_track_index (track->get_uuid ()))
      .get ();
  }

  QHash<int, QByteArray> roleNames () const override
  {
    QHash<int, QByteArray> roles;
    roles[ClipSlotPtrRole] = "clipSlot";
    return roles;
  }
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    if (parent.isValid ())
      return 0;
    return static_cast<int> (clip_slots_.size ());
  }
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override
  {
    const auto index_int = index.row ();
    if (!index.isValid () || index_int >= static_cast<int> (clip_slots_.size ()))
      return {};

    if (role == ClipSlotPtrRole)
      {
        return QVariant::fromValue (clip_slots_.at (index_int).get ());
      }

    return {};
  }

  // ========================================================================

  auto &clip_slots () const { return clip_slots_; }

private:
  friend void to_json (nlohmann::json &j, const ClipSlotList &list);
  friend void from_json (const nlohmann::json &j, ClipSlotList &list);

private:
  // These must be in the order of tracks in the tracklist.
  std::vector<utils::QObjectUniquePtr<ClipSlot>> clip_slots_;

  arrangement::ArrangerObjectRegistry &object_registry_;
  const tracks::TrackCollection       &track_collection_;
};
}
