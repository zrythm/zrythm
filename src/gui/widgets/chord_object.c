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

#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/chord_object.h"
#include "project.h"
#include "utils/ui.h"

G_DEFINE_TYPE (ChordObjectWidget,
               chord_object_widget,
               GTK_TYPE_BOX)

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
chord_draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  ChordObjectWidget * self)
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

/**
 * Sets the "selected" GTK state flag and adds the
 * note to TimelineSelections.
 */
void
chord_object_widget_select (
  ChordObjectWidget * self,
  int              select)
{
  ChordObject * main_chord =
    chord_object_get_main_chord_object (
      self->chord);
  if (select)
    {
      timeline_selections_add_chord (
        TL_SELECTIONS,
        main_chord);
    }
  else
    {
      timeline_selections_remove_chord (
        TL_SELECTIONS,
        main_chord);
    }
  EVENTS_PUSH (ET_CHORD_CHANGED,
               main_chord);
}

static gboolean
on_motion (
  GtkWidget *      widget,
  GdkEventMotion * event,
  ChordObjectWidget * self)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
      bot_bar_change_status (
        "Chord - Click and drag to move around ("
        "hold Shift to disable snapping)");
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }

  return FALSE;
}

ChordObjectWidget *
chord_object_widget_new (ChordObject * chord)
{
  ChordObjectWidget * self =
    g_object_new (CHORD_OBJECT_WIDGET_TYPE, NULL);

  self->chord = chord;

  return self;
}

static void
chord_object_widget_class_init (
  ChordObjectWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "chord-object");
}

static void
chord_object_widget_init (
  ChordObjectWidget * self)
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

  /* GDK_ALL_EVENTS_MASK is needed, otherwise the
   * grab gets broken */
  gtk_widget_add_events (
    GTK_WIDGET (self->drawing_area),
    GDK_ALL_EVENTS_MASK);

  g_signal_connect (
    G_OBJECT (self->drawing_area), "draw",
    G_CALLBACK (chord_draw_cb), self);
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

  g_object_ref (self);
}
