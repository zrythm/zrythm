// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/track_collection.h"

#include <QtQmlIntegration>

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
    QObject *                            parent)
      : QAbstractListModel (parent), object_registry_ (object_registry),
        track_collection_ (track_collection)
  {
    // Initialize clip slots to match existing tracks
    for (size_t i = 0; i < track_collection.track_count (); ++i)
      {
        clip_slots_.emplace_back (
          utils::make_qobject_unique<ClipSlot> (object_registry_, this));
      }

    // Connect to track collection changes to keep clip slots synced
    QObject::connect (
      &track_collection, &tracks::TrackCollection::rowsInserted, this,
      [this] (const QModelIndex &parent, int first, int last) {
        // Insert clip slots for new tracks
        beginInsertRows (parent, first, last);
        for (int i = first; i <= last; ++i)
          {
            clip_slots_.insert (
              clip_slots_.begin () + i,
              utils::make_qobject_unique<ClipSlot> (object_registry_, this));
          }
        endInsertRows ();
      });

    QObject::connect (
      &track_collection, &tracks::TrackCollection::rowsMoved, this,
      [this] (
        const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
        const QModelIndex &destinationParent, int destinationRow) {
        // Move clip slots to match track movement
        beginMoveRows (
          sourceParent, sourceStart, sourceEnd, destinationParent,
          destinationRow);

        // Extract the range to move
        std::vector<utils::QObjectUniquePtr<ClipSlot>> moved_slots;
        for (int i = sourceStart; i <= sourceEnd; ++i)
          {
            moved_slots.emplace_back (std::move (clip_slots_[i]));
          }

        // Remove from original position
        clip_slots_.erase (
          clip_slots_.begin () + sourceStart,
          clip_slots_.begin () + sourceEnd + 1);

        // Insert at new position
        int insert_pos = destinationRow;
        if (destinationRow > sourceStart)
          {
            insert_pos = destinationRow - (sourceEnd - sourceStart + 1);
          }
        clip_slots_.insert (
          clip_slots_.begin () + insert_pos,
          std::make_move_iterator (moved_slots.begin ()),
          std::make_move_iterator (moved_slots.end ()));

        endMoveRows ();
      });

    QObject::connect (
      &track_collection, &tracks::TrackCollection::rowsRemoved, this,
      [this] (const QModelIndex &parent, int first, int last) {
        // Remove clip slots for removed tracks
        beginRemoveRows (parent, first, last);
        clip_slots_.erase (
          clip_slots_.begin () + first, clip_slots_.begin () + last + 1);
        endRemoveRows ();
      });
  }
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
  }

  // ========================================================================

  auto &clip_slots () const { return clip_slots_; }

private:
  // These must be in the order of tracks in the tracklist.
  std::vector<utils::QObjectUniquePtr<ClipSlot>> clip_slots_;

  arrangement::ArrangerObjectRegistry &object_registry_;
  const tracks::TrackCollection       &track_collection_;
};
}
