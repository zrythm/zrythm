// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <type_traits>
#include <utility>

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"

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

  RemoveArrangerObjectCommand (
    structure::arrangement::ArrangerObjectOwner<ObjectT> &object_owner,
    structure::arrangement::ArrangerObjectUuidReference   object_ref)
      : QUndoCommand (QObject::tr ("Remove Object")),
        object_owner_ (object_owner), object_ref_ (std::move (object_ref))
  {
  }

  void undo () override { object_owner_.add_object (object_ref_); }
  void redo () override { object_owner_.remove_object (object_ref_.id ()); }

  int id () const override { return CommandId; }

private:
  structure::arrangement::ArrangerObjectOwner<ObjectT> &object_owner_;
  structure::arrangement::ArrangerObjectUuidReference   object_ref_;
};

} // namespace zrythm::commands
