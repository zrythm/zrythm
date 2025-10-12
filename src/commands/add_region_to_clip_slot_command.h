// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "structure/arrangement/arranger_object_all.h"
#include "structure/scenes/clip_slot.h"

#include <QUndoCommand>

namespace zrythm::commands
{

class AddRegionToClipSlotCommand : public QUndoCommand
{
public:
  AddRegionToClipSlotCommand (
    structure::scenes::ClipSlot                        &clip_slot,
    structure::arrangement::ArrangerObjectUuidReference object_ref)
      : QUndoCommand (QObject::tr ("Add Clip")), slot_ (clip_slot),
        object_ref_ (std::move (object_ref))
  {
  }

  void redo () override { slot_.setRegion (object_ref_.get_object_base ()); }
  void undo () override { slot_.clearRegion (); }

private:
  structure::scenes::ClipSlot                        &slot_;
  structure::arrangement::ArrangerObjectUuidReference object_ref_;
};

} // namespace zrythm::commands
