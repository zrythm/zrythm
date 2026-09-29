// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <utility>

#include "structure/arrangement/arranger_object_all.h"
#include "structure/scenes/clip_slot.h"

#include <QUndoCommand>

namespace zrythm::commands
{

class AddClipToClipSlotCommand : public QUndoCommand
{
public:
  AddClipToClipSlotCommand (
    structure::scenes::ClipSlotUuidReference            slot_ref,
    structure::arrangement::ArrangerObjectUuidReference object_ref)
      : QUndoCommand (QObject::tr ("Add Clip")),
        slot_ref_ (std::move (slot_ref)), object_ref_ (std::move (object_ref))
  {
  }

  void redo () override
  {
    // Capture whatever the slot held before
    if (!captured_previous_)
      {
        previous_clip_ = slot_ref_.get ()->clipReference ();
        captured_previous_ = true;
      }
    slot_ref_.get ()->setClip (
      object_ref_.get_object_as<structure::arrangement::Clip> ());
  }
  void undo () override
  {
    if (previous_clip_.has_value ())
      slot_ref_.get ()->setClip (
        previous_clip_->get_object_as<structure::arrangement::Clip> ());
    else
      slot_ref_.get ()->clearClip ();
  }

private:
  structure::scenes::ClipSlotUuidReference            slot_ref_;
  structure::arrangement::ArrangerObjectUuidReference object_ref_;
  std::optional<structure::arrangement::ArrangerObjectUuidReference>
       previous_clip_;
  bool captured_previous_{ false };
};

} // namespace zrythm::commands
