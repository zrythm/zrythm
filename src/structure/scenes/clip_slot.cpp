// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/clip.h"
#include "structure/scenes/clip_slot.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::scenes
{
ClipSlot::ClipSlot (utils::IObjectRegistry &registry, QObject * parent)
    : QObject (parent), registry_ (registry)
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
      auto   slot = utils::make_qobject_unique<ClipSlot> (registry_, this);
      auto * track = track_collection.get_track_at_index (i);
      if (track != nullptr)
        slot->setTimebaseProvider (track->timebaseProvider ());
      clip_slots_.emplace_back (std::move (slot));
    }

  // Connect to track collection changes to keep clip slots synced
  QObject::connect (
    &track_collection, &tracks::TrackCollection::rowsInserted, this,
    [this] (const QModelIndex &parentIndex, int first, int last) {
      // Insert clip slots for new tracks
      beginInsertRows (parentIndex, first, last);
      for (int i = first; i <= last; ++i)
        {
          auto   slot = utils::make_qobject_unique<ClipSlot> (registry_, this);
          auto * track =
            track_collection_.get_track_at_index (static_cast<size_t> (i));
          if (track != nullptr)
            slot->setTimebaseProvider (track->timebaseProvider ());
          clip_slots_.insert (clip_slots_.begin () + i, std::move (slot));
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
