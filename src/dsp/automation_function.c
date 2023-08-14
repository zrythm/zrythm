// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 */

#include "dsp/automation_function.h"
#include "dsp/engine.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
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
            ap, 1.f - ap->normalized_val, F_NORMALIZED,
            F_NO_PUBLISH_EVENTS);
          ap->curve_opts.curviness = -ap->curve_opts.curviness;
        }
      else
        {
          /* TODO */
        }
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
  g_message (
    "applying %s...",
    automation_function_type_to_string (type));

  switch (type)
    {
    case AUTOMATION_FUNCTION_FLIP_HORIZONTAL:
      /* TODO */
      break;
    case AUTOMATION_FUNCTION_FLIP_VERTICAL:
      flip ((AutomationSelections *) sel, true);
      break;
    }

  /* set last action */
  g_settings_set_int (S_UI, "automation-function", type);

  EVENTS_PUSH (ET_EDITOR_FUNCTION_APPLIED, NULL);

  return 0;
}
