// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <stdexcept>
#include <type_traits>
#include <utility>

#include "commands/arranger_object_owner_ref.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"

#include <QUndoCommand>

namespace zrythm::commands
{

template <structure::arrangement::FinalArrangerObjectSubclass ObjectT>
class AddArrangerObjectCommand : public QUndoCommand
{
public:
  /**
   * @brief QUndoCommand id for this addition.
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
      ? 1762954668
      : -1;

  /**
   * @param object_owner handle to the owner; must resolve to an
   * ArrangerObjectOwner<ObjectT>.
   * @param object_ref reference to the object being added.
   * @throw std::invalid_argument if the owner handle does not resolve to an
   * ArrangerObjectOwner<ObjectT>.
   */
  AddArrangerObjectCommand (
    ArrangerObjectOwnerRef                              object_owner,
    structure::arrangement::ArrangerObjectUuidReference object_ref)
      : QUndoCommand (QObject::tr ("Add Object")),
        object_owner_ (std::move (object_owner)),
        object_ref_ (std::move (object_ref))
  {
    object_owner_.resolve_or_throw<ObjectT> ();
  }

  /**
   * Convenience overload for owners that are not registry objects: the
   * owner is held as raw pointers and must outlive this command.
   */
  AddArrangerObjectCommand (
    structure::arrangement::ArrangerObjectOwner<ObjectT> &object_owner,
    structure::arrangement::ArrangerObjectUuidReference   object_ref)
      : AddArrangerObjectCommand (
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
    object_owner_.resolve<ObjectT> ()->remove_object (object_ref_.id ());
  }
  void redo () override
  {
    object_owner_.resolve<ObjectT> ()->add_object (object_ref_);
  }

  int id () const override { return CommandId; }

private:
  ArrangerObjectOwnerRef                              object_owner_;
  structure::arrangement::ArrangerObjectUuidReference object_ref_;
};

} // namespace zrythm::commands
