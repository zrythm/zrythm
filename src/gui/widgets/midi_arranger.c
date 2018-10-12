/*
 * gui/widgets/timeline.c - The timeline containing regions
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

#include "zrythm_app.h"
#include "project.h"
#include "settings_manager.h"
#include "gui/widgets/region.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_editor.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline.h"
#include "gui/widgets/tracks.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiArrangerWidget, midi_arranger_widget, GTK_TYPE_OVERLAY)

#define MW_RULER MAIN_WINDOW->ruler

/**
 * Sets up the MIDI editor for the given region.
 */
void
midi_arranger_widget_set_channel (Channel * channel)
{
  /*if (SELECTIONS.num_channels > 0 &&*/
      /*SELECTIONS.channels[SELECTIONS.num_channels - 1] == channel)*/

  selections_set_channel (channel);

  gtk_notebook_set_current_page (MAIN_WINDOW->bot_notebook, 0);

  char * label = g_strdup_printf ("%s",
                                  channel->name);
  gtk_label_set_text (MIDI_EDITOR->midi_name_label,
                      label);
  g_free (label);

  color_area_widget_set_color (MIDI_EDITOR->midi_track_color,
                               &channel->color);

  /* remove all previous children and add new */
  GList *children, *iter;
  children = gtk_container_get_children(GTK_CONTAINER(MIDI_EDITOR->midi_arranger));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    {
      if (iter->data !=  MIDI_EDITOR->midi_arranger->bg)
        gtk_container_remove (GTK_CONTAINER (MIDI_EDITOR->midi_arranger),
                              GTK_WIDGET (iter->data));
    }
  g_list_free(children);
  for (int i = 0; i < channel->track->num_regions; i++)
    {
      Region * region = channel->track->regions[i];
      for (int j = 0; j < region->num_midi_notes; j++)
        {
          gtk_overlay_add_overlay (GTK_OVERLAY (MIDI_EDITOR->midi_arranger),
                                   GTK_WIDGET (region->midi_notes[j]->widget));
        }
    }
  gtk_widget_queue_allocate (GTK_WIDGET (MIDI_EDITOR->midi_arranger));
  gtk_widget_show_all (GTK_WIDGET (MIDI_EDITOR->midi_arranger));
}

/**
 * Gets called to set the position/size of each overlayed widget.
 */
static gboolean
get_child_position (GtkOverlay   *overlay,
               GtkWidget    *widget,
               GdkRectangle *allocation,
               gpointer      user_data)
{
  if (IS_MIDI_NOTE_WIDGET (widget))
    {
      MidiNoteWidget * midi_note_widget = MIDI_NOTE_WIDGET (widget);


      allocation->x =
        (midi_note_widget->midi_note->start_pos.bars - 1) * MW_RULER->px_per_bar +
        (midi_note_widget->midi_note->start_pos.beats - 1) * MW_RULER->px_per_beat +
        (midi_note_widget->midi_note->start_pos.quarter_beats - 1) * MW_RULER->px_per_quarter_beat +
        midi_note_widget->midi_note->start_pos.ticks * MW_RULER->px_per_tick +
        SPACE_BEFORE_START;
      allocation->y = MAIN_WINDOW->midi_editor->piano_roll_labels->px_per_note *
        midi_note_widget->midi_note->val;
      allocation->width =
        ((midi_note_widget->midi_note->end_pos.bars - 1) * MW_RULER->px_per_bar +
        (midi_note_widget->midi_note->end_pos.beats - 1) * MW_RULER->px_per_beat +
        (midi_note_widget->midi_note->end_pos.quarter_beats - 1) * MW_RULER->px_per_quarter_beat +
        midi_note_widget->midi_note->end_pos.ticks * MW_RULER->px_per_tick +
        SPACE_BEFORE_START) - allocation->x;
      allocation->height = MAIN_WINDOW->midi_editor->piano_roll_labels->px_per_note;
    }

  return TRUE;
}

static MidiNoteWidget *
get_hit_midi_note_widget (double x, double y)
{
  GList *children, *iter;

  /* go through each overlay child */
  children = gtk_container_get_children(GTK_CONTAINER(MAIN_WINDOW->midi_editor->midi_arranger));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);

      /* if region */
      if (IS_MIDI_NOTE_WIDGET (widget))
        {
          GtkAllocation allocation;
          gtk_widget_get_allocation (widget,
                                     &allocation);

          gint wx, wy;
          gtk_widget_translate_coordinates(
                    GTK_WIDGET (widget),
                    GTK_WIDGET (MAIN_WINDOW->midi_editor->midi_arranger),
                    0,
                    0,
                    &wx,
                    &wy);

          /* if hit */
          if (x >= wx &&
              x <= (wx + allocation.width) &&
              y >= wy &&
              y <= (wy + allocation.height))
            {
              return MIDI_NOTE_WIDGET (widget);
            }
        }
    }
  return NULL;
}


static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  MidiArrangerWidget * self = (MidiArrangerWidget *) user_data;
  gtk_widget_grab_focus (GTK_WIDGET (self));
  self->n_press = n_press;

  /* open MIDI editor if double clicked on a region */
  MidiNoteWidget * midi_note_widget = get_hit_midi_note_widget (x, y);
  if (midi_note_widget && n_press == 2)
    {
      /* TODO */
      g_message ("note double clicked ");
    }
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  MidiArrangerWidget * self = (MidiArrangerWidget *) user_data;
  self->start_x = start_x;

  MidiNoteWidget * midi_note_widget = get_hit_midi_note_widget (start_x, start_y);
  if (midi_note_widget)
    {
      self->midi_note = midi_note_widget->midi_note;

      switch (midi_note_widget->hover_state)
        {
        case MIDI_NOTE_HOVER_STATE_NONE:
          g_warning ("hitting midi note but midi note hover state is none, should be fixed");
          break;
        case MIDI_NOTE_HOVER_STATE_EDGE_L:
          self->action = MAA_RESIZING_NOTE_L;
          break;
        case MIDI_NOTE_HOVER_STATE_EDGE_R:
          self->action = MAA_RESIZING_NOTE_R;
          break;
        case MIDI_NOTE_HOVER_STATE_MIDDLE:
          self->action = MAA_MOVING_NOTE;
          ui_set_cursor (GTK_WIDGET (midi_note_widget), "grabbing");
          break;
        }
    }
  else /* no note hit */
    {
      if (self->n_press == 1)
        {
          /* area selection */

        }
      else if (self->n_press == 2)
        {
          Position pos;
          ruler_widget_px_to_pos (
                               &pos,
                               start_x - SPACE_BEFORE_START);
          int note = (start_y) /
            MIDI_EDITOR->piano_roll_labels->px_per_note;

          /* if inside a region */
          Region * region = region_at_position (
                      SELECTIONS.channels[SELECTIONS.num_channels - 1]->track,
                      &pos);
          if (region)
            {
              self->midi_note = midi_note_new (region,
                                               &pos,
                                               &pos,
                                               note,
                                               -1);
              region_add_midi_note (region,
                                    self->midi_note);
              gtk_overlay_add_overlay (GTK_OVERLAY (self),
                                       GTK_WIDGET (self->midi_note->widget));
              gtk_widget_queue_allocate (GTK_WIDGET (self));
              self->action = MAA_RESIZING_NOTE_R;
            }
        }
    }
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  MidiArrangerWidget * self = MIDI_ARRANGER_WIDGET (user_data);

  if (self->action == MAA_RESIZING_NOTE_L)
    {
      Position pos;
      ruler_widget_px_to_pos (&pos,
                 (self->start_x + offset_x) - SPACE_BEFORE_START);
      midi_note_set_start_pos (self->midi_note,
                            &pos);
    }
  else if (self->action == MAA_RESIZING_NOTE_R)
    {
      Position pos;
      ruler_widget_px_to_pos (&pos,
                 (self->start_x + offset_x) - SPACE_BEFORE_START);
      midi_note_set_end_pos (self->midi_note,
                            &pos);
    }
  else if (self->action == MAA_MOVING_NOTE)
    {
      Position pos;

      /* move region start & end on x-axis */
      int start_px = ruler_widget_pos_to_px (&self->midi_note->start_pos);
      ruler_widget_px_to_pos (&pos,
                              start_px + (offset_x - self->last_offset_x));
      midi_note_set_start_pos (self->midi_note,
                            &pos);
      start_px = ruler_widget_pos_to_px (&self->midi_note->end_pos);
      ruler_widget_px_to_pos (&pos,
                              start_px + (offset_x - self->last_offset_x));
      midi_note_set_end_pos (self->midi_note,
                          &pos);

    }
  gtk_widget_queue_allocate(
              GTK_WIDGET (self));
  gtk_widget_queue_draw (GTK_WIDGET (self));
  gtk_widget_show_all (GTK_WIDGET (self));
  self->last_offset_x = offset_x;
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  MidiArrangerWidget * self = (MidiArrangerWidget *) user_data;
  self->start_x = 0;
  self->last_offset_x = 0;
  self->action = MAA_NONE;
  if (self->midi_note)
    {
      ui_set_cursor (GTK_WIDGET (self->midi_note->widget), "default");
    }
  self->midi_note = NULL;
}


MidiArrangerWidget *
midi_arranger_widget_new ()
{
  g_message ("Creating timeline...");
  MidiArrangerWidget * self = g_object_new (MIDI_ARRANGER_WIDGET_TYPE, NULL);
  MAIN_WINDOW->midi_editor->midi_arranger = self;

  self->bg = midi_arranger_bg_widget_new ();

  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->bg));

  /* make it able to notify */
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);
  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);

  g_signal_connect (G_OBJECT (self), "get-child-position",
                    G_CALLBACK (get_child_position), NULL);
  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);


  return self;
}

static void
midi_arranger_widget_class_init (MidiArrangerWidgetClass * klass)
{
}

static void
midi_arranger_widget_init (MidiArrangerWidget *self )
{
  self->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
}

