// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "dsp/automation_region.h"
#include "dsp/automation_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/channel.h"
#include "dsp/control_port.h"
#include "dsp/curve.h"
#include "dsp/midi_region.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/track.h"
#include "utils/rt_thread_id.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

void
automation_arranger_widget_create_ap (
  ArrangerWidget *   self,
  const Position *   pos,
  const double       start_y,
  AutomationRegion * region,
  bool               autofilling)
{
  auto at = region->get_automation_track ();
  if (!at)
    return;
  auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
  if (!port)
    return;

  if (!autofilling)
    {
      self->action = UiOverlayAction::CREATING_MOVING;
    }

  /* get local pos */
  Position local_pos (pos->ticks_ - region->pos_.ticks_);
  int height = gtk_widget_get_height (GTK_WIDGET (self));
  /* do height - because it's uside down */
  float normalized_val = static_cast<float> ((height - start_y) / height);
  z_debug ("normalized val is %f", static_cast<double> (normalized_val));

  /* clamp the value because the cursor might be outside the widget */
  normalized_val = std::clamp<float> (normalized_val, 0, 1);

  float value = port->normalized_val_to_real (normalized_val);

  /* create a new ap and add it to the automation region */
  auto ap = region->append_object (
    std::make_shared<AutomationPoint> (value, normalized_val, local_pos), true);

  /* set position to all counterparts */
  ap->set_position (&local_pos, ArrangerObject::PositionType::Start, false);

  /* set it as start object */
  self->prj_start_object = ap->shared_from_this ();
  self->start_object = ap->clone_unique ();

  EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, ap.get ());
  ArrangerObject::select (ap->shared_from_this (), true, false, false);
}

void
automation_arranger_widget_resize_curves (ArrangerWidget * self, double offset_y)
{
  double diff = (self->last_offset_y - offset_y) / 120.0;
  for (auto &obj : AUTOMATION_SELECTIONS->objects_)
    {
      if (auto ap = dynamic_cast<AutomationPoint *> (obj.get ()))
        {
          auto prj_ap =
            std::dynamic_pointer_cast<AutomationPoint> (ap->find_in_project ());
          double new_curve_val = std::clamp<double> (
            prj_ap->curve_opts_.curviness_ + diff, -1.0, 1.0);
          prj_ap->set_curviness (new_curve_val);

          /* make the change in the selection copy too */
          ap->set_curviness (new_curve_val);
        }
    }

  EVENTS_PUSH (
    EventType::ET_ARRANGER_SELECTIONS_CHANGED, AUTOMATION_SELECTIONS.get ());
}

GMenu *
automation_arranger_widget_gen_context_menu (
  ArrangerWidget * self,
  double           x,
  double           y)
{
  auto menu = Gio::Menu::create ();
  auto obj = arranger_widget_get_hit_arranger_object (
    self, ArrangerObject::Type::AutomationPoint, x, y);
  auto ap = dynamic_cast<AutomationPoint *> (obj);

  if (ap)
    {
      auto edit_submenu = Gio::Menu::create ();

      /* create cut, copy, duplicate, delete */
      edit_submenu->append (_ ("Cut"), "app.cut");
      edit_submenu->append (_ ("Copy"), "app.copy");
      edit_submenu->append (_ ("Duplicate"), "app.duplicate");
      edit_submenu->append (_ ("Delete"), "app.delete");

      edit_submenu->append (
        _ ("View info"),
        Glib::ustring::compose ("app.arranger-object-view-info::%1", obj));

      menu->append_section (Glib::RefPtr<Gio::MenuModel> (edit_submenu));

      /* add curve algorithm selection */
      auto curve_algorithm_submenu = Gio::Menu::create ();
      for (
        unsigned int i = 0;
        i < static_cast<unsigned int> (CurveOptions::Algorithm::Logarithmic) + 1;
        i++)
        {
          curve_algorithm_submenu->append (
            CurveOptions_Algorithm_to_string (
              ENUM_INT_TO_VALUE (CurveOptions::Algorithm, i), true),
            Glib::ustring::compose ("app.set-curve-algorithm(%1)", i));
        }

      menu->append_section (
        _ ("Curve algorithm"),
        Glib::RefPtr<Gio::MenuModel> (curve_algorithm_submenu));
    }

  auto gobj = menu->gobj ();
  g_object_ref (gobj);
  return gobj;
}

bool
automation_arranger_move_hit_aps (ArrangerWidget * self, double x, double y)
{
  int height = gtk_widget_get_height (GTK_WIDGET (self));

  /* get snapped x */
  Position pos = arranger_widget_px_to_pos (self, x, true);
  if (!self->shift_held && self->snap_grid->any_snap ())
    {
      pos.snap (
        self->earliest_obj_start_pos.get (), nullptr, nullptr, *self->snap_grid);
      x = arranger_widget_pos_to_px (self, pos, true);
    }

  /* move any hit automation points */
  auto obj = arranger_widget_get_hit_arranger_object (
    self, ArrangerObject::Type::AutomationPoint, x, -1);
  if (auto ap = dynamic_cast<AutomationPoint *> (obj))
    {
      if (automation_point_is_point_hit (*ap, x, -1))
        {
          ArrangerObject::select (ap->shared_from_this (), true, true, false);

          auto port = ap->get_port ();
          if (!port)
            return false;

          /* move it to the y value */
          /* do height - because it's uside down */
          float normalized_val = static_cast<float> ((height - y) / height);

          /* clamp the value because the cursor might be outside the widget */
          normalized_val = std::clamp<float> (normalized_val, 0.f, 1.f);
          float value = port->normalized_val_to_real (normalized_val);
          ap->set_fvalue (value, false, true);

          return true;
        }
    }

  return false;
}

void
automation_arranger_on_drag_end (ArrangerWidget * self)
{
  auto automation_sel_at_start =
    dynamic_cast<AutomationSelections *> (self->sel_at_start.get ());
  try
    {
      switch (self->action)
        {
        case UiOverlayAction::RESIZING_UP:
          {
            try
              {
                UNDO_MANAGER->perform (
                  std::make_unique<ArrangerSelectionsAction::EditAction> (
                    *self->sel_at_start, AUTOMATION_SELECTIONS.get (),
                    ArrangerSelectionsAction::EditType::Primitive, true));
              }
            catch (const ZrythmException &e)
              {
                e.handle (_ ("Failed to edit selection"));
              }
          }
          break;
        case UiOverlayAction::STARTING_MOVING:
          break;
        case UiOverlayAction::MOVING:
          {
            const AutomationPoint * start_ap =
              automation_sel_at_start->get_automation_point (0);
            const AutomationPoint * ap =
              AUTOMATION_SELECTIONS->get_automation_point (0);
            double ticks_diff = ap->pos_.ticks_ - start_ap->pos_.ticks_;
            double norm_value_diff =
              (double) (ap->normalized_val_ - start_ap->normalized_val_);
            UNDO_MANAGER->perform (
              std::make_unique<
                ArrangerSelectionsAction::MoveOrDuplicateAutomationAction> (
                *AUTOMATION_SELECTIONS, true, ticks_diff, norm_value_diff, true));
          }
          break;
          /* if copy-moved */
        case UiOverlayAction::MOVING_COPY:
          {
            auto start_ap =
              dynamic_cast<AutomationPoint *> (self->start_object.get ());
            double ticks_diff =
              start_ap->pos_.ticks_ - start_ap->transient_->pos_.ticks_;
            float value_diff =
              start_ap->normalized_val_
              - dynamic_cast<AutomationPoint *> (start_ap->transient_)
                  ->normalized_val_;

            UNDO_MANAGER->perform (
              std::make_unique<
                ArrangerSelectionsAction::MoveOrDuplicateAutomationAction> (
                *AUTOMATION_SELECTIONS, false, ticks_diff, value_diff, true));
          }
          break;
        case UiOverlayAction::NONE:
        case UiOverlayAction::STARTING_SELECTION:
          {
            AUTOMATION_SELECTIONS->clear (false);
          }
          break;
        /* if something was created */
        case UiOverlayAction::CREATING_MOVING:
          {
            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::CreateAction> (
                *AUTOMATION_SELECTIONS));
          }
          break;
        case UiOverlayAction::DELETE_SELECTING:
        case UiOverlayAction::ERASING:
          arranger_widget_handle_erase_action (self);
          break;
        case UiOverlayAction::AUTOFILLING:
          {
            auto region = CLIP_EDITOR->get_region<AutomationRegion> ();
            z_return_if_fail (region);
            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::AutomationFillAction> (
                *self->region_at_start, *region, true));
          }
          break;
        /* if didn't click on something */
        default:
          break;
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to perform action"));
    }

  self->start_object = nullptr;
}