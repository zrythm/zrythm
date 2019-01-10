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
#include "gui/widgets/audio_unit.h"
#include "gui/widgets/audio_unit_label.h"
#include "utils/gtk.h"

G_DEFINE_TYPE (AudioUnitLabelWidget, audio_unit_label_widget, GTK_TYPE_BOX)

static void
draw_text (cairo_t *cr, char * name)
{
#define FONT "Sans Bold 9"

  PangoLayout *layout;
  PangoFontDescription *desc;

  cairo_translate (cr, 2, 2);

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  pango_layout_set_text (layout, name, -1);
  desc = pango_font_description_from_string (FONT);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  /* Inform Pango to re-layout the text with the new transformation */
  /*pango_cairo_update_layout (cr, layout);*/

  /*pango_layout_get_size (layout, &width, &height);*/
  /*cairo_move_to (cr, - ((double)width / PANGO_SCALE) / 2, - RADIUS);*/
  pango_cairo_show_layout (cr, layout);


  /* free the layout object */
  g_object_unref (layout);
#undef FONT
}

static int
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  AudioUnitLabelWidget * self = (AudioUnitLabelWidget *) data;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  Port * port = self->port;
  if ((!self->parent->is_right && self->type == AUL_TYPE_LEFT) ||
      (self->parent->is_right && self->type == AUL_TYPE_RIGHT))
    {
      cairo_set_source_rgba (cr, 0, 0, 0, 1);
    }
  else if (port->type == TYPE_AUDIO)
    {
      cairo_set_source_rgba (cr, 1, 0, 0, 1);
    }
  else if (port->type == TYPE_EVENT)
    {
      cairo_set_source_rgba (cr, 0, 0, 1, 1);
    }
  else if (port->type == TYPE_CONTROL)
    {
      cairo_set_source_rgba (cr, 0, 1, 0, 1);
    }
  else
    {
      cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 1);
    }
  draw_text (cr, self->port->label);

  return FALSE;
}


static void
on_crossing (GtkWidget * widget, GdkEvent *event)
{
  AudioUnitLabelWidget * self =
    Z_AUDIO_UNIT_LABEL_WIDGET (widget);
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
                             Port *  port,
                             AudioUnitWidget * parent)
{
  AudioUnitLabelWidget * self = g_object_new (AUDIO_UNIT_LABEL_WIDGET_TYPE,
                                              "orientation",
                                              GTK_ORIENTATION_HORIZONTAL,
                                              NULL);
  self->type = type;
  self->port = port;
  self->parent = parent;
  self->da = GTK_DRAWING_AREA (gtk_drawing_area_new ());
  self->drag_box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

  if (type == AUL_TYPE_LEFT)
    {
      gtk_box_pack_start (GTK_BOX (self),
                          GTK_WIDGET (self->drag_box),
                          Z_GTK_EXPAND,
                          Z_GTK_FILL,
                          0);
      gtk_box_pack_end (GTK_BOX (self),
                        GTK_WIDGET (self->da),
                        Z_GTK_EXPAND,
                        Z_GTK_FILL,
                        0);
    }
  else
    {
      gtk_box_pack_start (GTK_BOX (self),
                          GTK_WIDGET (self->da),
                          Z_GTK_EXPAND,
                          Z_GTK_FILL,
                          0);
      gtk_box_pack_end (GTK_BOX (self),
                        GTK_WIDGET (self->drag_box),
                        Z_GTK_EXPAND,
                        Z_GTK_FILL,
                        0);
    }
  gtk_widget_show_all (GTK_WIDGET (self->da));
  gtk_widget_show_all (GTK_WIDGET (self->drag_box));

  /* set sizes */
  gtk_widget_set_size_request (GTK_WIDGET (self->da), -1, 20);
  gtk_widget_set_size_request (GTK_WIDGET (self->drag_box), 10, -1);

  /* connect signals */
  g_signal_connect (G_OBJECT (self->da), "draw",
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



