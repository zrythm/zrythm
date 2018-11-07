/*
 * gui/widgets/audio_unit_label.c - audio_unit_label widget
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

/** \file
 */

#include "audio/port.h"
#include "gui/widgets/audio_unit_label.h"

G_DEFINE_TYPE (AudioUnitLabelWidget, audio_unit_label_widget, GTK_TYPE_DRAWING_AREA)


static int
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  /*AudioUnitLabelWidget * self = (AudioUnitLabelWidget *) widget;*/
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  return FALSE;
}


static void
on_crossing (GtkWidget * widget, GdkEvent *event)
{
  AudioUnitLabelWidget * self = AUDIO_UNIT_LABEL_WIDGET (widget);
  int type = gdk_event_get_event_type (event);
  if (type == GDK_ENTER_NOTIFY)
    {
      self->hover = 1;
    }
  else if (type == GDK_LEAVE_NOTIFY)
    {
      self->hover = 0;
    }
  gtk_widget_queue_draw(widget);
}

/**
 * Creates a new AudioUnitLabel widget and binds it to the given value.
 */
AudioUnitLabelWidget *
audio_unit_label_widget_new (AULType type,
                             Port *  port)
{
  AudioUnitLabelWidget * self = g_object_new (AUDIO_UNIT_LABEL_WIDGET_TYPE, NULL);
  self->type = type;
  self->port = port;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self), 120, 20);

  /* connect signals */
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), self);
  g_signal_connect (G_OBJECT (self), "enter-notify-event",
                    G_CALLBACK (on_crossing),  self);
  g_signal_connect (G_OBJECT(self), "leave-notify-event",
                    G_CALLBACK (on_crossing),  self);

  return self;
}

static void
audio_unit_label_widget_init (AudioUnitLabelWidget * self)
{
  /* make it able to notify */
  /*gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);*/
  /*int crossing_mask = GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;*/
  /*gtk_widget_add_events (GTK_WIDGET (self), crossing_mask);*/
}

static void
audio_unit_label_widget_class_init (AudioUnitLabelWidgetClass * klass)
{
}



