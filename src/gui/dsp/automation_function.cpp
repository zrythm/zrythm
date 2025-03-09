// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/arranger_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/automation_function.h"
#include "gui/dsp/engine.h"

#include "utils/flags.h"
#include "utils/rt_thread_id.h"

using namespace zrythm;

static void
flip (AutomationSelections * sel, bool vertical)
{
  for (
    auto * ap : sel->get_object_span ().get_elements_by_type<AutomationPoint> ())
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
  for (
    auto * ap : sel->get_object_span ().get_elements_by_type<AutomationPoint> ())
    {

      ap->curve_opts_.curviness_ = 1.0;
      ap->curve_opts_.algo_ = dsp::CurveOptions::Algorithm::Pulse;
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
  gui::SettingsManager::get_instance ()->set_lastAutomationFunction (
    ENUM_VALUE_TO_INT (type));

  // EVENTS_PUSH (EventType::ET_EDITOR_FUNCTION_APPLIED, nullptr);
}
