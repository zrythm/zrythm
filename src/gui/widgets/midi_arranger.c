// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/midi_region.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "audio/velocity.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/dialogs/arranger_object_info.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

/**
 * Called on drag begin in parent when background is
 * double clicked (i.e., a note is created).
 * @param pos The absolute position in the piano
 *   roll.
 */
void
midi_arranger_widget_create_note (
  ArrangerWidget * self,
  Position *       pos,
  int              note,
  ZRegion *        region)
{
  ArrangerObject * region_obj = (ArrangerObject *) region;

  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos, pos->ticks - region_obj->pos.ticks);

  /* set action */
  bool autofilling =
    self->action == UI_OVERLAY_ACTION_AUTOFILLING;
  if (!autofilling)
    {
      bool drum_mode =
        arranger_widget_get_drum_mode_enabled (self);
      if (drum_mode)
        self->action = UI_OVERLAY_ACTION_MOVING;
      else
        self->action = UI_OVERLAY_ACTION_CREATING_RESIZING_R;
    }

  int default_velocity_type =
    g_settings_get_enum (S_UI, "piano-roll-default-velocity");
  midi_byte_t velocity = VELOCITY_DEFAULT;
  switch (default_velocity_type)
    {
    case 0:
      velocity = (midi_byte_t) g_settings_get_int (
        S_UI, "piano-roll-last-set-velocity");
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

  /* create midi note */
  MidiNote * midi_note = midi_note_new (
    &region->id, &local_pos, &local_pos, (midi_byte_t) note,
    velocity);
  ArrangerObject * midi_note_obj =
    (ArrangerObject *) midi_note;

  /* add it to region */
  midi_region_add_midi_note (region, midi_note, 1);

  /*arranger_object_gen_widget (midi_note_obj);*/

  /* set visibility */
  /*arranger_object_set_widget_visibility_and_state (*/
  /*midi_note_obj, 1);*/

  /* set end pos */
  Position tmp;
  position_set_min_size (
    &midi_note_obj->pos, &tmp, self->snap_grid);
  arranger_object_set_position (
    midi_note_obj, &tmp, ARRANGER_OBJECT_POSITION_TYPE_END,
    F_NO_VALIDATE);
  arranger_object_set_position (
    midi_note_obj, &tmp, ARRANGER_OBJECT_POSITION_TYPE_END,
    F_NO_VALIDATE);

  /* set as start object */
  self->start_object = midi_note_obj;

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, midi_note);

  /* select it */
  arranger_object_select (
    midi_note_obj, F_SELECT,
    autofilling ? F_APPEND : F_NO_APPEND, F_NO_PUBLISH_EVENTS);
}

/**
 * Called during drag_update in the parent when
 * resizing the selection. It sets the start
 * Position of the selected MidiNote's.
 *
 * @param pos Absolute position in the arrranger.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
midi_arranger_widget_snap_midi_notes_l (
  ArrangerWidget * self,
  Position *       pos,
  bool             dry_run)
{
  ArrangerObject * r_obj =
    (ArrangerObject *) clip_editor_get_region (CLIP_EDITOR);

  /* get delta with first clicked note's start
   * pos */
  double delta =
    pos->ticks
    - (self->start_object->pos.ticks + r_obj->pos.ticks);
  g_debug ("delta %f", delta);

  Position         new_start_pos, new_global_start_pos;
  MidiNote *       midi_note;
  ArrangerObject * mn_obj;
  for (int i = 0; i < MA_SELECTIONS->num_midi_notes; i++)
    {
      midi_note = MA_SELECTIONS->midi_notes[i];
      mn_obj = (ArrangerObject *) midi_note;

      /* calculate new start pos */
      position_set_to_pos (&new_start_pos, &mn_obj->pos);
      position_add_ticks (&new_start_pos, delta);

      /* get the global star pos first to
       * snap it */
      position_set_to_pos (
        &new_global_start_pos, &new_start_pos);
      position_add_ticks (
        &new_global_start_pos, r_obj->pos.ticks);

      /* snap the global pos */
      ZRegion * clip_editor_region =
        clip_editor_get_region (CLIP_EDITOR);
      if (
        SNAP_GRID_ANY_SNAP (self->snap_grid)
        && !self->shift_held
        && position_is_positive (&new_global_start_pos))
        {
          position_snap (
            &self->earliest_obj_start_pos,
            &new_global_start_pos, NULL, clip_editor_region,
            self->snap_grid);
        }

      /* convert it back to a local pos */
      position_set_to_pos (
        &new_start_pos, &new_global_start_pos);
      position_add_ticks (&new_start_pos, -r_obj->pos.ticks);

      if (
        !position_is_positive (&new_global_start_pos)
        || position_is_after_or_equal (
          &new_start_pos, &mn_obj->end_pos))
        {
          return -1;
        }
      else if (!dry_run)
        {
          arranger_object_pos_setter (mn_obj, &new_start_pos);
        }
    }

  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CHANGED, MA_SELECTIONS);

  return 0;
}

/**
 * Sets the currently hovered note and queues a
 * redraw if it changed.
 *
 * @param pitch The note pitch, or -1 for no note.
 */
void
midi_arranger_widget_set_hovered_note (
  ArrangerWidget * self,
  int              pitch)
{
  if (self->hovered_note != pitch)
    {
      GdkRectangle rect;
      arranger_widget_get_visible_rect (self, &rect);
      double adj_px_per_key =
        MW_PIANO_ROLL_KEYS->px_per_key + 1.0;
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
          rect.y =
            (int) (adj_px_per_key * (127.0 - (double) pitch) - 1);
          rect.height = (int) adj_px_per_key;
        }
    }
}

/**
 * Called during drag_update in parent when
 * resizing the selection. It sets the end
 * Position of the selected MIDI notes.
 *
 * @param pos Absolute position in the arrranger.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
midi_arranger_widget_snap_midi_notes_r (
  ArrangerWidget * self,
  Position *       pos,
  bool             dry_run)
{
  ArrangerObject * r_obj =
    (ArrangerObject *) clip_editor_get_region (CLIP_EDITOR);

  /* get delta with first clicked notes's end
   * pos */
  double delta =
    pos->ticks
    - (self->start_object->end_pos.ticks + r_obj->pos.ticks);
  g_debug ("delta %f", delta);

  MidiNote * midi_note;
  Position   new_end_pos, new_global_end_pos;
  for (int i = 0; i < MA_SELECTIONS->num_midi_notes; i++)
    {
      midi_note = MA_SELECTIONS->midi_notes[i];
      ArrangerObject * mn_obj = (ArrangerObject *) midi_note;

      /* get new end pos by adding delta
       * to the cached end pos */
      position_set_to_pos (
        &new_end_pos, &midi_note->base.end_pos);
      /*&self->start_object->end_pos);*/
      position_add_ticks (&new_end_pos, delta);

      /* get the global end pos first to snap it */
      position_set_to_pos (&new_global_end_pos, &new_end_pos);
      position_add_ticks (
        &new_global_end_pos, r_obj->pos.ticks);

      /* snap the global pos */
      if (
        SNAP_GRID_ANY_SNAP (self->snap_grid)
        && !self->shift_held
        && position_is_positive (&new_global_end_pos))
        {
          position_snap (
            &self->earliest_obj_start_pos, &new_global_end_pos,
            NULL, (ZRegion *) r_obj, self->snap_grid);
        }

      /* convert it back to a local pos */
      position_set_to_pos (&new_end_pos, &new_global_end_pos);
      position_add_ticks (&new_end_pos, -r_obj->pos.ticks);

      if (position_is_before_or_equal (
            &new_end_pos, &mn_obj->pos))
        return -1;
      else if (!dry_run)
        {
          arranger_object_end_pos_setter (
            mn_obj, &new_end_pos);
        }
    }

  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CHANGED, MA_SELECTIONS);

  return 0;
}

/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
int
midi_arranger_calc_deltamax_for_note_movement (int y_delta)
{
  for (int i = 0; i < MA_SELECTIONS->num_midi_notes; i++)
    {
      MidiNote * midi_note = MA_SELECTIONS->midi_notes[i];
      /*g_message ("midi note val %d, y delta %d",*/
      /*midi_note->val, y_delta);*/
      if (midi_note->val + y_delta < 0)
        {
          y_delta = 0;
        }
      else if (midi_note->val + y_delta >= 127)
        {
          y_delta = 127 - midi_note->val;
        }
    }
  /*g_message ("y delta %d", y_delta);*/
  return y_delta;
  /*return y_delta < 0 ? -1 : 1;*/
}

/**
 * Listen to the currently selected notes.
 *
 * This function either turns on the notes if they
 * are not playing, changes the notes if the pitch
 * changed, or otherwise does nothing.
 *
 * @param listen Turn notes on if 1, or turn them
 *   off if 0.
 */
void
midi_arranger_listen_notes (ArrangerWidget * self, bool listen)
{
  /*g_message ("%s: listen: %d", __func__, listen);*/

  if (!g_settings_get_boolean (S_UI, "listen-notes"))
    return;

  ArrangerSelections * sel =
    arranger_widget_get_selections (self);
  MidiArrangerSelections * mas =
    (MidiArrangerSelections *) sel;
  Position start_pos;
  arranger_selections_get_start_pos (
    sel, &start_pos, F_NOT_GLOBAL);
  double ticks_cutoff =
    start_pos.ticks + (double) TRANSPORT->ticks_per_beat;
  for (int i = 0; i < mas->num_midi_notes; i++)
    {
      MidiNote *       mn = mas->midi_notes[i];
      ArrangerObject * mn_obj = (ArrangerObject *) mn;

      /* only add notes during the first beat of the
       * selections if listening */
      if (!listen || mn_obj->pos.ticks < ticks_cutoff)
        midi_note_listen (mn, listen);
    }
}

/**
 * Generate a context menu at x, y.
 *
 * @param menu A menu to append entries to (optional).
 *
 * @return The given updated menu or a new menu.
 */
GMenu *
midi_arranger_widget_gen_context_menu (
  ArrangerWidget * self,
  GMenu *          menu,
  double           x,
  double           y)
{
  if (!menu)
    {
      menu = g_menu_new ();
    }
  GMenuItem * menuitem;

  ArrangerObject * mn_obj =
    arranger_widget_get_hit_arranger_object (
      self, ARRANGER_OBJECT_TYPE_MIDI_NOTE, x, y);
  MidiNote * mn = (MidiNote *) mn_obj;

  if (mn)
    {
      int selected = arranger_object_is_selected (mn_obj);
      if (!selected)
        {
          arranger_object_select (
            mn_obj, F_SELECT, F_NO_APPEND,
            F_NO_PUBLISH_EVENTS);
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

      menuitem = z_gtk_create_menu_item (
        _ ("View info"), NULL, "app.duplicate");
      g_menu_append_item (menu, menuitem);
    }
  else
    {
      arranger_widget_select_all (
        (ArrangerWidget *) self, F_NO_SELECT,
        F_PUBLISH_EVENTS);
      arranger_selections_clear (
        (ArrangerSelections *) MA_SELECTIONS, F_NO_FREE,
        F_NO_PUBLISH_EVENTS);

      menuitem = CREATE_PASTE_MENU_ITEM ("app.paste");
      g_menu_append_item (menu, menuitem);
    }

  GMenu * selection_submenu = g_menu_new ();
  menuitem =
    CREATE_CLEAR_SELECTION_MENU_ITEM ("app.clear-selection");
  g_menu_append_item (selection_submenu, menuitem);
  menuitem = CREATE_SELECT_ALL_MENU_ITEM ("app.select-all");
  g_menu_append_item (selection_submenu, menuitem);

  g_menu_append_section (
    menu, _ ("Selection"), G_MENU_MODEL (selection_submenu));

  return menu;
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
  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (scroll_controller));

  if (!(state & GDK_CONTROL_MASK && state & GDK_SHIFT_MASK))
    return;

#if 0
  GtkScrolledWindow * scroll =
    arranger_widget_get_scrolled_window (self);

  double y = self->hover_y;
  double delta_y = dy;

  /* get current adjustment so we can get the
   * difference from the cursor */
  GtkAdjustment * adj =
    gtk_scrolled_window_get_vadjustment (scroll);
  double adj_val = gtk_adjustment_get_value (adj);
  double size_before = gtk_adjustment_get_upper (adj);
  double adj_perc = y / size_before;

  /* get px diff so we can calculate the new
   * adjustment later */
  double diff = y - adj_val;

  /* scroll down, zoom out */
  double size_after;
  float  notes_zoom_before = PIANO_ROLL->notes_zoom;
  double _multiplier = 1.16;
  double multiplier =
    delta_y > 0 ? 1 / _multiplier : _multiplier;
  piano_roll_set_notes_zoom (
    PIANO_ROLL, PIANO_ROLL->notes_zoom * (float) multiplier,
    0);
  size_after = size_before * multiplier;

  if (math_floats_equal (
        PIANO_ROLL->notes_zoom, notes_zoom_before))
    {
      size_after = size_before;
    }

  /* refresh relevant widgets */
  midi_editor_space_widget_refresh (MW_MIDI_EDITOR_SPACE);

  /* get updated adjustment and set its value
   at the same offset as before */
  adj = gtk_scrolled_window_get_vadjustment (scroll);
  gtk_adjustment_set_value (adj, adj_perc * size_after - diff);
#endif
}
