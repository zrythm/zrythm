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

/**
 * \file
 *
 */

#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/ruler.h"
#include "utils/ui.h"

G_DEFINE_TYPE (MidiNoteWidget,
               midi_note_widget,
               GTK_TYPE_BOX)

/**
 * Space on the edges to show resize cursors
 */
#define RESIZE_CURSOR_SPACE 9

static gboolean
draw_cb (GtkDrawingArea * widget, cairo_t *cr,
         MidiNoteWidget * self)
{
  guint width, height;
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (GTK_WIDGET (self));

  width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  gtk_render_background (
    context, cr, 0, 0, width, height);

  Region * region =
    (Region *) self->midi_note->midi_region;
  Track * track = region->track;
  /*Channel * channel = track_get_channel (track);*/
  GdkRGBA * color = &track->color;
  cairo_set_source_rgba (cr,
                         color->red,
                         color->green,
                         color->blue,
                         0.7);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_stroke_preserve(cr);
  cairo_fill(cr);

  /*draw_text (cr, self->midi_note->name);*/

 return FALSE;
}

static int
on_motion (GtkWidget *      widget,
           GdkEventMotion * event,
           MidiNoteWidget * self)
{
  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);
  ARRANGER_WIDGET_GET_PRIVATE (MIDI_ARRANGER);

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
    }
  if (event->type == GDK_MOTION_NOTIFY)
    {
      if (event->x < RESIZE_CURSOR_SPACE)
        {
          self->cursor_state =
            UI_CURSOR_STATE_RESIZE_L;
          if (ar_prv->action !=
                UI_OVERLAY_ACTION_MOVING)
            ui_set_cursor (widget, "w-resize");
        }

      else if (event->x > allocation.width -
                 RESIZE_CURSOR_SPACE)
        {
          self->cursor_state =
            UI_CURSOR_STATE_RESIZE_R;
          if (ar_prv->action !=
                UI_OVERLAY_ACTION_MOVING)
            ui_set_cursor (widget, "e-resize");
        }
      else
        {
          self->cursor_state =
            UI_CURSOR_STATE_DEFAULT;
          if (ar_prv->action !=
                UI_OVERLAY_ACTION_MOVING &&
              ar_prv->action !=
                UI_OVERLAY_ACTION_STARTING_MOVING &&
              ar_prv->action !=
                UI_OVERLAY_ACTION_RESIZING_L &&
              ar_prv->action !=
                UI_OVERLAY_ACTION_RESIZING_R)
            {
              ui_set_cursor (widget, "default");
            }
        }
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (ar_prv->action !=
            UI_OVERLAY_ACTION_MOVING &&
          ar_prv->action !=
            UI_OVERLAY_ACTION_RESIZING_L &&
          ar_prv->action !=
            UI_OVERLAY_ACTION_RESIZING_R)
        {
          ui_set_cursor (widget, "default");
          gtk_widget_unset_state_flags (
            GTK_WIDGET (self),
            GTK_STATE_FLAG_PRELIGHT);
        }
    }

  return FALSE;
}

void
midi_note_widget_update_tooltip (
  MidiNoteWidget * self,
  int              show)
{
  const char * note =
    chord_note_to_string (
      self->midi_note->val % 12);

  /* set tooltip text */
  char * tooltip =
    g_strdup_printf (
      "[%s%d] %d",
      note,
      self->midi_note->val / 12 - 2,
      self->midi_note->vel->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);

  /* set tooltip window */
  if (show)
    {
      tooltip =
        g_strdup_printf (
          "%s%d",
          note,
          self->midi_note->val / 12 - 2);
      gtk_label_set_text (self->tooltip_label,
                          tooltip);
      gtk_window_present (self->tooltip_win);

      g_free (tooltip);
    }
  else
    gtk_widget_hide (
      GTK_WIDGET (self->tooltip_win));
}

void
midi_note_widget_select (MidiNoteWidget * self,
                      int            select)
{
  self->midi_note->selected = select;
  if (select)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_SELECTED,
        0);
    }
  else
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_SELECTED);
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
  gtk_widget_queue_draw (GTK_WIDGET (self->midi_note->vel->widget));
}

MidiNoteWidget *
midi_note_widget_new (MidiNote * midi_note)
{
  MidiNoteWidget * self =
    g_object_new (MIDI_NOTE_WIDGET_TYPE,
                  "visible", 1,
                  NULL);

  self->midi_note = midi_note;

  /* set tooltip text */
  char * tooltip =
    g_strdup_printf (
      "[%s%d] %d",
      chord_note_to_string (
        self->midi_note->val % 12),
      self->midi_note->val / 12 - 2,
      self->midi_note->vel->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);

  return self;
}

static void
midi_note_widget_class_init (
  MidiNoteWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "midi-note");
}

static void
midi_note_widget_init (MidiNoteWidget * self)
{
  self->drawing_area =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_widget_set_hexpand (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->drawing_area));

  gtk_widget_add_events (
    GTK_WIDGET (self->drawing_area),
    GDK_ALL_EVENTS_MASK);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self->drawing_area), "draw",
    G_CALLBACK (draw_cb), self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);

  /* set tooltip window */
  self->tooltip_win =
    GTK_WINDOW (gtk_window_new (GTK_WINDOW_POPUP));
  gtk_window_set_type_hint (
    self->tooltip_win,
    GDK_WINDOW_TYPE_HINT_TOOLTIP);
  self->tooltip_label =
    GTK_LABEL (gtk_label_new ("label"));
  gtk_widget_set_visible (
    GTK_WIDGET (self->tooltip_label), 1);
  gtk_container_add (
    GTK_CONTAINER (self->tooltip_win),
    GTK_WIDGET (self->tooltip_label));
  gtk_window_set_position (
    self->tooltip_win, GTK_WIN_POS_MOUSE);
}
