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
class AddArrangerObjectCommand : public QUndoCommand
{
public:
  AddArrangerObjectCommand (
    structure::arrangement::ArrangerObjectOwner<ObjectT> &object_owner,
    structure::arrangement::ArrangerObjectUuidReference   object_ref)
      : QUndoCommand (QObject::tr ("Add Object")), object_owner_ (object_owner),
        object_ref_ (std::move (object_ref))
  {
  }

  void undo () override { object_owner_.remove_object (object_ref_.id ()); }
  void redo () override { object_owner_.add_object (object_ref_); }

private:
  structure::arrangement::ArrangerObjectOwner<ObjectT> &object_owner_;
  structure::arrangement::ArrangerObjectUuidReference   object_ref_;
};

/**
 * @brief Specialization of AddArrangerObjectCommand with a custom ID to be used
 * for special handling by the undo stack (for example to require the engine to
 * be stopped).
 */
template <structure::arrangement::FinalArrangerObjectSubclass ObjectT>
class AddTempoMapAffectingArrangerObjectCommand
    : public AddArrangerObjectCommand<ObjectT>
{
public:
  static constexpr auto CommandId = 1762954668;
  using AddArrangerObjectCommand<ObjectT>::AddArrangerObjectCommand;

  int id () const override { return CommandId; }
};

} // namespace zrythm::commands
