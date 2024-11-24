// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/automation_selections.h"
#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "dsp/position.h"
#include "gui/dsp/region.h"

AutomationSelections::AutomationSelections (QObject * parent)
    : QObject (parent), ArrangerSelections (Type::Automation)
{
}

bool
AutomationSelections::can_be_pasted_at_impl (const Position pos, const int idx)
  const
{
  if (!ArrangerSelections::can_be_pasted_at (pos))
    return false;

  auto r_opt = CLIP_EDITOR->get_region ();
  if (!r_opt || !std::holds_alternative<AutomationRegion *> (r_opt.value ()))
    return false;

  auto * r = std::get<AutomationRegion *> (r_opt.value ());
  if (r->pos_->frames_ + pos.frames_ < 0)
    return false;

  return true;
}

void
AutomationSelections::sort_by_indices (bool desc)
{
  std::sort (
    objects_.begin (), objects_.end (), [desc] (const auto &a, const auto &b) {
      bool ret = false;
      ret =
        std::get<AutomationPoint *> (a)->index_
        < std::get<AutomationPoint *> (b)->index_;
      return desc ? !ret : ret;
    });
}
