// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/position.h"
#include "common/dsp/region.h"
#include "gui/backend/backend/automation_selections.h"
#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

AutomationSelections::AutomationSelections ()
    : ArrangerSelections (Type::Automation)
{
}

bool
AutomationSelections::can_be_pasted_at_impl (const Position pos, const int idx)
  const
{
  if (!ArrangerSelections::can_be_pasted_at (pos))
    return false;

  Region * r = CLIP_EDITOR->get_region ();
  if (!r || !r->is_automation ())
    return false;

  if (r->pos_.frames_ + pos.frames_ < 0)
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
        dynamic_cast<AutomationPoint &> (*a).index_
        < dynamic_cast<AutomationPoint &> (*b).index_;
      return desc ? !ret : ret;
    });
}