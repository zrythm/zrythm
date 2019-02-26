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
      MidiNoteWidget* zMidiNoteWidget = Z_MIDI_NOTE_WIDGET (widget);
      self->start_midi_note = zMidiNoteWidget->midi_note;
      return Z_MIDI_NOTE_WIDGET (widget);
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
  if (!PIANO_ROLL->region)
    return;

  MIDI_ARRANGER_SELECTIONS->num_midi_notes = 0;

  /* select midi notes */
  MidiRegion * mr =
    (MidiRegion *) PIANO_ROLL_SELECTED_REGION;
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
        }
      else
        {
          array_delete (
            MIDI_ARRANGER_SELECTIONS->midi_notes,
            MIDI_ARRANGER_SELECTIONS->num_midi_notes,
            midi_note);
        }
    }
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
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    rw_prv->total_px,
    hh);
}

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

void
midi_arranger_widget_on_drag_begin_note_hit (
  MidiArrangerWidget * self,
  GdkModifierType          state_mask,
  double                   start_x,
  MidiNoteWidget *     midi_note_widget)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* update arranger action */
  if (midi_note_widget->cursor_state ==
        UI_CURSOR_STATE_RESIZE_L)
    ar_prv->action = UI_OVERLAY_ACTION_RESIZING_L;
  else if (midi_note_widget->cursor_state ==
             UI_CURSOR_STATE_RESIZE_R)
    ar_prv->action = UI_OVERLAY_ACTION_RESIZING_R;
  else
    {
      ar_prv->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      ui_set_cursor (
        GTK_WIDGET (midi_note_widget), "grabbing");
    }

  /* select/ deselect regions */
  MidiNote * midi_note =
    midi_note_widget->midi_note;
  if (state_mask & GDK_SHIFT_MASK ||
      state_mask & GDK_CONTROL_MASK)
    {
      /* if ctrl pressed toggle on/off */
      midi_arranger_widget_toggle_select_midi_note (
        self, midi_note, 1);
    }
  else if (!array_contains (
            (void **)MIDI_ARRANGER_SELECTIONS->midi_notes,
            MIDI_ARRANGER_SELECTIONS->num_midi_notes,
            midi_note))
    {
      /* else if not already selected select only it */
      midi_arranger_widget_select_all (self, 0);
      midi_arranger_widget_toggle_select_midi_note (self, midi_note, 0);
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
  gtk_widget_show (GTK_WIDGET (midi_note->widget));
  ar_prv->action = UI_OVERLAY_ACTION_RESIZING_R;
  MIDI_ARRANGER_SELECTIONS->midi_notes[0] =
    midi_note;
  MIDI_ARRANGER_SELECTIONS->num_midi_notes = 1;
}

/**
 * Called when in selection mode.
 *
 * Called by arranger widget during drag_update to find and
 * select the midi notes enclosed in the selection area.
 */
void
midi_arranger_widget_select (
  MidiArrangerWidget * self,
  double               offset_x,
  double               offset_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

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

  /* select the enclosed midi_notes */
  for (int i = 0; i < num_midi_note_widgets; i++)
    {
      MidiNoteWidget * midi_note_widget =
        Z_MIDI_NOTE_WIDGET (midi_note_widgets[i]);
      MidiNote * midi_note =
        midi_note_widget->midi_note;
      midi_arranger_widget_toggle_select_midi_note (
        self,
        midi_note,
        1);
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
    }
}


/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
static void
calc_deltamax_for_note_movement (int *y_delta)
{
  for (int i = 0; i < MIDI_ARRANGER_SELECTIONS->num_midi_notes; i++){

      MidiNote * midi_note =
      MIDI_ARRANGER_SELECTIONS->midi_notes[i];
      if (midi_note->val + *y_delta < 0)
	{
	  *y_delta = 0;
	}
      if (midi_note->val + *y_delta >= 127)
	{
	  *y_delta = 127 - midi_note->val;

	}
    }
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
  int ar_end_val = midi_arranger_widget_get_note_at_y (
      ar_prv->start_y + offset_y);
  y_delta = ar_end_val - ar_start_val;
  if (ar_end_val != ar_start_val)
    {
      calc_deltamax_for_note_movement (&y_delta);
//      g_message ("MidiNote drag: off: %2u, start_note:%3u,end_note:%4u",
//		 offset_y, ar_start_val, ar_end_val);
      for (int i = 0; i < MIDI_ARRANGER_SELECTIONS->num_midi_notes; i++)
	{
	  MidiNote * midi_note =
	  MIDI_ARRANGER_SELECTIONS->midi_notes[i];
	  /*midi_note_set_end_pos (midi_note,*/
	  /*&deltamax);*/
	  /* check if should be moved to new note  */
	  int old_val = midi_note->val;

	  midi_note->val = midi_note->val + y_delta;
	  /* check if should be moved to     new note */
	  int val = midi_note->val;
	  if (val < 128 && val >= 0)
	    {
	      int pval = old_val - 1;
	      int nval = old_val + 1;
	      if (midi_note->val != val)
		{
		  /* if new val is lower and bot midinote is not at the lowest val */
		  if (val == nval &&
		  MIDI_ARRANGER_SELECTIONS->bot_midi_note->val != 0)
		    {
		      /* shift all selected regions to their next track */
		      for (int i = 0;
			  i < MIDI_ARRANGER_SELECTIONS->num_midi_notes; i++)
			{
			  MidiNote * midi_note =
			  MIDI_ARRANGER_SELECTIONS->midi_notes[i];
			  nval = midi_note->val + 1;
			  old_val = midi_note->val;
			  midi_note->val = nval;
			}
		    }
		  else if (val == pval &&
		  MIDI_ARRANGER_SELECTIONS->top_midi_note->val != 127)
		    {
		      /* shift all selected midi_notes to their prev track */
		      for (int i = 0;
			  i < MIDI_ARRANGER_SELECTIONS->num_midi_notes; i++)
			{
			  MidiNote * midi_note =
			  MIDI_ARRANGER_SELECTIONS->midi_notes[i];
			  pval = midi_note->val - 1;
			  old_val = midi_note->val;
			  midi_note->val = pval;
			}
		    }
		}
	    }
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
  for (int i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * midi_note =
        MIDI_ARRANGER_SELECTIONS->midi_notes[i];
      ui_set_cursor (
        GTK_WIDGET (midi_note->widget), "default");
    }
  self->start_midi_note = NULL;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * midi_note =
        MIDI_ARRANGER_SELECTIONS->midi_notes[i];
      ui_set_cursor (
        GTK_WIDGET (midi_note->widget), "default");
    }

  /* if didn't click on something */
  if (ar_prv->action !=
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      self->start_midi_note = NULL;
    }
}

void
midi_arranger_widget_toggle_select_midi_note (
  MidiArrangerWidget * self,
  MidiNote *           midi_note,
  int                  append)
{
  arranger_widget_toggle_select (
    Z_ARRANGER_WIDGET (self),
    MIDI_NOTE_WIDGET_TYPE,
    (void *) midi_note,
    append);
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

  if (PIANO_ROLL->region)
    {
      MidiRegion * mr =
        (MidiRegion *) PIANO_ROLL->region;
      for (int j = 0; j < mr->num_midi_notes; j++)
        {
          MidiNote * midi_note = mr->midi_notes[j];
          gtk_overlay_add_overlay (
            GTK_OVERLAY (self),
            GTK_WIDGET (midi_note->widget));
        }
    }
}

static void
midi_arranger_widget_class_init (MidiArrangerWidgetClass * klass)
{
}

static void
midi_arranger_widget_init (MidiArrangerWidget *self)
{
}
