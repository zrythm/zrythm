// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/actions/arranger_selections.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/arranger_object.h"
#include "gui/cpp/gtk_widgets/bot_dock_edge.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/clip_editor.h"
#include "gui/cpp/gtk_widgets/clip_editor_inner.h"
#include "gui/cpp/gtk_widgets/dialogs/arranger_object_info.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/midi_arranger.h"
#include "gui/cpp/gtk_widgets/midi_editor_space.h"
#include "gui/cpp/gtk_widgets/midi_modifier_arranger.h"
#include "gui/cpp/gtk_widgets/piano_roll_keys.h"
#include "gui/cpp/gtk_widgets/ruler.h"
#include "gui/cpp/gtk_widgets/timeline_bg.h"
#include "gui/cpp/gtk_widgets/track.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/midi_region.h"
#include "common/dsp/transport.h"
#include "common/dsp/velocity.h"
#include "common/utils/flags.h"
#include "common/utils/gtk.h"
#include "common/utils/math.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/ui.h"

ArrangerCursor
midi_arranger_widget_get_cursor (ArrangerWidget * self, Tool tool)
{
  ArrangerCursor ac = ArrangerCursor::Select;
  // UiOverlayAction action = self->action;

  const ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
    (ArrangerWidget *) self, ArrangerObject::Type::MidiNote, self->hover_x,
    self->hover_y);
  const int is_hit = obj != nullptr;

  const bool drum_mode = arranger_widget_get_drum_mode_enabled (self);

  switch (self->action)
    {
    case UiOverlayAction::None:
      if (tool == Tool::Select || tool == Tool::Edit)
        {
          int is_resize_l = 0, is_resize_r = 0;

          if (is_hit)
            {
              is_resize_l = arranger_object_is_resize_l (
                obj, (int) self->hover_x - obj->full_rect_.x);
              is_resize_r = arranger_object_is_resize_r (
                obj, (int) self->hover_x - obj->full_rect_.x);
            }

          if (is_hit && is_resize_l && !drum_mode)
            {
              return ArrangerCursor::ResizingL;
            }
          else if (is_hit && is_resize_r && !drum_mode)
            {
              return ArrangerCursor::ResizingR;
            }
          else if (is_hit)
            {
              return ArrangerCursor::Grab;
            }
          else
            {
              /* set cursor to whatever it is */
              if (tool == Tool::Edit)
                return ArrangerCursor::Edit;
              else
                return ac;
            }
        }
      else if (P_TOOL == Tool::Edit)
        ac = ArrangerCursor::Edit;
      else if (P_TOOL == Tool::Eraser)
        ac = ArrangerCursor::Eraser;
      else if (P_TOOL == Tool::Ramp)
        ac = ArrangerCursor::Ramp;
      else if (P_TOOL == Tool::Audition)
        ac = ArrangerCursor::Audition;
      break;
    case UiOverlayAction::STARTING_DELETE_SELECTION:
    case UiOverlayAction::DELETE_SELECTING:
    case UiOverlayAction::STARTING_ERASING:
    case UiOverlayAction::ERASING:
      ac = ArrangerCursor::Eraser;
      break;
    case UiOverlayAction::STARTING_MOVING_COPY:
    case UiOverlayAction::MovingCopy:
      ac = ArrangerCursor::GrabbingCopy;
      break;
    case UiOverlayAction::STARTING_MOVING:
    case UiOverlayAction::MOVING:
      ac = ArrangerCursor::Grabbing;
      break;
    case UiOverlayAction::STARTING_MOVING_LINK:
    case UiOverlayAction::MOVING_LINK:
      ac = ArrangerCursor::GrabbingLink;
      break;
    case UiOverlayAction::StartingPanning:
    case UiOverlayAction::Panning:
      ac = ArrangerCursor::Panning;
      break;
    case UiOverlayAction::ResizingL:
      ac = ArrangerCursor::ResizingL;
      break;
    case UiOverlayAction::ResizingR:
    case UiOverlayAction::CreatingResizingR:
      ac = ArrangerCursor::ResizingR;
      break;
    case UiOverlayAction::STARTING_SELECTION:
    case UiOverlayAction::SELECTING:
      /* TODO depends on tool */
      break;
    case UiOverlayAction::AUTOFILLING:
      ac = ArrangerCursor::Autofill;
      break;
    default:
      z_warn_if_reached ();
      ac = ArrangerCursor::Select;
      break;
    }

  return ac;
}

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
        self->action = UiOverlayAction::CreatingResizingR;
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
      arranger_widget_select_all ((ArrangerWidget *) self, F_NO_SELECT, true);
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
    [&] (auto &&s) {
      double adj_val = s->scroll_start_y_;
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
      s->set_scroll_start_y ((int) (adj_perc * size_after - diff), false);
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

  const auto start_mn_in_prj =
    std::dynamic_pointer_cast<MidiNote> (self->prj_start_object.lock ());
  const auto start_mn_transient =
    start_mn_in_prj ? start_mn_in_prj->get_transient<MidiNote> () : nullptr;
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
        case UiOverlayAction::ResizingL:
          {
            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::ResizeAction> (
                *self->sel_at_start, MIDI_SELECTIONS.get (),
                ArrangerSelectionsAction::ResizeType::L, ticks_diff));
          }
          break;
        case UiOverlayAction::ResizingR:
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
        case UiOverlayAction::MovingCopy:
          {
            UNDO_MANAGER->perform (
              std::make_unique<
                ArrangerSelectionsAction::MoveOrDuplicateMidiAction> (
                *MIDI_SELECTIONS, false, ticks_diff, pitch_diff, true));
          }
          break;
        case UiOverlayAction::None:
          {
            MIDI_SELECTIONS->clear (false);
          }
          break;
        /* something was created */
        case UiOverlayAction::CreatingResizingR:
        case UiOverlayAction::AUTOFILLING:
          {
            UNDO_MANAGER->perform (
              std::make_unique<CreateArrangerSelectionsAction> (
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