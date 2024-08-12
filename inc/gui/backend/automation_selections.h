// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * API for selections in the AutomationArrangerWidget.
 */

#ifndef __GUI_BACKEND_AUTOMATION_SELECTIONS_H__
#define __GUI_BACKEND_AUTOMATION_SELECTIONS_H__

#include "dsp/automation_point.h"
#include "gui/backend/arranger_selections.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUTOMATION_SELECTIONS (PROJECT->automation_selections_)

/**
 * Selections to be used for the AutomationArrangerWidget's current selections,
 * copying, undoing, etc.
 */
class AutomationSelections final
    : public ArrangerSelections,
      public ICloneable<AutomationSelections>,
      public ISerializable<AutomationSelections>
{
public:
  const AutomationPoint * get_automation_point (int index) const
  {
    return dynamic_cast<AutomationPoint *> (objects_[index].get ());
  }

  void sort_by_indices (bool desc) override;

  void init_after_cloning (const AutomationSelections &other) override
  {
    ArrangerSelections::copy_members_from (other);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool can_be_pasted_at_impl (const Position pos, const int idx) const final;
};

/**
 * @}
 */

#endif
