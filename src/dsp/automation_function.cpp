// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/automation_function.h"
#include "dsp/engine.h"
#include "gui/cpp/backend/arranger_selections.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

static void
flip (AutomationSelections * sel, bool vertical)
{
  for (auto ap : sel->objects_ | type_is<AutomationPoint> ())
    {
      if (vertical)
        {
          ap->set_fvalue (1.f - ap->normalized_val_, true, false);
          ap->curve_opts_.curviness_ = -ap->curve_opts_.curviness_;
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
  for (auto ap : sel->objects_ | type_is<AutomationPoint> ())
    {

      ap->curve_opts_.curviness_ = 1.0;
      ap->curve_opts_.algo_ = CurveOptions::Algorithm::Pulse;
    }
}

void
automation_function_apply (AutomationSelections &sel, AutomationFunctionType type)
{
  z_debug ("applying {}...", AutomationFunctionType_to_string (type));

  switch (type)
    {
    case AutomationFunctionType::FlipHorizontal:
      /* TODO */
      break;
    case AutomationFunctionType::FlipVertical:
      flip (&sel, true);
      break;
    case AutomationFunctionType::Flatten:
      flatten (&sel);
      break;
    }

  /* set last action */
  g_settings_set_int (S_UI, "automation-function", ENUM_VALUE_TO_INT (type));

  EVENTS_PUSH (EventType::ET_EDITOR_FUNCTION_APPLIED, nullptr);
}
