/*
* Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
*
* This file is part of Zrythm
*
* Zrythm is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Zrythm is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "actions/arranger_selections.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "audio/velocity.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "utils/resources.h"
#include "settings/settings.h"
#include "zrythm.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiArrangerWidget,
               midi_arranger_widget,
               ARRANGER_WIDGET_TYPE)

/**
 * To be called from get_child_position in parent
 * widget.
 *
 * Used to allocate the overlay children.
 */
void
midi_arranger_widget_set_allocation (
  MidiArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation)
{
  if (Z_IS_MIDI_NOTE_WIDGET (widget))
    {
      MidiNoteWidget * midi_note_widget =
        Z_MIDI_NOTE_WIDGET (widget);
      MidiNote * mn = midi_note_widget->midi_note;
      ArrangerObject * mn_obj =
        (ArrangerObject *) mn;

      /* use transient or non transient region
       * depending on which is visible */
      Region * region =
        (midi_note_get_main (mn))->region;
      g_return_if_fail (region);
      region =
        (Region *)
        arranger_object_get_visible_counterpart (
          (ArrangerObject *) region);
      ArrangerObject * region_obj =
        (ArrangerObject *) region;

      long region_start_ticks =
        region_obj->pos.total_ticks;
      Position tmp;
      int adj_px_per_key =
        (int)
        MW_MIDI_EDITOR_SPACE->px_per_key + 1;

      /* use absolute position */
      position_from_ticks (
        &tmp,
        region_start_ticks +
        mn_obj->pos.total_ticks);
      allocation->x =
        ui_pos_to_px_editor (
          &tmp, 1);
      allocation->y =
        adj_px_per_key *
        piano_roll_find_midi_note_descriptor_by_val (
          PIANO_ROLL, mn->val)->index;

      allocation->height = adj_px_per_key;
      if (PIANO_ROLL->drum_mode)
        {
          allocation->width = allocation->height;
          allocation->x -= allocation->width / 2;
        }
      else
        {
          /* use absolute position */
          position_from_ticks (
            &tmp,
            region_start_ticks +
            mn_obj->end_pos.total_ticks);
          allocation->width =
            ui_pos_to_px_editor (
              &tmp, 1) - allocation->x;
        }
    }
}

int
midi_arranger_widget_get_note_at_y (double y)
{
  double adj_y = y - 1;
  double adj_px_per_key =
    MW_MIDI_EDITOR_SPACE->px_per_key + 1;
  if (PIANO_ROLL->drum_mode)
    {
      return
        PIANO_ROLL->drum_descriptors[
          (int)(adj_y / adj_px_per_key)].value;
    }
  else
    return
      PIANO_ROLL->piano_descriptors[
        (int)(adj_y / adj_px_per_key)].value;
}

static int
on_motion (
  GtkWidget *      widget,
  GdkEventMotion * event,
  MidiArrangerWidget * self)
{
  if (event->type == GDK_LEAVE_NOTIFY)
    self->hovered_note = -1;
  else
    self->hovered_note =
      midi_arranger_widget_get_note_at_y (
        event->y);
  /*g_message ("hovered note: %d",*/
             /*MIDI_ARRANGER->hovered_note);*/

  arranger_widget_redraw_bg (
    Z_ARRANGER_WIDGET (self));

  return FALSE;
}

void
midi_arranger_widget_set_size (
  MidiArrangerWidget * self)
{
  // set the size
  RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    (int) rw_prv->total_px,
    gtk_widget_get_allocated_height (
      GTK_WIDGET (
        MW_MIDI_EDITOR_SPACE->
          piano_roll_keys_box)));
}

void
midi_arranger_widget_setup (
  MidiArrangerWidget * self)
{
  midi_arranger_widget_set_size (self);

  ARRANGER_WIDGET_GET_PRIVATE (self);
  g_signal_connect (
    G_OBJECT(ar_prv->bg),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
}

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
midi_arranger_widget_get_cursor (
  MidiArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_MIDI_NOTE,
      ar_prv->hover_x, ar_prv->hover_y);
  int is_hit = obj != NULL;


  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (tool == TOOL_SELECT_NORMAL ||
          tool == TOOL_SELECT_STRETCH ||
          tool == TOOL_EDIT)
        {
          int is_resize_l = 0, is_resize_r = 0;

          if (is_hit)
            {
              ARRANGER_OBJECT_WIDGET_GET_PRIVATE (
                obj->widget);
              is_resize_l = ao_prv->resize_l;
              is_resize_r = ao_prv->resize_r;
            }

          if (is_hit && is_resize_l &&
              !PIANO_ROLL->drum_mode)
            {
              return ARRANGER_CURSOR_RESIZING_L;
            }
          else if (is_hit && is_resize_r &&
                   !PIANO_ROLL->drum_mode)
            {
              return ARRANGER_CURSOR_RESIZING_R;
            }
          else if (is_hit)
            {
              return ARRANGER_CURSOR_GRAB;
            }
          else
            {
              /* set cursor to whatever it is */
              if (tool == TOOL_EDIT)
                return ARRANGER_CURSOR_EDIT;
              else
                return ARRANGER_CURSOR_SELECT;
            }
        }
      else if (P_TOOL == TOOL_EDIT)
        ac = ARRANGER_CURSOR_EDIT;
      else if (P_TOOL == TOOL_ERASER)
        ac = ARRANGER_CURSOR_ERASER;
      else if (P_TOOL == TOOL_RAMP)
        ac = ARRANGER_CURSOR_RAMP;
      else if (P_TOOL == TOOL_AUDITION)
        ac = ARRANGER_CURSOR_AUDITION;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
    case UI_OVERLAY_ACTION_SELECTING:
      ac = ARRANGER_CURSOR_SELECT;
      /* TODO depends on tool */
      break;
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

/**
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 * @param pos The absolute position in the piano
 *   roll.
 */
void
midi_arranger_widget_create_note (
  MidiArrangerWidget * self,
  Position * pos,
  int                  note,
  Region * region)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos,
    pos->total_ticks -
    region_obj->pos.total_ticks);

  /* set action */
  if (PIANO_ROLL->drum_mode)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING;
  else
    ar_prv->action =
      UI_OVERLAY_ACTION_CREATING_RESIZING_R;

  /* create midi note */
  MidiNote * midi_note =
    midi_note_new (
      region, &local_pos, &local_pos,
      (midi_byte_t) note,
      VELOCITY_DEFAULT, 1);
  ArrangerObject * midi_note_obj =
    (ArrangerObject *) midi_note;

  /* add it to region */
  midi_region_add_midi_note (
    region, midi_note);

  arranger_object_gen_widget (midi_note_obj);

  /* set visibility */
  arranger_object_set_widget_visibility_and_state (
    midi_note_obj, 1);

  /* set end pos */
  Position tmp;
  position_set_min_size (
    &midi_note_obj->pos,
    &tmp, ar_prv->snap_grid);
  arranger_object_set_position (
    midi_note_obj, &tmp,
    ARRANGER_OBJECT_POSITION_TYPE_END,
    F_NO_CACHED, F_NO_VALIDATE, AO_UPDATE_ALL);
  arranger_object_set_position (
    midi_note_obj, &tmp,
    ARRANGER_OBJECT_POSITION_TYPE_END,
    F_CACHED, F_NO_VALIDATE, AO_UPDATE_ALL);

  /* set as start object */
  ar_prv->start_object = midi_note_obj;

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CREATED, midi_note);

  /* select it */
  arranger_object_select (
    midi_note_obj, F_SELECT, F_NO_APPEND);
}

/**
 * Called when in selection mode.
 *
 * Called by arranger widget during drag_update to find and
 * select the midi notes enclosed in the selection area.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
void
midi_arranger_widget_select (
  MidiArrangerWidget * self,
  double               offset_x,
  double               offset_y,
  int                  delete)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (!delete)
    /* deselect all */
    arranger_widget_select_all (
      Z_ARRANGER_WIDGET (self), 0);

  /* find enclosed midi notes */
  GtkWidget *    midi_note_widgets[800];
  int            num_midi_note_widgets = 0;
  arranger_widget_get_hit_widgets_in_range (
    Z_ARRANGER_WIDGET (self),
    MIDI_NOTE_WIDGET_TYPE,
    ar_prv->start_x,
    ar_prv->start_y,
    offset_x,
    offset_y,
    midi_note_widgets,
    &num_midi_note_widgets);

  ArrangerObjectWidget * midi_note_widget;
  ArrangerObject * midi_note_obj;
  if (delete)
    {
      /* delete the enclosed midi notes */
      for (int i = 0; i < num_midi_note_widgets; i++)
        {
          midi_note_widget =
            Z_ARRANGER_OBJECT_WIDGET (
              midi_note_widgets[i]);
          ARRANGER_OBJECT_WIDGET_GET_PRIVATE (
            midi_note_widget);
          midi_note_obj =
            arranger_object_get_main (
              ao_prv->arranger_object);

          MidiNote * mn =
            (MidiNote *) midi_note_obj;

          midi_region_remove_midi_note (
            mn->region, mn,
            F_NO_FREE, F_PUBLISH_EVENTS);
        }
    }
  else
    {
      /* select the enclosed midi_notes */
      for (int i = 0; i < num_midi_note_widgets; i++)
        {
          midi_note_widget =
            Z_ARRANGER_OBJECT_WIDGET (
              midi_note_widgets[i]);
          ARRANGER_OBJECT_WIDGET_GET_PRIVATE (
            midi_note_widget);
          midi_note_obj =
            arranger_object_get_main (
              ao_prv->arranger_object);
          arranger_object_select (
            midi_note_obj, F_SELECT, F_APPEND);
        }
    }
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
  MidiArrangerWidget *self,
  Position *          pos,
  int                 dry_run)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ArrangerObject * r_obj =
    (ArrangerObject *) CLIP_EDITOR->region;

  /* get delta with first clicked note's start
   * pos */
  long delta;
  delta =
    pos->total_ticks -
    (ar_prv->start_object->
      cache_pos.total_ticks +
    r_obj->pos.total_ticks);

  Position new_start_pos, new_global_start_pos;
  MidiNote * midi_note;
  ArrangerObject * mn_obj;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        midi_note_get_main (
          MA_SELECTIONS->midi_notes[i]);
      mn_obj =
        (ArrangerObject *) midi_note;

      /* calculate new start pos by adding
       * delta to the cached start pos */
      position_set_to_pos (
        &new_start_pos,
        &mn_obj->cache_pos);
      position_add_ticks (
        &new_start_pos, delta);

      /* get the global star pos first to
       * snap it */
      position_set_to_pos (
        &new_global_start_pos,
        &new_start_pos);
      position_add_ticks (
        &new_global_start_pos,
        r_obj->pos.total_ticks);

      /* snap the global pos */
      if (SNAP_GRID_ANY_SNAP (
            ar_prv->snap_grid) &&
          !ar_prv->shift_held)
        position_snap (
          NULL, &new_global_start_pos,
          NULL, CLIP_EDITOR->region,
          ar_prv->snap_grid);

      /* convert it back to a local pos */
      position_set_to_pos (
        &new_start_pos,
        &new_global_start_pos);
      position_add_ticks (
        &new_start_pos,
        - r_obj->pos.total_ticks);

      if (position_is_before (
            &new_global_start_pos,
            &POSITION_START) ||
          position_is_after_or_equal (
            &new_start_pos,
            &mn_obj->end_pos))
        return -1;
      else if (!dry_run)
        {
          arranger_object_pos_setter (
            mn_obj, &new_start_pos);
        }
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED,
    MA_SELECTIONS);

  return 0;
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
  MidiArrangerWidget *self,
  Position *          pos,
  int                 dry_run)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ArrangerObject * r_obj =
    (ArrangerObject *) CLIP_EDITOR->region;

  /* get delta with first clicked notes's end
   * pos */
  long delta =
    pos->total_ticks -
    (ar_prv->start_object->
      cache_end_pos.total_ticks +
    r_obj->pos.total_ticks);

  MidiNote * midi_note;
  Position new_end_pos, new_global_end_pos;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        midi_note_get_main (
          MA_SELECTIONS->midi_notes[i]);
      ArrangerObject * mn_obj =
        (ArrangerObject *) midi_note;

      Track * track =
        arranger_object_get_track (mn_obj);

      /* get new end pos by adding delta
       * to the cached end pos */
      position_set_to_pos (
        &new_end_pos,
        &mn_obj->cache_end_pos);
      position_add_ticks (
        &new_end_pos, delta);

      /* get the global end pos first to snap it */
      position_set_to_pos (
        &new_global_end_pos,
        &new_end_pos);
      position_add_ticks (
        &new_global_end_pos,
        r_obj->pos.total_ticks);

      /* snap the global pos */
      if (SNAP_GRID_ANY_SNAP (
            ar_prv->snap_grid) &&
          !ar_prv->shift_held)
        position_snap (
          NULL, &new_global_end_pos, track,
          NULL, ar_prv->snap_grid);

      /* convert it back to a local pos */
      position_set_to_pos (
        &new_end_pos,
        &new_global_end_pos);
      position_add_ticks (
        &new_end_pos,
        - r_obj->pos.total_ticks);

      if (position_is_before_or_equal (
            &new_end_pos,
            &mn_obj->pos))
        return -1;
      else if (!dry_run)
        {
          arranger_object_end_pos_setter (
            mn_obj, &new_end_pos);
        }
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED,
    MA_SELECTIONS);

  return 0;
}

/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
static int
calc_deltamax_for_note_movement (int y_delta)
{
  MidiNote * midi_note;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        midi_note_get_main (
          MA_SELECTIONS->midi_notes[i]);
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
 * Called when moving midi notes in drag update in arranger
 * widget for moving up/down (changing note).
 */
void
midi_arranger_widget_move_items_y (
  MidiArrangerWidget *self,
  double              offset_y)
{
  ARRANGER_WIDGET_GET_PRIVATE(self);

  int y_delta;
  /* first note selected */
  int first_note_selected =
    midi_note_get_main (
      ar_prv->start_object)->val;
  /* note at cursor */
  int note_at_cursor =
    midi_arranger_widget_get_note_at_y (
      ar_prv->start_y + offset_y);

  y_delta = note_at_cursor - first_note_selected;
  y_delta =
    calc_deltamax_for_note_movement (y_delta);
  MidiNote * midi_note;
  ArrangerObjectUpdateFlag flag =
    AO_UPDATE_NON_TRANS;

  g_message ("midi notes %d", MA_SELECTIONS->num_midi_notes);
  int i;
  for (i = 0; i < MA_SELECTIONS->num_midi_notes; i++)
    {
      midi_note =
        midi_note_get_main (
          MA_SELECTIONS->midi_notes[i]);
      ArrangerObject * mn_obj =
        (ArrangerObject *) midi_note;
      midi_note_set_val (
        midi_note,
        (midi_byte_t)
          ((int) midi_note->val + y_delta),
        flag);
      if (Z_IS_ARRANGER_OBJECT_WIDGET (
            mn_obj->widget))
        {
          arranger_object_widget_update_tooltip (
            Z_ARRANGER_OBJECT_WIDGET (
              mn_obj->widget), 0);
        }
    }
}


/**
 * Called on drag end.
 *
 * Sets default cursors back and sets the start midi note
 * to NULL if necessary.
 */
void
midi_arranger_widget_on_drag_end (
  MidiArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  MidiNote * midi_note;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        MA_SELECTIONS->midi_notes[i];
      ArrangerObject * mn_obj =
        (ArrangerObject *) midi_note;

      if (Z_IS_ARRANGER_OBJECT_WIDGET (
            mn_obj->widget))
        {
          arranger_object_widget_update_tooltip (
            Z_ARRANGER_OBJECT_WIDGET (
              mn_obj->widget), 0);
        }

      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_CREATED, midi_note);
    }

  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_L:
    {
      ArrangerObject * trans_note =
        arranger_object_get_main_trans (
          MA_SELECTIONS->midi_notes[0]);
      UndoableAction * ua =
        arranger_selections_action_new_resize (
          (ArrangerSelections *) MA_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_L,
          trans_note->pos.total_ticks -
          trans_note->cache_pos.total_ticks);
      undo_manager_perform (
        UNDO_MANAGER, ua);
      arranger_selections_reset_counterparts (
        (ArrangerSelections *) MA_SELECTIONS, 1);
    }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    {
      ArrangerObject * trans_note =
        arranger_object_get_main_trans (
          MA_SELECTIONS->midi_notes[0]);
      UndoableAction * ua =
        arranger_selections_action_new_resize (
          (ArrangerSelections *) MA_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_R,
          trans_note->end_pos.total_ticks -
          trans_note->cache_end_pos.total_ticks);
      undo_manager_perform (
        UNDO_MANAGER, ua);
      arranger_selections_reset_counterparts (
        (ArrangerSelections *) MA_SELECTIONS, 1);
    }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    {
      /* if something was clicked with ctrl without
       * moving*/
      if (ar_prv->ctrl_held)
        if (ar_prv->start_object &&
            arranger_object_is_selected (
              ar_prv->start_object))
          {
            /* deselect it */
            arranger_object_select (
              ar_prv->start_object,
              F_NO_SELECT, F_APPEND);
          }
    }
      break;
    case UI_OVERLAY_ACTION_MOVING:
    {
      ArrangerObject * note_obj =
        arranger_object_get_main (
          MA_SELECTIONS->midi_notes[0]);
      MidiNote * mn =
        (MidiNote *) note_obj;
      long ticks_diff =
        note_obj->pos.total_ticks -
          note_obj->cache_pos.total_ticks;
      int pitch_diff =
        mn->val - mn->cache_val;
      UndoableAction * ua =
        arranger_selections_action_new_move_midi (
          MA_SELECTIONS, ticks_diff, pitch_diff);
      undo_manager_perform (
        UNDO_MANAGER, ua);
      arranger_selections_reset_counterparts (
        (ArrangerSelections *) MA_SELECTIONS, 1);
    }
      break;
    /* if copy/link-moved */
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
    {
      ArrangerObject * note_obj =
        arranger_object_get_main (
          MA_SELECTIONS->midi_notes[0]);
      MidiNote * mn =
        (MidiNote *) note_obj;
      long ticks_diff =
        note_obj->pos.total_ticks -
          note_obj->cache_pos.total_ticks;
      int pitch_diff =
        mn->val - mn->cache_val;
      arranger_selections_reset_counterparts (
        (ArrangerSelections *) MA_SELECTIONS, 0);
      UndoableAction * ua =
        (UndoableAction *)
        arranger_selections_action_new_duplicate_midi (
          (ArrangerSelections *) MA_SELECTIONS,
          ticks_diff,
          pitch_diff);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
      break;
    case UI_OVERLAY_ACTION_NONE:
    {
      arranger_selections_clear (
        (ArrangerSelections *) MA_SELECTIONS);
    }
      break;
    /* something was created */
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
    {
      UndoableAction * ua =
        arranger_selections_action_new_create (
          (ArrangerSelections *) MA_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
      break;
    /* if didn't click on something */
    default:
    {
    }
      break;
    }

  ar_prv->start_object = NULL;
  /*if (self->start_midi_note_clone)*/
    /*{*/
      /*midi_note_free (self->start_midi_note_clone);*/
      /*self->start_midi_note_clone = NULL;*/
    /*}*/

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED, MA_SELECTIONS);
}

static inline void
add_children_from_region (
  MidiArrangerWidget * self,
  Region *             region)
{
  g_return_if_fail (
    self && GTK_IS_OVERLAY (self) && region);

  int i, j;
  MidiNote * mn;
  for (i = 0; i < region->num_midi_notes; i++)
    {
      mn = region->midi_notes[i];

      for (j = 0;
           j <= AOI_COUNTERPART_MAIN_TRANSIENT; j++)
        {
          if (j == AOI_COUNTERPART_MAIN)
            mn = midi_note_get_main (mn);
          else if (
            j == AOI_COUNTERPART_MAIN_TRANSIENT)
            mn = midi_note_get_main_trans (mn);

          g_return_if_fail (mn);

          ArrangerObject * mn_obj =
            (ArrangerObject *) mn;

          if (!Z_IS_ARRANGER_OBJECT_WIDGET (
                mn_obj->widget))
            {
              arranger_object_gen_widget (
                mn_obj);
            }

          arranger_widget_add_overlay_child (
            (ArrangerWidget *) self, mn_obj);
        }
    }
}

/**
 * Readd children.
 */
void
midi_arranger_widget_refresh_children (
  MidiArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  int i, k;

  /* remove all children except bg */
  GList *children, *iter;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (self));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);
      if (widget != (GtkWidget *) ar_prv->bg &&
          widget != (GtkWidget *) ar_prv->playhead)
        {
          g_object_ref (widget);
          gtk_container_remove (
            GTK_CONTAINER (self),
            widget);
        }
    }
  g_list_free (children);

  if (CLIP_EDITOR->region &&
      CLIP_EDITOR->region->type == REGION_TYPE_MIDI)
    {
      /* add notes of all regions in the track */
      TrackLane * lane;
      Track * track =
        CLIP_EDITOR->region->lane->track;
      for (k = 0; k < track->num_lanes; k++)
        {
          lane = track->lanes[k];

          for (i = 0; i < lane->num_regions; i++)
            {
              add_children_from_region (
                self, lane->regions[i]);
            }
        }
    }

  gtk_overlay_reorder_overlay (
    GTK_OVERLAY (self),
    (GtkWidget *) ar_prv->playhead, -1);
}

static gboolean
on_focus (GtkWidget       *widget,
          gpointer         user_data)
{
  /*g_message ("timeline focused");*/
  MAIN_WINDOW->last_focused = widget;

  return FALSE;
}

static void
midi_arranger_widget_class_init (
  MidiArrangerWidgetClass * klass)
{
}

static void
midi_arranger_widget_init (MidiArrangerWidget *self)
{
  g_signal_connect (
    self, "grab-focus",
    G_CALLBACK (on_focus), self);
}
