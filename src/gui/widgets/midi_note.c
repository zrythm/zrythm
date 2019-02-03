/*
 * gui/widgets/midi_note.c - MIDI Note
 *
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex@zrythm.org>
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

#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
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
draw_cb (MidiNoteWidget * self, cairo_t *cr, gpointer data)
{
  guint width, height;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  Region * region = (Region *) self->midi_note->midi_region;
  Track * track = region->track;
  /*Channel * channel = track_get_channel (track);*/
  GdkRGBA * color = &track->color;
  if (self->state != MNW_STATE_NONE)
    {
      cairo_set_source_rgba (cr,
                             color->red + 0.1,
                             color->green + 0.1,
                             color->blue + 0.1,
                             0.7);
    }
  else
    {
      cairo_set_source_rgba (cr, color->red, color->green, color->blue, 0.7);
    }
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_stroke_preserve(cr);
  cairo_fill(cr);

  /*draw_text (cr, self->midi_note->name);*/

 return FALSE;
}

static void
on_motion (GtkWidget * widget, GdkEventMotion *event)
{
  MidiNoteWidget * self = Z_MIDI_NOTE_WIDGET (widget);
  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);
  ArrangerWidgetPrivate * ar_prv =
    arranger_widget_get_private (
      Z_ARRANGER_WIDGET (MIDI_ARRANGER));

  if (event->type == GDK_MOTION_NOTIFY)
    {
      if (event->x < RESIZE_CURSOR_SPACE &&
          ar_prv->action != ARRANGER_ACTION_MOVING)
        {
          self->state = MNW_STATE_RESIZE_L;
          ui_set_cursor (widget, "w-resize");
        }

      else if (event->x > allocation.width - RESIZE_CURSOR_SPACE &&
          ar_prv->action != ARRANGER_ACTION_MOVING)
        {
          self->state = MNW_STATE_RESIZE_R;
          ui_set_cursor (widget, "e-resize");
        }
      else
        {
          if (self->state != MNW_STATE_SELECTED)
            self->state = MNW_STATE_HOVER;
          if (ar_prv->action !=
              ARRANGER_ACTION_MOVING &&
              ar_prv->action !=
              ARRANGER_ACTION_RESIZING_L &&
              ar_prv->action !=
              ARRANGER_ACTION_RESIZING_R)
            {
              ui_set_cursor (widget, "default");
            }
        }
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (self->state != MNW_STATE_SELECTED)
        self->state = MNW_STATE_NONE;
      if (ar_prv->action !=
          ARRANGER_ACTION_MOVING &&
          ar_prv->action !=
          ARRANGER_ACTION_RESIZING_L &&
          ar_prv->action !=
          ARRANGER_ACTION_RESIZING_R)
        {
          ui_set_cursor (widget, "default");
        }
    }
  g_idle_add ((GSourceFunc) gtk_widget_queue_draw, GTK_WIDGET (self));
}

MidiNoteWidget *
midi_note_widget_new (MidiNote * midi_note)
{
  g_message ("Creating midi_note widget...");
  MidiNoteWidget * self =
    g_object_new (MIDI_NOTE_WIDGET_TYPE,
                  "visible", 1,
                  NULL);

  self->midi_note = midi_note;

  return self;
}

static void
midi_note_widget_class_init (MidiNoteWidgetClass * klass)
{
}

static void
midi_note_widget_init (MidiNoteWidget * self)
{
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  self->drawing_area =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->drawing_area));

  /* connect signals */
  g_signal_connect (G_OBJECT (self->drawing_area), "draw",
                    G_CALLBACK (draw_cb), self);
  g_signal_connect (G_OBJECT (self), "enter-notify-event",
                    G_CALLBACK (on_motion),  self);
  g_signal_connect (G_OBJECT(self), "leave-notify-event",
                    G_CALLBACK (on_motion),  self);
  g_signal_connect (G_OBJECT(self), "motion-notify-event",
                    G_CALLBACK (on_motion),  self);

  /* set tooltip */
  gtk_widget_set_tooltip_text (GTK_WIDGET (self),
                               "Midi note");
}
