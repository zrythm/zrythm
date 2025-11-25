// SPDX-FileCopyrightText: © 2020, 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/settings_manager.h"
#include "structure/arrangement/automation_function.h"

namespace zrythm::structure::arrangement
{

void
AutomationFunction::apply (ArrangerObjectSpan sel_var, Type type)
{
  z_debug ("applying {}...", AutomationFunctionType_to_string (type));

  const auto &sel = sel_var;
  const auto  flip = [&] (const bool vertical) {
    for (auto * ap : sel.template get_elements_by_type<AutomationPoint> ())
      {
        if (vertical)
          {
            ap->setValue (1.f - ap->value ());
            ap->curveOpts ()->setCurviness (-ap->curveOpts ()->curviness ());
          }
        else
          {
            /* TODO */
          }
      }
  };

  const auto flatten = [&] () {
    for (auto * ap : sel.template get_elements_by_type<AutomationPoint> ())
      {

        ap->curveOpts ()->setCurviness (1.0);
        ap->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Pulse);
      }
  };

  switch (type)
    {
    case Type::FlipHorizontal:
      /* TODO */
      break;
    case Type::FlipVertical:
      flip (true);
      break;
    case Type::Flatten:
      flatten ();
      break;
    }

  /* set last action */
  gui::SettingsManager::get_instance ()->set_lastAutomationFunction (
    ENUM_VALUE_TO_INT (type));

  // EVENTS_PUSH (EventType::ET_EDITOR_FUNCTION_APPLIED, nullptr);
}
}
