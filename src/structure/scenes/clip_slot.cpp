// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "structure/arrangement/clip.h"
#include "structure/scenes/clip_slot.h"
#include "utils/exceptions.h"
#include "utils/registry_utils.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::scenes
{
ClipSlot::ClipSlot (utils::IObjectRegistry &registry, QObject * parent)
    : utils::UuidIdentifiableObject<ClipSlot> (parent), registry_ (registry)
{
}

void
ClipSlot::setClip (arrangement::Clip * clip)
{
  assert (clip);

  if (clip_ref_.has_value () && clip_ref_->id () == clip->get_uuid ())
    {
      return;
    }

  // Detach the previous occupant's timebase provider from this slot.
  if (auto * old = this->clip ())
    {
      if (auto * tp = old->timebaseProvider ())
        tp->setSource (nullptr);
    }

  if (auto * tp = clip->timebaseProvider ())
    tp->setSource (timebase_provider_);

  clip_ref_ =
    arrangement::ArrangerObjectUuidReference{ clip->get_uuid (), registry_ };
  Q_EMIT clipObjectChanged (clip);
}

void
ClipSlot::setState (ClipState state)
{
  if (state == state_.load ())
    {
      return;
    }
  state_ = state;
  Q_EMIT stateChanged (state);
}

ClipSlotList::ClipSlotList (
  utils::IObjectRegistry        &registry,
  const tracks::TrackCollection &track_collection,
  QObject *                      parent)
    : QAbstractListModel (parent), registry_ (registry),
      track_collection_ (track_collection)
{
  // Initialize clip slots to match existing tracks
  for (size_t i = 0; i < track_collection.track_count (); ++i)
    {
      clip_slots_.emplace_back (
        utils::create_object<ClipSlot> (registry_, registry_));
      update_timebase_provider (i);
    }

  // Connect to track collection changes to keep clip slots synced
  QObject::connect (
    &track_collection, &tracks::TrackCollection::rowsInserted, this,
    [this] (const QModelIndex &parentIndex, int first, int last) {
      // Insert clip slots for new tracks
      beginInsertRows (parentIndex, first, last);
      for (int i = first; i <= last; ++i)
        {
          clip_slots_.insert (
            clip_slots_.begin () + i,
            utils::create_object<ClipSlot> (registry_, registry_));
          update_timebase_provider (static_cast<size_t> (i));
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
        sourceParent, sourceStart, sourceEnd, destinationParent, destinationRow);

      // Extract the range to move
      std::vector<ClipSlotUuidReference> moved_slots;
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
    [this] (const QModelIndex &parentIndex, int first, int last) {
      // Remove clip slots for removed tracks
      beginRemoveRows (parentIndex, first, last);
      clip_slots_.erase (
        clip_slots_.begin () + first, clip_slots_.begin () + last + 1);
      endRemoveRows ();
    });
}

// ============================================================================
// Serialization
// ============================================================================

void
to_json (nlohmann::json &j, const ClipSlot &slot)
{
  to_json (j, static_cast<const ClipSlot::UuidIdentifiableObject &> (slot));
  if (slot.clip_ref_.has_value ())
    {
      j[ClipSlot::kClipIdKey] = slot.clip_ref_->id ();
    }
}

void
from_json (const nlohmann::json &j, ClipSlot &slot)
{
  if (j.contains (ClipSlot::kClipIdKey))
    {
      arrangement::ArrangerObject::Uuid clip_id;
      j.at (ClipSlot::kClipIdKey).get_to (clip_id);
      slot.clip_ref_.emplace (clip_id, slot.registry_);
    }
}

void
ClipSlotList::update_timebase_provider (size_t index)
{
  auto * track = track_collection_.get_track_at_index (index);
  if (track != nullptr)
    {
      clip_slots_.at (index).get ()->setTimebaseProvider (
        track->timebaseProvider ());
    }
}

void
to_json (nlohmann::json &j, const ClipSlotList &list)
{
  j = list.clip_slots_;
}
void
from_json (const nlohmann::json &j, ClipSlotList &list)
{
  if (!j.is_array ())
    {
      return;
    }

  // A scene holds one slot per track
  if (j.size () != list.track_collection_.track_count ())
    {
      throw ZrythmException (
        fmt::format (
          "scene has {} clip slots but the project has {} tracks", j.size (),
          list.track_collection_.track_count ()));
    }

  // References are built before the model is touched, so a failure
  // here leaves the list and its default-created slots as they were
  std::vector<ClipSlotUuidReference> slot_refs;
  slot_refs.reserve (j.size ());
  for (const auto &slot_id_json : j)
    {
      const auto slot_id = slot_id_json.get<QUuid> ();
      if (
        qobject_cast<ClipSlot *> (list.registry_.find_by_raw_uuid (slot_id))
        == nullptr)
        {
          throw ZrythmException (
            fmt::format (
              "clip slot id {} does not reference a registered clip slot",
              slot_id.toString ()));
        }
      slot_refs.emplace_back (ClipSlot::Uuid{ slot_id }, list.registry_);
    }

  // Releasing the list's references deletes the default-created slots
  // not referenced anywhere else
  if (!list.clip_slots_.empty ())
    {
      list.beginRemoveRows (
        QModelIndex (), 0, static_cast<int> (list.clip_slots_.size ()) - 1);
      list.clip_slots_.clear ();
      list.endRemoveRows ();
    }

  if (!slot_refs.empty ())
    {
      list.beginInsertRows (
        QModelIndex (), 0, static_cast<int> (slot_refs.size () - 1));
      list.clip_slots_ = std::move (slot_refs);
      list.endInsertRows ();

      for (size_t i = 0; i < list.clip_slots_.size (); ++i)
        list.update_timebase_provider (i);
    }
}

}
