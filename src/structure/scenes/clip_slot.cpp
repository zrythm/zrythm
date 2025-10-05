// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/clip_slot.h"

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
}
