// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "dsp/midi_note.h"
#include "dsp/velocity.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/ruler.h"
#include "settings/g_settings_manager.h"
#include "utils/flags.h"
#include "utils/rt_thread_id.h"
#include "zrythm_app.h"

void
midi_modifier_arranger_widget_set_start_vel (ArrangerWidget * self)
{
  auto region = CLIP_EDITOR->get_region<MidiRegion> ();
  z_return_if_fail (region);

  for (auto &mn : region->midi_notes_)
    {
      auto &vel = mn->vel_;
      vel->vel_at_start_ = vel->vel_;

      /* do the same for selections */
      auto selections_mn = MIDI_SELECTIONS->find_object (*mn);
      selections_mn->vel_->vel_at_start_ = vel->vel_;
    }
}

/**
 * Returns the enclosed velocities.
 *
 * @param hit Return hit or unhit velocities.
 */
static auto
get_enclosed_velocities (ArrangerWidget * self, double offset_x, bool hit)
{
  Position selection_start_pos = ui_px_to_pos_editor (self->start_x, F_PADDING);
  Position selection_end_pos =
    ui_px_to_pos_editor (self->start_x + offset_x, F_PADDING);
  if (offset_x < 0)
    std::swap (selection_start_pos, selection_end_pos);

  std::vector<Velocity *> velocities;
  auto                    region = CLIP_EDITOR->get_region<MidiRegion> ();
  z_return_val_if_fail (region && region->is_midi (), velocities);

  /* find enclosed velocities */
  region->get_velocities_in_range (
    &selection_start_pos, &selection_end_pos, velocities, hit);

  return velocities;
}

void
midi_modifier_arranger_widget_select_vels_in_range (
  ArrangerWidget * self,
  double           offset_x)
{
  auto velocities = get_enclosed_velocities (self, offset_x, true);
  MIDI_SELECTIONS->clear (false);
  for (auto vel : velocities)
    {
      auto mn = vel->get_midi_note ();
      mn->select (true, true, false);
    }
}

void
midi_modifier_arranger_widget_ramp (
  ArrangerWidget * self,
  double           offset_x,
  double           offset_y)
{
  /* find enclosed velocities */
  auto velocities = get_enclosed_velocities (self, offset_x, true);

  /* ramp */
  const int height = gtk_widget_get_height (GTK_WIDGET (self));
  for (auto vel : velocities)
    {
      auto     mn = vel->get_midi_note ();
      Position start_pos;
      mn->get_global_start_pos (start_pos);
      auto px = ui_pos_to_px_editor (start_pos, F_PADDING);

      auto x1 = self->start_x;
      auto x2 = self->start_x + offset_x;
      auto y1 = height - self->start_y;
      auto y2 = height - (self->start_y + offset_y);
      /*z_info ("x1 %f.0 x2 %f.0 y1 %f.0 y2 %f.0",*/
      /*x1, x2, y1, y2);*/

      /* y = y1 + ((y2 - y1)/(x2 - x1))*(x - x1)
       * http://stackoverflow.com/questions/2965144/ddg#2965188 */
      /* get val in pixels */
      auto val = (int) (y1 + ((y2 - y1) / (x2 - x1)) * ((double) px - x1));

      /* normalize and multiply by 127 to get velocity value */
      val = (int) (((double) val / (double) height) * 127.0);
      val = std::clamp<int> (val, 1, 127);
      /*z_info ("val %d", val);*/

      vel->set_val (val);

      /* do the same for selections */
      auto selection_mn = MIDI_SELECTIONS->find_object (*mn);
      if (selection_mn)
        {
          selection_mn->vel_->set_val (val);
        }
    }

  /* find velocities not hit */
  velocities = get_enclosed_velocities (self, offset_x, false);

  /* reset their value */
  for (auto vel : velocities)
    {
      vel->set_val (vel->vel_at_start_);

      /* do the same for selections */
      auto selection_mn = MIDI_SELECTIONS->find_object (*vel->get_midi_note ());
      if (selection_mn)
        {
          selection_mn->vel_->set_val (vel->vel_at_start_);
        }
    }

  EVENTS_PUSH (EventType::ET_VELOCITIES_RAMPED, nullptr);
}

void
midi_modifier_arranger_widget_resize_velocities (
  ArrangerWidget * self,
  double           offset_y)
{
  int height = gtk_widget_get_height (GTK_WIDGET (self));

  /* adjust for circle radius */
  double start_y = self->start_y;

  double start_ratio = CLAMP (1.0 - start_y / (double) height, 0.0, 1.0);
  double ratio = CLAMP (1.0 - (start_y + offset_y) / (double) height, 0.0, 1.0);
  z_return_if_fail (start_ratio <= 1.0);
  z_return_if_fail (ratio <= 1.0);
  int start_val = (int) (start_ratio * 127.0);
  int val = (int) (ratio * 127.0);
  self->vel_diff = val - start_val;

  for (auto mn : MIDI_SELECTIONS->objects_ | type_is<MidiNote> ())
    {
      auto &prj_vel =
        dynamic_cast<MidiNote *> (mn->find_in_project ().get ())->vel_;
      prj_vel->set_val (
        std::clamp<int> (prj_vel->vel_at_start_ + self->vel_diff, 1, 127));
      EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CHANGED, prj_vel.get ());

      /* make the change in the selection copies too */
      auto &selections_vel = mn->vel_;
      selections_vel->vel_ = prj_vel->vel_;
      selections_vel->vel_at_start_ = prj_vel->vel_at_start_;
    }

  if (self->start_object)
    {
      z_return_if_fail (
        self->start_object->type_ == ArrangerObject::Type::Velocity);
      auto vel = dynamic_cast<Velocity *> (
        self->start_object->find_in_project ().get ());
      g_settings_set_int (S_UI, "piano-roll-last-set-velocity", vel->vel_);
    }
}

void
midi_modifier_arranger_set_hit_velocity_vals (
  ArrangerWidget * self,
  double           x,
  double           y,
  bool             append_to_selections)
{
  std::vector<ArrangerObject *> objs;
  arranger_widget_get_hit_objects_at_point (
    self, ArrangerObject::Type::Velocity, x, -1, objs);
  z_debug ("{} velocities hit", objs.size ());

  int    height = gtk_widget_get_height (GTK_WIDGET (self));
  double ratio = 1.0 - y / (double) height;
  int    val = std::clamp<int> ((int) (ratio * 127.0), 1, 127);

  for (auto vel : objs | type_is<Velocity> ())
    {
      auto mn = vel->get_midi_note ();

      /* if object not already selected, add to selections */
      if (!mn->is_selected ())
        {
          /* add a clone of midi note before the change to sel_at_start */
          self->sel_at_start->add_object_owned (mn->clone_unique ());

          if (append_to_selections)
            {
              mn->select (true, true, false);
            }
        }

      vel->set_val (val);
    }
}

void
midi_modifier_arranger_on_drag_end (ArrangerWidget * self)
{
  switch (self->action)
    {
    case UiOverlayAction::RESIZING_UP:
    case UiOverlayAction::RAMPING:
    case UiOverlayAction::AUTOFILLING:
      {
        try
          {
            auto sel_before =
              self->sel_at_start
                ? static_cast<MidiSelections *> (self->sel_at_start.get ())
                    ->clone_unique ()
                : std::make_unique<MidiSelections> ();
            auto sel_after = MIDI_SELECTIONS->clone_unique ();

            if (self->action == UiOverlayAction::RAMPING)
              {
                Position selection_start_pos =
                  ui_px_to_pos_editor (self->start_x, true);
                Position selection_end_pos = ui_px_to_pos_editor (
                  self->start_x + self->last_offset_x, true);
                if (self->last_offset_x < 0)
                  std::swap (selection_start_pos, selection_end_pos);

                midi_modifier_arranger_widget_select_vels_in_range (
                  self, self->last_offset_x);
                sel_before = MIDI_SELECTIONS->clone_unique ();
                for (auto &mn : sel_before->objects_)
                  {
                    auto midi_note = dynamic_cast<MidiNote *> (mn.get ());
                    midi_note->vel_->vel_ = midi_note->vel_->vel_at_start_;
                  }
              }

            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::EditAction> (
                *sel_before, sel_after.get (),
                ArrangerSelectionsAction::EditType::Primitive, false));
          }
        catch (const ZrythmException &e)
          {
            e.handle (_ ("Failed to edit selections"));
          }
      }
      break;
    case UiOverlayAction::DELETE_SELECTING:
    case UiOverlayAction::ERASING:
      arranger_widget_handle_erase_action (self);
      break;
    default:
      break;
    }
}