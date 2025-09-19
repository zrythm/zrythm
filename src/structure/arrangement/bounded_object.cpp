// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/bounded_object.h"

namespace zrythm::structure::arrangement
{
ArrangerObjectBounds::ArrangerObjectBounds (
  const dsp::AtomicPositionQmlAdapter &start_position,
  QObject *                            parent)
    : QObject (parent), position_ (start_position),
      length_ (start_position.position ().time_conversion_functions ()),
      length_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (length_, false, this))
{
}

void
init_from (
  ArrangerObjectBounds       &obj,
  const ArrangerObjectBounds &other,
  utils::ObjectCloneType      clone_type)
{
  obj.length_.set_ticks (other.length_.get_ticks ());
}
}
