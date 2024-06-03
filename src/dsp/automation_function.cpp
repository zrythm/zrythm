// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/automation_function.h"
#include "dsp/engine.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "zrythm_app.h"

static void
flip (AutomationSelections * sel, bool vertical)
{
  for (int i = 0; i < sel->num_automation_points; i++)
    {
      AutomationPoint * ap = sel->automation_points[i];

      if (vertical)
        {
          automation_point_set_fvalue (
            ap, 1.f - ap->normalized_val, F_NORMALIZED, F_NO_PUBLISH_EVENTS);
          ap->curve_opts.curviness = -ap->curve_opts.curviness;
        }
      else
        {
          /* TODO */
        }
    }
}

static void
flatten (AutomationSelections * sel)
{
  for (int i = 0; i < sel->num_automation_points; i++)
    {
      AutomationPoint * ap = sel->automation_points[i];

      ap->curve_opts.curviness = 1.0;
      ap->curve_opts.algo = CurveAlgorithm::PULSE;
    }
}

/**
 * Applies the given action to the given selections.
 *
 * @param sel Selections to edit.
 * @param type Function type.
 */
int
automation_function_apply (
  ArrangerSelections *   sel,
  AutomationFunctionType type,
  GError **              error)
{
  g_message ("applying %s...", automation_function_type_to_string (type));

  switch (type)
    {
    case AutomationFunctionType::AUTOMATION_FUNCTION_FLIP_HORIZONTAL:
      /* TODO */
      break;
    case AutomationFunctionType::AUTOMATION_FUNCTION_FLIP_VERTICAL:
      flip ((AutomationSelections *) sel, true);
      break;
    case AutomationFunctionType::AUTOMATION_FUNCTION_FLATTEN:
      flatten ((AutomationSelections *) sel);
      break;
    }

  /* set last action */
  g_settings_set_int (S_UI, "automation-function", ENUM_VALUE_TO_INT (type));

  EVENTS_PUSH (EventType::ET_EDITOR_FUNCTION_APPLIED, NULL);

  return 0;
}
