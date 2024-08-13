// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "dsp/midi_region.h"
#include "dsp/transport.h"
#include "dsp/velocity.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/dialogs/arranger_object_info.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/track.h"
#include "settings/g_settings_manager.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/rt_thread_id.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

void
midi_arranger_widget_create_note (
  ArrangerWidget * self,
  const Position   pos,
  int              note,
  MidiRegion      &region)
{
  /* get local pos */
  Position local_pos{ pos.ticks_ - region.pos_.ticks_ };

  /* set action */
  bool autofilling = self->action == UiOverlayAction::AUTOFILLING;
  if (!autofilling)
    {
      bool drum_mode = arranger_widget_get_drum_mode_enabled (self);
      if (drum_mode)
        self->action = UiOverlayAction::MOVING;
      else
        self->action = UiOverlayAction::CREATING_RESIZING_R;
    }

  int default_velocity_type =
    g_settings_get_enum (S_UI, "piano-roll-default-velocity");
  midi_byte_t velocity = VELOCITY_DEFAULT;
  switch (default_velocity_type)
    {
    case 0:
      velocity =
        (midi_byte_t) g_settings_get_int (S_UI, "piano-roll-last-set-velocity");
      break;
    case 1:
      velocity = 40;
      break;
    case 2:
      velocity = 90;
      break;
    case 3:
      velocity = 120;
      break;
    default:
      break;
    }

  /* add a new note to the region */
  auto midi_note = region.append_object (
    std::make_shared<MidiNote> (region.id_, local_pos, local_pos, note, velocity),
    true);

  /* set end pos */
  Position tmp;
  Position::set_min_size (midi_note->pos_, tmp, *self->snap_grid);
  midi_note->set_position (&tmp, ArrangerObject::PositionType::End, false);

  /* set as start object */
  arranger_widget_set_start_object (self, midi_note);

  EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, midi_note.get ());

  /* select it */
  midi_note->select (true, autofilling, false);

  midi_note->listen (true);
}

bool
midi_arranger_widget_snap_midi_notes_l (
  ArrangerWidget * self,
  const Position   pos,
  bool             dry_run)
{
  auto region = CLIP_EDITOR->get_region<MidiRegion> ();

  /* get delta with first clicked note's start pos */
  double delta =
    pos.ticks_ - (self->start_object->pos_.ticks_ + region->pos_.ticks_);
  z_debug ("delta %f", delta);

  for (auto midi_note : MIDI_SELECTIONS->objects_ | type_is<MidiNote> ())
    {
      /* calculate new start pos */
      Position new_start_pos = midi_note->pos_;
      new_start_pos.add_ticks (delta);

      /* get the global star pos first to snap it */
      Position new_global_start_pos = new_start_pos;
      new_global_start_pos.add_ticks (region->pos_.ticks_);

      /* snap the global pos */
      if (
        self->snap_grid->any_snap () && !self->shift_held
        && new_global_start_pos.is_positive ())
        {
          new_global_start_pos.snap (
            self->earliest_obj_start_pos.get (), nullptr, region,
            *self->snap_grid);
        }

      /* convert it back to a local pos */
      new_start_pos = new_global_start_pos;
      new_start_pos.add_ticks (-region->pos_.ticks_);

      if (
        !new_global_start_pos.is_positive ()
        || new_start_pos >= midi_note->end_pos_)
        {
          return false;
        }
      else if (!dry_run)
        {
          midi_note->pos_setter (&new_start_pos);
        }
    }

  EVENTS_PUSH (
    EventType::ET_ARRANGER_SELECTIONS_CHANGED, MIDI_SELECTIONS.get ());

  return true;
}

void
midi_arranger_widget_set_hovered_note (ArrangerWidget * self, int pitch)
{
  if (self->hovered_note != pitch)
    {
      GdkRectangle rect;
      arranger_widget_get_visible_rect (self, &rect);
      double adj_px_per_key = MW_PIANO_ROLL_KEYS->px_per_key + 1.0;
      if (self->hovered_note != -1)
        {
          /* redraw the previous note area to
           * unhover it */
          rect.y =
            (int) (adj_px_per_key * (127.0 - (double) self->hovered_note) - 1.0);
          rect.height = (int) adj_px_per_key;
        }
      self->hovered_note = pitch;

      if (pitch != -1)
        {
          /* redraw newly hovered note area */
          rect.y = (int) (adj_px_per_key * (127.0 - (double) pitch) - 1);
          rect.height = (int) adj_px_per_key;
        }
    }
}

bool
midi_arranger_widget_snap_midi_notes_r (
  ArrangerWidget * self,
  const Position   pos,
  bool             dry_run)
{
  auto region = CLIP_EDITOR->get_region<MidiRegion> ();

  /* get delta with first clicked notes's end pos */
  auto start_object_lo =
    dynamic_cast<LengthableObject *> (self->start_object.get ());
  double delta =
    pos.ticks_ - (start_object_lo->end_pos_.ticks_ + region->pos_.ticks_);
  z_debug ("delta %f", delta);

  for (auto midi_note : MIDI_SELECTIONS->objects_ | type_is<MidiNote> ())
    {
      /* get new end pos by adding delta to the cached end pos */
      Position new_end_pos = midi_note->end_pos_;
      new_end_pos.add_ticks (delta);

      /* get the global end pos first to snap it */
      Position new_global_end_pos = new_end_pos;
      new_global_end_pos.add_ticks (region->pos_.ticks_);

      /* snap the global pos */
      if (
        self->snap_grid->any_snap () && !self->shift_held
        && new_global_end_pos.is_positive ())
        {
          new_global_end_pos.snap (
            self->earliest_obj_start_pos.get (), nullptr, region,
            *self->snap_grid);
        }

      /* convert it back to a local pos */
      new_end_pos = new_global_end_pos;
      new_end_pos.add_ticks (-region->pos_.ticks_);

      if (new_end_pos <= midi_note->pos_)
        return false;
      else if (!dry_run)
        {
          midi_note->end_pos_setter (&new_end_pos);
        }
    }

  EVENTS_PUSH (
    EventType::ET_ARRANGER_SELECTIONS_CHANGED, MIDI_SELECTIONS.get ());

  return true;
}

int
midi_arranger_calc_deltamax_for_note_movement (int y_delta)
{
  for (auto midi_note : MIDI_SELECTIONS->objects_ | type_is<MidiNote> ())
    {
      if (midi_note->val_ + y_delta < 0)
        {
          y_delta = 0;
        }
      else if (midi_note->val_ + y_delta >= 127)
        {
          y_delta = 127 - midi_note->val_;
        }
    }
  return y_delta;
}

gboolean
midi_arranger_unlisten_notes_source_func (gpointer user_data)
{
  ArrangerWidget * self = Z_ARRANGER_WIDGET (user_data);
  midi_arranger_listen_notes (self, false);
  return G_SOURCE_REMOVE;
}

void
midi_arranger_listen_notes (ArrangerWidget * self, bool listen)
{
  if (!g_settings_get_boolean (S_UI, "listen-notes"))
    return;

  auto sel = arranger_widget_get_selections<MidiSelections> (self);
  auto [start_obj, start_pos] = sel->get_first_object_and_pos (false);
  double ticks_cutoff = start_pos.ticks_ + (double) TRANSPORT->ticks_per_beat_;
  for (auto midi_note : sel->objects_ | type_is<MidiNote> ())
    {
      /* only add notes during the first beat of the selections if listening */
      if (!listen || midi_note->pos_.ticks_ < ticks_cutoff)
        midi_note->listen (listen);
    }
}

GMenu *
midi_arranger_widget_gen_context_menu (ArrangerWidget * self, double x, double y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  auto midi_note =
    dynamic_cast<MidiNote *> (arranger_widget_get_hit_arranger_object (
      self, ArrangerObject::Type::MidiNote, x, y));

  if (midi_note)
    {
      bool selected = midi_note->is_selected ();
      if (!selected)
        {
          midi_note->select (true, false, false);
        }

      menuitem = CREATE_CUT_MENU_ITEM ("app.cut");
      g_menu_append_item (menu, menuitem);
      menuitem = CREATE_COPY_MENU_ITEM ("app.copy");
      g_menu_append_item (menu, menuitem);
      menuitem = CREATE_PASTE_MENU_ITEM ("app.paste");
      g_menu_append_item (menu, menuitem);
      menuitem = CREATE_DELETE_MENU_ITEM ("app.delete");
      g_menu_append_item (menu, menuitem);
      menuitem = CREATE_DUPLICATE_MENU_ITEM ("app.duplicate");
      g_menu_append_item (menu, menuitem);

      char str[100];
      sprintf (str, "app.arranger-object-view-info::%p", midi_note);
      menuitem = z_gtk_create_menu_item (_ ("View info"), nullptr, str);
      g_menu_append_item (menu, menuitem);
    }
  else
    {
      arranger_widget_select_all (
        (ArrangerWidget *) self, F_NO_SELECT, F_PUBLISH_EVENTS);
      MIDI_SELECTIONS->clear (false);

      menuitem = CREATE_PASTE_MENU_ITEM ("app.paste");
      g_menu_append_item (menu, menuitem);
    }

  GMenu * selection_submenu = g_menu_new ();
  menuitem = CREATE_CLEAR_SELECTION_MENU_ITEM ("app.clear-selection");
  g_menu_append_item (selection_submenu, menuitem);
  menuitem = CREATE_SELECT_ALL_MENU_ITEM ("app.select-all");
  g_menu_append_item (selection_submenu, menuitem);

  g_menu_append_section (menu, nullptr, G_MENU_MODEL (selection_submenu));

  return menu;
}

/**
 * @param hover_y Current hover y-value, or -1 to use the
 *   topmost visible value.
 */
static void
do_vertical_zoom (ArrangerWidget * self, double hover_y, double delta_y)
{
  /* get current adjustment so we can get the difference from the cursor */
  auto settings = arranger_widget_get_editor_settings (self);
  std::visit (
    [&] (auto &&settings) {
      double adj_val = settings->scroll_start_y_;
      double size_before =
        gtk_widget_get_height (GTK_WIDGET (MW_PIANO_ROLL_KEYS));
      if (hover_y < 0)
        {
          hover_y = adj_val;
        }
      double adj_perc = hover_y / size_before;

      /* get px diff so we can calculate the new
       * adjustment later */
      double diff = hover_y - adj_val;

      /* scroll down, zoom out */
      double size_after;
      float  notes_zoom_before = PIANO_ROLL->notes_zoom_;
      double _multiplier = 1.16;
      double multiplier = delta_y > 0 ? 1 / _multiplier : _multiplier;
      PIANO_ROLL->set_notes_zoom (
        PIANO_ROLL->notes_zoom_ * (float) multiplier, false);
      size_after = size_before * multiplier;

      if (math_floats_equal (PIANO_ROLL->notes_zoom_, notes_zoom_before))
        {
          size_after = size_before;
        }

      /* refresh relevant widgets */
      midi_editor_space_widget_refresh (MW_MIDI_EDITOR_SPACE);

      /* set value at the same offset as before */
      settings->set_scroll_start_y ((int) (adj_perc * size_after - diff), false);
    },
    settings);
}

void
midi_arranger_handle_vertical_zoom_action (ArrangerWidget * self, bool zoom_in)
{
  do_vertical_zoom (self, -1, zoom_in ? -1 : 1);
}

/**
 * Handle ctrl+shift+scroll.
 */
void
midi_arranger_handle_vertical_zoom_scroll (
  ArrangerWidget *           self,
  GtkEventControllerScroll * scroll_controller,
  double                     dy)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (scroll_controller));

  if (!(state & GDK_CONTROL_MASK && state & GDK_SHIFT_MASK))
    return;

  double y = self->hover_y;
  double delta_y = dy;

  do_vertical_zoom (self, y, delta_y);
}

void
midi_arranger_on_drag_end (ArrangerWidget * self)
{
  midi_arranger_listen_notes (self, 0);

  auto start_mn_in_prj =
    std::dynamic_pointer_cast<MidiNote> (self->prj_start_object.lock ());
  auto start_mn_transient =
    dynamic_cast<MidiNote *> (start_mn_in_prj->transient_);
  double ticks_diff = 0;
  double end_ticks_diff = 0;
  int    pitch_diff = 0;
  if (start_mn_in_prj && start_mn_transient)
    {
      ticks_diff =
        start_mn_in_prj->pos_.ticks_ - start_mn_transient->pos_.ticks_;
      end_ticks_diff =
        start_mn_in_prj->end_pos_.ticks_ - start_mn_transient->end_pos_.ticks_;
      pitch_diff = start_mn_in_prj->val_ - start_mn_transient->val_;
    }

  try
    {
      switch (self->action)
        {
        case UiOverlayAction::RESIZING_L:
          {
            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::ResizeAction> (
                *self->sel_at_start, MIDI_SELECTIONS.get (),
                ArrangerSelectionsAction::ResizeType::L, ticks_diff));
          }
          break;
        case UiOverlayAction::RESIZING_R:
          {
            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::ResizeAction> (
                *self->sel_at_start, MIDI_SELECTIONS.get (),
                ArrangerSelectionsAction::ResizeType::R, end_ticks_diff));
            if (pitch_diff)
              {
                UNDO_MANAGER->perform (
                  std::make_unique<ArrangerSelectionsAction::MoveMidiAction> (
                    *MIDI_SELECTIONS, 0, pitch_diff, true));
                auto last_action = UNDO_MANAGER->get_last_action ();
                last_action->num_actions_ = 2;
              }
          }
          break;
        case UiOverlayAction::STARTING_MOVING:
          {
            /* if something was clicked with ctrl without moving*/
            if (self->ctrl_held)
              {
                if (start_mn_in_prj && self->start_object_was_selected)
                  {
                    /* deselect it */
                    start_mn_in_prj->select (false, true, false);
                  }
              }
          }
          break;
        case UiOverlayAction::MOVING:
          {
            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::MoveMidiAction> (
                *MIDI_SELECTIONS, ticks_diff, pitch_diff, true));
          }
          break;
        /* if copy/link-moved */
        case UiOverlayAction::MOVING_COPY:
          {
            UNDO_MANAGER->perform (
              std::make_unique<
                ArrangerSelectionsAction::MoveOrDuplicateMidiAction> (
                *MIDI_SELECTIONS, false, ticks_diff, pitch_diff, true));
          }
          break;
        case UiOverlayAction::NONE:
          {
            MIDI_SELECTIONS->clear (false);
          }
          break;
        /* something was created */
        case UiOverlayAction::CREATING_RESIZING_R:
        case UiOverlayAction::AUTOFILLING:
          {
            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::CreateAction> (
                *MIDI_SELECTIONS));
          }
          break;
        case UiOverlayAction::DELETE_SELECTING:
        case UiOverlayAction::ERASING:
          arranger_widget_handle_erase_action (self);
          break;
        /* if didn't click on something */
        default:
          {
          }
          break;
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to perform MIDI arranger action"));
    }

  EVENTS_PUSH (
    EventType::ET_ARRANGER_SELECTIONS_CHANGED, MIDI_SELECTIONS.get ());
}