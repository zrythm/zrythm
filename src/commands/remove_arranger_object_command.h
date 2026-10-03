// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "commands/arranger_object_owner_ref.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/tracks/lane_restoration.h"

#include <QUndoCommand>

namespace zrythm::commands
{

template <structure::arrangement::FinalArrangerObjectSubclass ObjectT>
class RemoveArrangerObjectCommand : public QUndoCommand
{
public:
  /**
   * @brief QUndoCommand id for this removal.
   *
   * Tempo/time-signature object specializations evaluate to a distinct id so
   * the undo stack can recognize them and pause the audio engine before
   * running the command — the tempo map's RT-side data must not change while
   * the audio thread is processing. All other object types have no id (-1),
   * which is QUndoCommand's default for "no special handling".
   */
  static constexpr int CommandId =
    (std::is_same_v<ObjectT, structure::arrangement::TempoObject>
     || std::is_same_v<ObjectT, structure::arrangement::TimeSignatureObject>)
      ? 1781964242
      : -1;

  /**
   * @param object_owner handle to the owner; must resolve to an
   * ArrangerObjectOwner<ObjectT>.
   * @param object_ref reference to the object being removed.
   * @throw std::invalid_argument if the owner handle does not resolve to an
   * ArrangerObjectOwner<ObjectT>.
   */
  RemoveArrangerObjectCommand (
    ArrangerObjectOwnerRef                              object_owner,
    structure::arrangement::ArrangerObjectUuidReference object_ref)
      : QUndoCommand (QObject::tr ("Remove Object")),
        object_owner_ (std::move (object_owner)),
        object_ref_ (std::move (object_ref))
  {
    object_owner_.resolve_or_throw<ObjectT> ();
    lane_restoration_ = structure::tracks::capture_lane_restoration (
      object_owner_.typed_ref<structure::tracks::TrackLane> ());
  }

  /**
   * Convenience overload for owners that are not registry objects: the
   * owner is held as raw pointers and must outlive this command.
   */
  RemoveArrangerObjectCommand (
    structure::arrangement::ArrangerObjectOwner<ObjectT> &object_owner,
    structure::arrangement::ArrangerObjectUuidReference   object_ref)
      : RemoveArrangerObjectCommand (
          ArrangerObjectOwnerRef::from_owner_pointers (
            structure::arrangement::ArrangerObjectOwnerPtrVariant{ &object_owner }),
          std::move (object_ref))
  {
  }

  // The constructor validated this resolution, and the handle keeps the
  // owner alive for the command's lifetime (through its registry
  // keep-alive for registered owners, or the caller's ownership for
  // owners held as raw pointers), so these resolves cannot return null
  void undo () override
  {
    // The trailing-lane trim of a command undone before this one may
    // have detached the owner lane; the helper reattaches it first
    reattach_and_add<ObjectT> (object_owner_, object_ref_, lane_restoration_);
  }
  void redo () override
  {
    auto * owner = object_owner_.resolve<ObjectT> ();
    owner->remove_object (object_ref_.id ());
    // keeps a single trailing empty lane in the list the object left
    structure::tracks::trim_trailing_empty_lanes_if_lane (owner);
  }

  int id () const override { return CommandId; }

private:
  ArrangerObjectOwnerRef                              object_owner_;
  structure::arrangement::ArrangerObjectUuidReference object_ref_;
  std::optional<structure::tracks::LaneRestoration>   lane_restoration_;
};

} // namespace zrythm::commands
