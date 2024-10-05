// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "dsp/region.h"
#include "gui/cpp/backend/automation_selections.h"
#include "gui/cpp/backend/clip_editor.h"
#include "project.h"
#include "zrythm.h"

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