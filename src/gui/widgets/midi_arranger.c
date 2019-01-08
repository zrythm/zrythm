/*
 * gui/widgets/midi_arranger.c - The timeline containing regions
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
#include "settings.h"
#include "gui/widgets/region.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/arrays.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiArrangerWidget,
               midi_arranger_widget,
               ARRANGER_WIDGET_TYPE)

/**
 * Sets up the MIDI editor for the given region.
 */
void
midi_arranger_widget_set_channel (
  MidiArrangerWidget * self,
  Channel *            channel)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  if (channel->track->type != TRACK_TYPE_INSTRUMENT)
    {
      g_error ("Track is not an instrument track");
      return;
    }
  InstrumentTrack * it = (InstrumentTrack *) channel->track;
  if (self->channel)
    {
      channel_reattach_midi_editor_manual_press_port (
        self->channel,
        0);
    }
  channel_reattach_midi_editor_manual_press_port (
    channel,
    1);
  self->channel = channel;

  /*gtk_notebook_set_current_page (MAIN_WINDOW->bot_notebook, 0);*/

  char * label = g_strdup_printf ("%s",
                                  channel->name);
  gtk_label_set_text (PIANO_ROLL->midi_name_label,
                      label);
  g_free (label);

  color_area_widget_set_color (PIANO_ROLL->color_bar,
                               &channel->color);

  /* remove all previous children and add new */
  GList *children, *iter;
  children = gtk_container_get_children(GTK_CONTAINER(self));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    {
      if (iter->data != prv->bg)
        gtk_container_remove (GTK_CONTAINER (self),
                              GTK_WIDGET (iter->data));
    }
  g_list_free(children);
  for (int i = 0; i < it->num_regions; i++)
    {
      MidiRegion * region = it->regions[i];
      for (int j = 0; j < region->num_midi_notes; j++)
        {
          gtk_overlay_add_overlay (GTK_OVERLAY (self),
                                   GTK_WIDGET (region->midi_notes[j]->widget));
        }
    }
  gtk_widget_queue_allocate (GTK_WIDGET (self));
  gtk_widget_show_all (GTK_WIDGET (self));

  GtkAdjustment * adj =
    gtk_scrolled_window_get_vadjustment (
      PIANO_ROLL->arranger_scroll);
  gtk_adjustment_set_value (
    adj,
    gtk_adjustment_get_upper (adj) / 2);
  gtk_scrolled_window_set_vadjustment (
    PIANO_ROLL->arranger_scroll,
    adj);
}

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
  if (IS_MIDI_NOTE_WIDGET (widget))
    {
      MidiNoteWidget * midi_note_widget = MIDI_NOTE_WIDGET (widget);
      allocation->x =
        arranger_widget_pos_to_px (
          ARRANGER_WIDGET (self),
          &midi_note_widget->midi_note->start_pos);
      allocation->y =
        PIANO_ROLL->piano_roll_labels->px_per_note *
          (127 - midi_note_widget->midi_note->val);
      allocation->width =
        arranger_widget_pos_to_px (
          ARRANGER_WIDGET (self),
          &midi_note_widget->midi_note->end_pos) -
            allocation->x;
      allocation->height =
        PIANO_ROLL->piano_roll_labels->px_per_note;
    }
}

int
midi_arranger_widget_get_note_at_y (double y)
{
  return 128 - y / PIANO_ROLL_LABELS->px_per_note;
}

MidiNoteWidget *
midi_arranger_widget_get_hit_midi_note (MidiArrangerWidget *  self,
                                 double            x,
                                 double            y)
{
  GtkWidget * widget =
    arranger_widget_get_hit_widget (
      ARRANGER_WIDGET (self),
      ARRANGER_CHILD_TYPE_MIDI_NOTE,
      x,
      y);
  if (widget)
    {
      return MIDI_NOTE_WIDGET (widget);
    }
  return NULL;
}


void
midi_arranger_widget_update_inspector (MidiArrangerWidget *self)
{
  inspector_widget_show_selections (INSPECTOR_CHILD_MIDI,
                                    (void **) self->midi_notes,
                                 self->num_midi_notes);
}

void
midi_arranger_widget_select_all (MidiArrangerWidget *  self,
                                 int               select)
{
  /* TODO */
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
midi_arranger_widget_on_drag_begin_note_hit (
  MidiArrangerWidget * self,
  MidiNoteWidget *     midi_note_widget)
{
  /* set selections, positions, actions, cursor */
  ARRANGER_WIDGET_GET_PRIVATE (self);
  MidiNote * midi_note = midi_note_widget->midi_note;
  prv->start_pos_px =
    ruler_widget_pos_to_px (RULER_WIDGET (MIDI_RULER),
                            &midi_note->start_pos);
  position_set_to_pos (&prv->start_pos, &midi_note->start_pos);
  position_set_to_pos (&prv->end_pos, &midi_note->end_pos);

  /* if already in selected notes, prepare to do action on all of them,
   * otherwise change selection to just this note */
  self->start_midi_note = midi_note;
  if (!array_contains ((void **) self->midi_notes,
                       self->num_midi_notes,
                       (void *) midi_note))
    {
      self->midi_notes[0] = midi_note;
      self->num_midi_notes = 1;
    }
  switch (midi_note_widget->state)
    {
    case MNW_STATE_NONE:
      g_warning ("hitting midi note but midi note hover state is none, should be fixed");
      break;
    case MNW_STATE_RESIZE_L:
      prv->action = ARRANGER_ACTION_RESIZING_L;
      break;
    case MNW_STATE_RESIZE_R:
      prv->action = ARRANGER_ACTION_RESIZING_R;
      break;
    case MNW_STATE_HOVER:
    case MNW_STATE_SELECTED:
      prv->action = ARRANGER_ACTION_STARTING_MOVING;
      ui_set_cursor (GTK_WIDGET (midi_note_widget), "grabbing");
      break;
    }
}

/**
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 */
void
midi_arranger_widget_on_drag_begin_create_note (
  MidiArrangerWidget * self,
  Position * pos,
  int                  note,
  MidiRegion * region)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  if (SNAP_GRID_ANY_SNAP(prv->snap_grid))
    position_snap (NULL,
                   pos,
                   NULL,
                   (Region *) region,
                   prv->snap_grid);
  MidiNote * midi_note = midi_note_new (region,
                                   pos,
                                   pos,
                                   note,
                                   -1);
  position_set_min_size (&midi_note->start_pos,
                         &midi_note->end_pos,
                         prv->snap_grid);
  midi_region_add_midi_note (region,
                        midi_note);
  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                           GTK_WIDGET (midi_note->widget));
  gtk_widget_show (GTK_WIDGET (midi_note->widget));
  prv->action = ARRANGER_ACTION_RESIZING_R;
  self->midi_notes[0] = midi_note;
  self->num_midi_notes = 1;
}

/**
 * Called when in selection mode.
 *
 * Called by arranger widget during drag_update to find and
 * select the midi notes enclosed in the selection area.
 */
void
midi_arranger_widget_find_and_select_midi_notes (
  MidiArrangerWidget * self,
  double               offset_x,
  double               offset_y)
{
  ArrangerWidget * aw = ARRANGER_WIDGET (self);
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* find enclosed midi notes */
  GtkWidget *    midi_note_widgets[800];
  int            num_midi_note_widgets = 0;
  arranger_widget_get_hit_widgets_in_range (
    aw,
    ARRANGER_CHILD_TYPE_MIDI_NOTE,
    prv->start_x,
    prv->start_y,
    offset_x,
    offset_y,
    midi_note_widgets,
    &num_midi_note_widgets);

  /* select the enclosed midi_notes */
  for (int i = 0; i < num_midi_note_widgets; i++)
    {
      MidiNoteWidget * midi_note_widget =
        MIDI_NOTE_WIDGET (midi_note_widgets[i]);
      MidiNote * midi_note = midi_note_widget->midi_note;
      self->midi_notes[self->num_midi_notes++] = midi_note;
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
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * midi_note = self->midi_notes[i];
      if (SNAP_GRID_ANY_SNAP(prv->snap_grid))
        position_snap (NULL,
                       pos,
                       NULL,
                       (Region *) midi_note->midi_region,
                       prv->snap_grid);
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
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * midi_note = self->midi_notes[i];
      if (SNAP_GRID_ANY_SNAP(prv->snap_grid))
        position_snap (NULL,
                       pos,
                       NULL,
                       (Region *) midi_note->midi_region,
                       prv->snap_grid);
      if (position_compare (pos, &midi_note->start_pos) > 0)
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
midi_arranger_widget_move_midi_notes_x (
  MidiArrangerWidget *self,
  Position *          pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* snap first selected midi note's pos */
  if (SNAP_GRID_ANY_SNAP(prv->snap_grid))
    position_snap (NULL,
                   pos,
                   NULL,
                   (Region *) self->start_midi_note->midi_region,
                   prv->snap_grid);
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * midi_note = self->midi_notes[i];

      /* get adjusted pos for this midi note */
      Position midi_note_pos;
      position_set_to_pos (&midi_note_pos,
                           pos);
      int diff = position_to_frames (&midi_note->start_pos) -
        position_to_frames (&self->start_midi_note->start_pos);
      position_add_frames (&midi_note_pos, diff);

      midi_note_set_start_pos (midi_note,
                            &midi_note_pos);
    }
}

/**
 * Called when moving midi notes in drag update in arranger
 * widget for moving up/down (changing note).
 */
void
midi_arranger_widget_move_midi_notes_y (
  MidiArrangerWidget *self,
  double              offset_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * midi_note = self->midi_notes[i];
      /*midi_note_set_end_pos (midi_note,*/
                             /*&pos);*/
      /* check if should be moved to new note  */
      midi_note->val = midi_arranger_widget_get_note_at_y (
        prv->start_y + offset_y);
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
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * midi_note = self->midi_notes[i];
      ui_set_cursor (GTK_WIDGET (midi_note->widget), "default");
    }
  self->start_midi_note = NULL;
}

void
midi_arranger_widget_toggle_select_midi_note (
  MidiArrangerWidget * self,
  MidiNote *           midi_note,
  int                  append)
{
  arranger_widget_toggle_select (
    ARRANGER_WIDGET (self),
    ARRANGER_CHILD_TYPE_MIDI_NOTE,
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
  /* TODO */
}

static void
midi_arranger_widget_class_init (MidiArrangerWidgetClass * klass)
{
}

static void
midi_arranger_widget_init (MidiArrangerWidget *self)
{
}
