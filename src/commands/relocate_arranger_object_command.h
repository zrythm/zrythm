// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <stdexcept>
#include <utility>

#include "commands/arranger_object_owner_ref.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/tracks/track_lane_list.h"

#include <QUndoCommand>

namespace zrythm::commands
{
template <structure::arrangement::FinalArrangerObjectSubclass ObjectT>
class RelocateArrangerObjectCommand : public QUndoCommand
{
public:
  /**
   * @param clip_ref reference to the object being moved.
   * @param source_owner handle to the owner currently holding the object;
   * must resolve to an ArrangerObjectOwner<ObjectT> that contains the
   * object.
   * @param target_owner handle to the owner receiving the object; must
   * resolve to an ArrangerObjectOwner<ObjectT>.
   * @throw std::invalid_argument if either handle fails to resolve or the
   * source owner does not contain the object.
   */
  RelocateArrangerObjectCommand (
    structure::arrangement::ArrangerObjectUuidReference clip_ref,
    ArrangerObjectOwnerRef                              source_owner,
    ArrangerObjectOwnerRef                              target_owner)
      : QUndoCommand (QObject::tr ("Relocate Object")),
        obj_ref_ (std::move (clip_ref)), target_owner_ (std::move (target_owner)),
        source_owner_ (std::move (source_owner))
  {
    const auto &source = source_owner_.resolve_or_throw<ObjectT> ();
    (void) target_owner_.resolve_or_throw<ObjectT> ();
    if (
      !std::ranges::contains (
        source.get_children_vector (), obj_ref_.id (),
        &structure::arrangement::ArrangerObjectUuidReference::id))
      {
        throw std::invalid_argument ("Source owner does not include the object");
      }
  }

  /**
   * Convenience overload for owners that are not registry objects: both
   * owners are held as raw pointers and must outlive this command.
   */
  RelocateArrangerObjectCommand (
    structure::arrangement::ArrangerObjectUuidReference   clip_ref,
    structure::arrangement::ArrangerObjectOwner<ObjectT> &source_owner,
    structure::arrangement::ArrangerObjectOwner<ObjectT> &target_owner)
      : RelocateArrangerObjectCommand (
          std::move (clip_ref),
          ArrangerObjectOwnerRef::from_owner_pointers (
            structure::arrangement::ArrangerObjectOwnerPtrVariant{ &source_owner }),
          ArrangerObjectOwnerRef::from_owner_pointers (
            structure::arrangement::ArrangerObjectOwnerPtrVariant{ &target_owner }))
  {
  }

  // The constructor validated these resolutions, and each handle keeps
  // its owner alive for the command's lifetime (through its registry
  // keep-alive for registered owners, or the caller's ownership for
  // owners held as raw pointers), so they cannot return null
  void undo () override
  {
    // move object back
    auto * source = source_owner_.resolve<ObjectT> ();
    auto * target = target_owner_.resolve<ObjectT> ();
    auto   clip_ref = target->remove_object (obj_ref_.id ());
    source->add_object (clip_ref);
    // keeps a single trailing empty lane in the list the object left
    structure::tracks::trim_trailing_empty_lanes_if_lane (target);
  }
  void redo () override
  {
    // move object
    auto * source = source_owner_.resolve<ObjectT> ();
    auto * target = target_owner_.resolve<ObjectT> ();
    auto   clip_ref = source->remove_object (obj_ref_.id ());
    target->add_object (clip_ref);
    // keeps a single trailing empty lane in the list the object left
    structure::tracks::trim_trailing_empty_lanes_if_lane (source);
  }

private:
  structure::arrangement::ArrangerObjectUuidReference obj_ref_;
  ArrangerObjectOwnerRef                              target_owner_;
  ArrangerObjectOwnerRef                              source_owner_;
};

} // namespace zrythm::commands
