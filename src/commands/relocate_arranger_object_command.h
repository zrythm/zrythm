// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"

#include <QUndoCommand>

namespace zrythm::commands
{
template <structure::arrangement::FinalArrangerObjectSubclass ObjectT>
class RelocateArrangerObjectCommand : public QUndoCommand
{
public:
  RelocateArrangerObjectCommand (
    structure::arrangement::ArrangerObjectUuidReference   clip_ref,
    structure::arrangement::ArrangerObjectOwner<ObjectT> &source_owner,
    structure::arrangement::ArrangerObjectOwner<ObjectT> &target_owner)
      : QUndoCommand (QObject::tr ("Relocate Object")),
        obj_ref_ (std::move (clip_ref)), target_owner_ (target_owner),
        source_owner_ (source_owner)
  {
    if (
      !std::ranges::contains (
        source_owner_.get_children_vector (), obj_ref_.id (),
        &structure::arrangement::ArrangerObjectUuidReference::id))
      {
        throw std::invalid_argument ("Source owner does not include the object");
      }
  }

  void undo () override
  {
    // move object back
    auto clip_ref = target_owner_.remove_object (obj_ref_.id ());
    source_owner_.add_object (clip_ref);
  }
  void redo () override
  {
    // move object
    auto clip_ref = source_owner_.remove_object (obj_ref_.id ());
    target_owner_.add_object (clip_ref);
  }

private:
  structure::arrangement::ArrangerObjectUuidReference   obj_ref_;
  structure::arrangement::ArrangerObjectOwner<ObjectT> &target_owner_;
  structure::arrangement::ArrangerObjectOwner<ObjectT> &source_owner_;
};

} // namespace zrythm::commands
