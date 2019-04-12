/*
 * gui/widgets/chord.c- Chord widget on timeline.
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

#include "audio/chord.h"
#include "audio/chord_track.h"
#include "gui/widgets/chord.h"
#include "project.h"
#include "utils/ui.h"

G_DEFINE_TYPE (ChordWidget,
               chord_widget,
               GTK_TYPE_DRAWING_AREA)

static void
draw_text (cairo_t *cr, char * name)
{
#define FONT "Sans Bold 9"

  PangoLayout *layout;
  PangoFontDescription *desc;
  /*int i;*/

  cairo_translate (cr, 2, 2);

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  pango_layout_set_text (layout, name, -1);
  desc = pango_font_description_from_string (FONT);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  cairo_set_source_rgb (cr, 0, 0, 0);

  /* Inform Pango to re-layout the text with the new transformation */
  /*pango_cairo_update_layout (cr, layout);*/

  /*pango_layout_get_size (layout, &width, &height);*/
  /*cairo_move_to (cr, - ((double)width / PANGO_SCALE) / 2, - RADIUS);*/
  pango_cairo_show_layout (cr, layout);


  /* free the layout object */
  g_object_unref (layout);
}


static gboolean
draw_cb (ChordWidget * self,
         cairo_t *cr,
         gpointer data)
{
  guint width, height;
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (GTK_WIDGET (self));

  width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  GdkRGBA * color = &P_CHORD_TRACK->color;
  cairo_set_source_rgba (cr,
                         color->red - 0.06,
                         color->green - 0.06,
                         color->blue - 0.06,
                         0.7);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);
  cairo_set_source_rgba (cr,
                         color->red,
                         color->green,
                         color->blue,
                         1.0);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_set_line_width (cr, 3.5);
  cairo_stroke (cr);

  draw_text (cr, "A min");

 return FALSE;
}

void
chord_widget_select (ChordWidget * self,
                     int            select)
{
  self->chord->selected = select;
  if (select)
    {
      gtk_widget_set_state_flags (GTK_WIDGET (self),
                                  GTK_STATE_FLAG_SELECTED,
                                  0);
    }
  else
    {
      gtk_widget_unset_state_flags (GTK_WIDGET (self),
                                    GTK_STATE_FLAG_SELECTED);
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Sets hover in CSS.
 */
static void
on_motion (GtkWidget * widget, GdkEventMotion *event)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (widget,
                                  GTK_STATE_FLAG_PRELIGHT,
                                  0);
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (widget,
                                    GTK_STATE_FLAG_PRELIGHT);
    }
}

ChordWidget *
chord_widget_new (ZChord * chord)
{
  ChordWidget * self =
    g_object_new (CHORD_WIDGET_TYPE, NULL);

  self->chord = chord;

  /* connect signals */
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), self);
  g_signal_connect (G_OBJECT (self), "enter-notify-event",
                    G_CALLBACK (on_motion),  self);
  g_signal_connect (G_OBJECT(self), "leave-notify-event",
                    G_CALLBACK (on_motion),  self);

  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  return self;
}

static void
chord_widget_class_init (ChordWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "chord");
}

static void
chord_widget_init (ChordWidget * self)
{
}
