/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm.h"
#include "project.h"
#include "settings/settings.h"
#include "gui/widgets/region.h"
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
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/arrays.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiArrangerWidget,
               midi_arranger_widget,
               ARRANGER_WIDGET_TYPE)

/**
 * To be called from get_child_position in parent widget.
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
      allocation->x =
        ui_pos_to_px_piano_roll (
          &midi_note_widget->midi_note->start_pos,
          1);
      allocation->y =
        MW_PIANO_ROLL->piano_roll_labels->px_per_note *
          (127 - midi_note_widget->midi_note->val);
      allocation->width =
        ui_pos_to_px_piano_roll (
          &midi_note_widget->midi_note->end_pos,
          1) - allocation->x;
      allocation->height =
        MW_PIANO_ROLL->piano_roll_labels->px_per_note;
    }
}

int
midi_arranger_widget_get_note_at_y (double y)
{
  return 128 - y / PIANO_ROLL_LABELS->px_per_note;
}

MidiNoteWidget *
midi_arranger_widget_get_hit_midi_note (
  MidiArrangerWidget *  self,
  double                x,
  double                y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      MIDI_NOTE_WIDGET_TYPE);
  if (widget)
    {
      MidiNoteWidget * mn_w =
        Z_MIDI_NOTE_WIDGET (widget);
      self->start_midi_note = mn_w->midi_note;
      return mn_w;
    }
  return NULL;
}


void
midi_arranger_widget_update_inspector (
  MidiArrangerWidget *self)
{
  inspector_widget_show_selections (
    INSPECTOR_CHILD_MIDI,
    (void **) MIDI_ARRANGER_SELECTIONS->midi_notes,
    MIDI_ARRANGER_SELECTIONS->num_midi_notes);
}

void
midi_arranger_widget_select_all (
  MidiArrangerWidget *  self,
  int               select)
{
  if (!CLIP_EDITOR->region)
    return;

  MIDI_ARRANGER_SELECTIONS->num_midi_notes = 0;
  MIDI_MODIFIER_ARRANGER->num_velocities = 0;

  /* select midi notes */
  MidiRegion * mr =
    (MidiRegion *) CLIP_EDITOR_SELECTED_REGION;
  for (int i = 0; i < mr->num_midi_notes; i++)
    {
      MidiNote * midi_note = mr->midi_notes[i];
      midi_note_widget_select (
        midi_note->widget, select);

      if (select)
        {
          /* select  */
          array_append (
            MIDI_ARRANGER_SELECTIONS->midi_notes,
            MIDI_ARRANGER_SELECTIONS->num_midi_notes,
            midi_note);
          array_append (
            MIDI_MODIFIER_ARRANGER->velocities,
            MIDI_MODIFIER_ARRANGER->num_velocities,
            midi_note->vel);
        }
      else
        {
          array_delete (
            MIDI_ARRANGER_SELECTIONS->midi_notes,
            MIDI_ARRANGER_SELECTIONS->num_midi_notes,
            midi_note);
          array_delete (
            MIDI_MODIFIER_ARRANGER->velocities,
            MIDI_MODIFIER_ARRANGER->num_velocities,
            midi_note->vel);
        }
    }
  arranger_widget_refresh(&self->parent_instance);
  
}

static int
on_motion (GtkWidget *      widget,
           GdkEventMotion * event,
           RegionWidget *   self)
{
  if (event->type == GDK_LEAVE_NOTIFY)
    MIDI_ARRANGER->hovered_note = -1;
  else
    MIDI_ARRANGER->hovered_note =
      midi_arranger_widget_get_note_at_y (
        event->y - 2); // for line boundaries

  ARRANGER_WIDGET_GET_PRIVATE (MIDI_ARRANGER);
  gtk_widget_queue_draw (
              GTK_WIDGET (ar_prv->bg));

  return FALSE;
}

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
midi_arranger_widget_show_context_menu (MidiArrangerWidget * self)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_with_label("Do something");

  /*g_signal_connect(menuitem, "activate",*/
                   /*(GCallback) view_popup_menu_onDoSomething, treeview);*/

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

void
midi_arranger_widget_setup (
  MidiArrangerWidget * self)
{
  // set the size
  int ww, hh;
  gtk_widget_get_size_request (
    GTK_WIDGET (PIANO_ROLL_LABELS),
    &ww,
    &hh);
  RULER_WIDGET_GET_PRIVATE (MIDI_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    rw_prv->total_px,
    hh);

  ARRANGER_WIDGET_GET_PRIVATE (self);
  g_signal_connect (
    G_OBJECT(ar_prv->bg),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
}

/**
 * Sets start positions for selected midi notes.
 */
void
midi_arranger_widget_find_start_poses (
  MidiArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * r =
        MIDI_ARRANGER_SELECTIONS->midi_notes[i];
      if (position_compare (&r->start_pos,
                            &ar_prv->start_pos) <= 0)
        {
          position_set_to_pos (&ar_prv->start_pos,
                               &r->start_pos);
        }

      /* set start poses for midi_notes */
      position_set_to_pos (
        &self->midi_note_start_poses[i],
        &r->start_pos);
    }
}

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
midi_arranger_widget_get_cursor (
  UiOverlayAction action,
  Tool            tool)
{
  MidiArrangerWidget * self =
    MIDI_ARRANGER;
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (tool == TOOL_SELECT_NORMAL ||
      tool == TOOL_SELECT_STRETCH ||
      tool == TOOL_EDIT)
    {
      MidiNoteWidget * mnw =
        midi_arranger_widget_get_hit_midi_note (
          self,
          ar_prv->hover_x,
          ar_prv->hover_y);

      /* TODO chords, aps */

      int is_hit =
        mnw != NULL;
      int is_resize_l =
        mnw && mnw->resize_l;
      int is_resize_r =
        mnw && mnw->resize_r;

      if (is_hit && is_resize_l)
        {
          return ARRANGER_CURSOR_RESIZING_L;
        }
      else if (is_hit && is_resize_r)
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
  else if (tool == TOOL_ERASER)
    return ARRANGER_CURSOR_ERASER;
  else if (tool == TOOL_RAMP)
    return ARRANGER_CURSOR_RAMP;
  else if (tool == TOOL_AUDITION)
    return ARRANGER_CURSOR_AUDITION;

  g_assert_not_reached ();
  return -1;
}

void
midi_arranger_widget_on_drag_begin_note_hit (
  MidiArrangerWidget * self,
  double               start_x,
  MidiNoteWidget *     mnw)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* get x as local to the midi note */
  gint wx, wy;
  gtk_widget_translate_coordinates(
            GTK_WIDGET (self),
            GTK_WIDGET (mnw),
            start_x,
            0,
            &wx,
            &wy);

  MidiNote * mn = mnw->midi_note;
  self->start_midi_note = mn;
  self->start_midi_note_clone =
    midi_note_clone (mn, mn->midi_region);

  /* update arranger action */
  switch (P_TOOL)
    {
    case TOOL_ERASER:
      ar_prv->action =
        UI_OVERLAY_ACTION_ERASING;
      break;
    case TOOL_AUDITION:
      ar_prv->action =
        UI_OVERLAY_ACTION_AUDITIONING;
      break;
    case TOOL_SELECT_NORMAL:
      if (midi_note_widget_is_resize_l (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_L;
      else if (midi_note_widget_is_resize_r (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_R;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_STARTING_MOVING;
      break;
    case TOOL_SELECT_STRETCH:
      if (midi_note_widget_is_resize_l (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_STRETCHING_L;
      else if (midi_note_widget_is_resize_r (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_STRETCHING_R;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_STARTING_MOVING;
      break;
    case TOOL_EDIT:
      if (midi_note_widget_is_resize_l (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_L;
      else if (midi_note_widget_is_resize_r (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_R;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_STARTING_MOVING;
      break;
    }

  /* select midi note if unselected */
  if (P_TOOL == TOOL_EDIT ||
      P_TOOL == TOOL_SELECT_NORMAL ||
      P_TOOL == TOOL_SELECT_STRETCH)
    {
      /* if ctrl held & not selected, add to
       * selections */
      if (ar_prv->ctrl_held && !mn->selected)
        {
          ARRANGER_WIDGET_SELECT_MIDI_NOTE (
            self, mn, 1, 1);
        }
      /* if ctrl not held & not selected, make it
       * the only
       * selection */
      else if (!ar_prv->ctrl_held &&
               !array_contains (
                  MIDI_ARRANGER_SELECTIONS->midi_notes,
                  MIDI_ARRANGER_SELECTIONS->num_midi_notes,
                  mn))
        {
          ARRANGER_WIDGET_SELECT_MIDI_NOTE (
            self, mn, 1, 0);
        }
    }

  /* find highest and lowest selected regions */
  MIDI_ARRANGER_SELECTIONS->top_midi_note =
    MIDI_ARRANGER_SELECTIONS->midi_notes[0];
  MIDI_ARRANGER_SELECTIONS->bot_midi_note =
    MIDI_ARRANGER_SELECTIONS->midi_notes[0];
  for (int i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * midi_note =
        MIDI_ARRANGER_SELECTIONS->midi_notes[i];
      if (midi_note->val >
            MIDI_ARRANGER_SELECTIONS->top_midi_note->val)
        {
          MIDI_ARRANGER_SELECTIONS->top_midi_note = midi_note;
        }
      if (midi_note->val <
          MIDI_ARRANGER_SELECTIONS->bot_midi_note->val)
        {
          MIDI_ARRANGER_SELECTIONS->bot_midi_note = midi_note;
        }
    }
}

/**
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 */
void
midi_arranger_widget_create_note (
  MidiArrangerWidget * self,
  Position * pos,
  int                  note,
  MidiRegion * region)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
      !ar_prv->shift_held)
    {
      position_snap (NULL,
                     pos,
                     NULL,
                     NULL,
                     ar_prv->snap_grid);
    }
  Velocity * vel = velocity_default ();
  MidiNote * midi_note = midi_note_new (region,
                                   pos,
                                   pos,
                                   note,
                                   vel);
  position_set_min_size (&midi_note->start_pos,
                         &midi_note->end_pos,
                         ar_prv->snap_grid);
  midi_region_add_midi_note (region,
                        midi_note);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (midi_note->widget));
  gtk_overlay_add_overlay (
    GTK_OVERLAY (MIDI_MODIFIER_ARRANGER),
    GTK_WIDGET (midi_note->vel->widget));
  ar_prv->action = UI_OVERLAY_ACTION_RESIZING_R;
  MIDI_ARRANGER_SELECTIONS->midi_notes[0] =
    midi_note;
  MIDI_ARRANGER_SELECTIONS->num_midi_notes = 1;
  midi_note_widget_select (midi_note->widget, 1);
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

  if (delete)
    {
      /* delete the enclosed midi notes */
      for (int i = 0; i < num_midi_note_widgets; i++)
        {
          MidiNoteWidget * midi_note_widget =
            Z_MIDI_NOTE_WIDGET (midi_note_widgets[i]);
          MidiNote * midi_note =
            midi_note_widget->midi_note;

          midi_region_remove_midi_note (
            midi_note->midi_region,
            midi_note);
        }
    }
  else
    {
      /* select the enclosed midi_notes */
      for (int i = 0; i < num_midi_note_widgets; i++)
        {
          MidiNoteWidget * midi_note_widget =
            Z_MIDI_NOTE_WIDGET (
              midi_note_widgets[i]);
          MidiNote * midi_note =
            midi_note_widget->midi_note;
          ARRANGER_WIDGET_SELECT_MIDI_NOTE (
            self,
            midi_note,
            1,
            1);
        }
    }
}

/**
 * Called during drag_update in parent when resizing the
 * selection. It sets the start pos of the selected MIDI
 * notes.
 */
void
midi_arranger_widget_snap_midi_notes_l (
  MidiArrangerWidget *self,
  Position *          pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  for (int i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * midi_note =
        MIDI_ARRANGER_SELECTIONS->midi_notes[i];
      if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
            !ar_prv->shift_held)
        position_snap (NULL,
                       pos,
                       NULL,
                       (Region *) midi_note->midi_region,
                       ar_prv->snap_grid);
      midi_note_set_start_pos (midi_note,
                               pos);

      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   midi_note);
    }
}

/**
 * Called during drag_update in parent when resizing the
 * selection. It sets the end pos of the selected MIDI
 * notes.
 */
void
midi_arranger_widget_snap_midi_notes_r (
  MidiArrangerWidget *self,
  Position *          pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * midi_note =
        MIDI_ARRANGER_SELECTIONS->midi_notes[i];
      if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
            !ar_prv->shift_held)
        position_snap (NULL,
                       pos,
                       NULL,
                       (Region *) midi_note->midi_region,
                       ar_prv->snap_grid);
      if (position_compare (
            pos, &midi_note->start_pos) > 0)
        {
          midi_note_set_end_pos (midi_note,
                                 pos);
        }
      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   midi_note);
    }
}

/**
 * Called when moving midi notes in drag update in arranger
 * widget.
 */
void
midi_arranger_widget_move_items_x (
  MidiArrangerWidget * self,
  long                 ticks_diff)
{
  /* update region positions */
  for (int i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * r =
        MIDI_ARRANGER_SELECTIONS->midi_notes[i];
      Position * prev_start_pos =
        &self->midi_note_start_poses[i];
      long length_ticks =
        position_to_ticks (&r->end_pos) -
          position_to_ticks (&r->start_pos);
      Position tmp;
      position_set_to_pos (&tmp, prev_start_pos);
      position_add_ticks (&tmp,
                          ticks_diff + length_ticks);
      midi_note_set_end_pos (r, &tmp);
      position_set_to_pos (&tmp, prev_start_pos);
      position_add_ticks (&tmp, ticks_diff);
      midi_note_set_start_pos (r, &tmp);

      if (r->widget)
        midi_note_widget_update_tooltip (
          r->widget, 1);

    }
}


/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
static int
calc_deltamax_for_note_movement (int y_delta)
{
  for (int i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * midi_note =
      MIDI_ARRANGER_SELECTIONS->midi_notes[i];
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
  int ar_start_val = self->start_midi_note->val;
  int ar_end_val =
    midi_arranger_widget_get_note_at_y (
      ar_prv->start_y + offset_y);

  y_delta = ar_end_val - ar_start_val;
  y_delta =
    calc_deltamax_for_note_movement (y_delta);
  if (ar_end_val != ar_start_val)
    {
      for (int i = 0;
           i < MIDI_ARRANGER_SELECTIONS->
                 num_midi_notes;
           i++)
        {
          MidiNote * midi_note =
          MIDI_ARRANGER_SELECTIONS->midi_notes[i];
          midi_note->val = midi_note->val + y_delta;
          if (midi_note->widget)
            midi_note_widget_update_tooltip (
              midi_note->widget, 0);

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

  for (int i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * midi_note =
        MIDI_ARRANGER_SELECTIONS->midi_notes[i];

      if (midi_note->widget)
        midi_note_widget_update_tooltip (
          midi_note->widget, 0);

      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   midi_note);
    }

  /* if something was clicked with ctrl without
   * moving*/
  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      /* deselect it */
      if (ar_prv->ctrl_held)
        if (self->start_midi_note_clone &&
            self->start_midi_note_clone->selected)
          {
            ARRANGER_WIDGET_SELECT_MIDI_NOTE (
              self, self->start_midi_note,
              0, 1);
          }
    }
  /* if didn't click on something */
  else
    {
      self->start_midi_note = NULL;
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
      MidiRegion * mr =
        (MidiRegion *) CLIP_EDITOR->region;
      for (int j = 0; j < mr->num_midi_notes; j++)
        {
          MidiNote * midi_note = mr->midi_notes[j];
          gtk_overlay_add_overlay (
            GTK_OVERLAY (self),
            GTK_WIDGET (midi_note->widget));
        }
    }
}

static gboolean
on_focus (GtkWidget       *widget,
          gpointer         user_data)
{
  /*g_message ("timeline focused");*/
  MAIN_WINDOW->last_focused = widget;

  return FALSE;
}

void
midi_arranger_auto_scroll (
  MidiArrangerWidget * self,
  GtkScrolledWindow * scrolled_window)
{
  Region * region = CLIP_EDITOR->region;
  int scroll_speed = 20;
  int border_distance = 10;
  if (region != 0)
  {
    MidiNote * first_note =
      midi_region_get_first_midi_note (region);
    MidiNote * last_note = midi_region_get_last_midi_note (
      region);
    MidiNote * lowest_note =
      midi_region_get_lowest_midi_note (region);
    MidiNote * highest_note =
      midi_region_get_highest_midi_note (region);
    int arranger_width = gtk_widget_get_allocated_width (
      GTK_WIDGET (scrolled_window));
    int arranger_height = gtk_widget_get_allocated_height (
      GTK_WIDGET (scrolled_window));
    GtkAdjustment *hadj =
      gtk_scrolled_window_get_hadjustment (
        GTK_SCROLLED_WINDOW (scrolled_window));
    GtkAdjustment *vadj =
      gtk_scrolled_window_get_vadjustment (
        GTK_SCROLLED_WINDOW (scrolled_window));
    int v_delta = 0;
    int h_delta = 0;
    if (lowest_note != 0)
    {
      GtkWidget *focused = GTK_WIDGET (lowest_note->widget);
      gint note_x, note_y;
      gtk_widget_translate_coordinates (
        focused,
        GTK_WIDGET (scrolled_window),
        0,
        0,
        &note_x,
        &note_y);
      int note_height = gtk_widget_get_allocated_height (
        GTK_WIDGET (focused));
      if (note_y + note_height + border_distance
        >= arranger_height)
      {
        v_delta = scroll_speed;
      }
    }
    if (highest_note != 0)
    {
      GtkWidget *focused = GTK_WIDGET (
        highest_note->widget);
      gint note_x, note_y;
      gtk_widget_translate_coordinates (
        focused,
        GTK_WIDGET (scrolled_window),
        0,
        0,
        &note_x,
        &note_y);

      int note_height = gtk_widget_get_allocated_height (
        GTK_WIDGET (focused));
      g_message("x%d,y%d,width%d,arranger h%d",note_x,note_y,note_height,arranger_height);

      if (note_y - border_distance <= 1)
      {
        v_delta = scroll_speed * -1;
      }
    }
    if (first_note != 0)
    {
      gint note_x, note_y;
      GtkWidget *focused = GTK_WIDGET (first_note->widget);
      gtk_widget_translate_coordinates (
        focused,
        GTK_WIDGET (self),
        0,
        0,
        &note_x,
        &note_y);
      int note_width = gtk_widget_get_allocated_width (
        GTK_WIDGET (focused));
      if (note_x - border_distance <= 10)
      {
        h_delta = scroll_speed * -1;
      }
    }
    if (last_note != 0)
    {
      gint note_x, note_y;
      GtkWidget *focused = GTK_WIDGET (last_note->widget);
      gtk_widget_translate_coordinates (
        focused,
        GTK_WIDGET (scrolled_window),
        0,
        0,
        &note_x,
        &note_y);
      int note_width = gtk_widget_get_allocated_width (
        GTK_WIDGET (focused));
      if (note_x + note_width + border_distance
        > arranger_width)
      {
        h_delta = scroll_speed;
      }
    }
    if (h_delta != 0)
    {
      gtk_adjustment_set_value (
        hadj,
        gtk_adjustment_get_value (hadj) + h_delta);
    }
    if (v_delta != 0)
    {
      gtk_adjustment_set_value (
        vadj,
        gtk_adjustment_get_value (vadj) + v_delta);
    }
  }
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
