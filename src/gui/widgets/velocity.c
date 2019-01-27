/*
 * gui/widgets/velocity.c - velocity for MIDI notes
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

/** \file */

#include "audio/channel.h"
#include "audio/channel_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "gui/widgets/velocity.h"

G_DEFINE_TYPE (VelocityWidget,
               velocity_widget,
               GTK_TYPE_BOX)

static gboolean
draw_cb (GtkDrawingArea * da,
         cairo_t *cr,
         gpointer data)
{
  VelocityWidget * self = Z_VELOCITY_WIDGET (data);
  guint width, height;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  Region * region =
    (Region *) self->velocity->midi_note->midi_region;
  GdkRGBA * color = &region->track->color;

  cairo_set_source_rgba (cr,
                         color->red - 0.2,
                         color->green - 0.2,
                         color->blue - 0.2,
                         0.7);

  /* TODO draw audio notes */
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);

 return FALSE;
}

/**
 * Creates a velocity.
 */
VelocityWidget *
velocity_widget_new (Velocity * velocity)
{
  VelocityWidget * self =
    g_object_new (VELOCITY_WIDGET_TYPE,
                  NULL);

  self->velocity = velocity;

  return self;
}

static void
velocity_widget_class_init (VelocityWidgetClass * klass)
{
}

static void
velocity_widget_init (VelocityWidget * self)
{
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  self->drawing_area = GTK_DRAWING_AREA (
    gtk_drawing_area_new ());
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->drawing_area));
  gtk_widget_set_visible (GTK_WIDGET (self->drawing_area),
                          1);
  gtk_widget_set_hexpand (GTK_WIDGET (self->drawing_area),
                          1);

  /* connect signals */
  g_signal_connect (G_OBJECT (self->drawing_area), "draw",
                    G_CALLBACK (draw_cb), self);
  /*g_signal_connect (G_OBJECT (self), "enter-notify-event",*/
                    /*G_CALLBACK (on_motion),  self);*/
  /*g_signal_connect (G_OBJECT(self), "leave-notify-event",*/
                    /*G_CALLBACK (on_motion),  self);*/
  /*g_signal_connect (G_OBJECT(self), "motion-notify-event",*/
                    /*G_CALLBACK (on_motion),  self);*/

  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  /* set tooltip */
  gtk_widget_set_tooltip_text (GTK_WIDGET (self),
                               "Velocity");
}
