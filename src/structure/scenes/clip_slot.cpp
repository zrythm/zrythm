// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/clip_slot.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::scenes
{
ClipSlot::ClipSlot (
  arrangement::ArrangerObjectRegistry &object_registry,
  QObject *                            parent)
    : QObject (parent), object_registry_ (object_registry)
{
}

void
ClipSlot::setRegion (arrangement::ArrangerObject * region)
{
  assert (region);

  if (region_ref_.has_value () && region_ref_->id () == region->get_uuid ())
    {
      return;
    }

  region_ref_ = arrangement::ArrangerObjectUuidReference{
    region->get_uuid (), object_registry_
  };
  Q_EMIT regionChanged (region);
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
    [this] (const QModelIndex &parentIndex, int first, int last) {
      // Insert clip slots for new tracks
      beginInsertRows (parentIndex, first, last);
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
        sourceParent, sourceStart, sourceEnd, destinationParent, destinationRow);

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
  if (slot.region_ref_.has_value ())
    {
      j[ClipSlot::kRegionIdKey] = slot.region_ref_->id ();
    }
}

void
from_json (const nlohmann::json &j, ClipSlot &slot)
{
  if (j.contains (ClipSlot::kRegionIdKey))
    {
      arrangement::ArrangerObject::Uuid region_id;
      j.at (ClipSlot::kRegionIdKey).get_to (region_id);
      slot.region_ref_.emplace (region_id, slot.object_registry_);
    }
}

void
to_json (nlohmann::json &j, const ClipSlotList &list)
{
  j = nlohmann::json::array ();
  for (const auto &slot : list.clip_slots_)
    {
      nlohmann::json slot_json;
      to_json (slot_json, *slot);
      j.push_back (std::move (slot_json));
    }
}

void
from_json (const nlohmann::json &j, ClipSlotList &list)
{
  // ClipSlotList is already sized to match track count
  if (j.is_array ())
    {
      for (auto &&[json_slot, slot_ptr] : std::views::zip (j, list.clip_slots_))
        {
          if (!json_slot.is_null () && !json_slot.empty ())
            {
              from_json (json_slot, *slot_ptr);
            }
        }
    }
}

}
