// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/colored_object.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
void
ArrangerObjectColor::setColor (QColor color)
{
  if (useColor () && color_.value ().to_qcolor () == color)
    return;

  if (!color.isValid ())
    return;

  const bool emit_use_color_changed = !useColor ();
  color_ = color;
  Q_EMIT colorChanged (color);
  if (emit_use_color_changed)
    Q_EMIT useColorChanged (true);
}

void
to_json (nlohmann::json &j, const ArrangerObjectColor &obj)
{
  j[ArrangerObjectColor::kColorKey] = obj.color_;
}

void
from_json (const nlohmann::json &j, ArrangerObjectColor &obj)
{
  j.at (ArrangerObjectColor::kColorKey).get_to (obj.color_);
}

void
init_from (
  ArrangerObjectColor       &obj,
  const ArrangerObjectColor &other,
  utils::ObjectCloneType     clone_type)
{
  obj.color_ = other.color_;
}
}
