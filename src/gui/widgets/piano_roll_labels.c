/*
 * gui/widgets/midi_note_labels.c- The midi
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

#include "gui/widgets/piano_roll_labels.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (PianoRollLabelsWidget,
               piano_roll_labels_widget,
               GTK_TYPE_DRAWING_AREA)

static char * notes[12] = {
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B" };

static void
draw_text (cairo_t *cr, int x, int y, char * text)
{
#define FONT "Sans Bold 9"

  PangoLayout *layout;
  PangoFontDescription *desc;

  cairo_move_to (cr, x, y);

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  pango_layout_set_text (layout, text, -1);
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
draw_cb (PianoRollLabelsWidget * self, cairo_t *cr, gpointer data)
{
  guint width, height;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, self->total_px, height);

  /* draw top line */


  /* draw note labels with bot lines */
  for (int i = 0; i < 128; i++)
    {
      int bot_line_px = self->total_px - self->px_per_note * i;
      cairo_move_to (cr, 0, bot_line_px);
      cairo_line_to (cr, width, bot_line_px);
      cairo_stroke (cr);

      char * text = g_strdup_printf ("%s%d",
                                     notes[i % 12],
                                     i / 12 - 2);
      draw_text (cr, 3, (bot_line_px - self->px_per_note) + 3,
                 text);
      g_free (text);
    }


  /* draw lines */
  /*int bar_count = 1;*/
  /*for (int i = 0; i < self->total_px - SPACE_BEFORE_START; i++)*/
    /*{*/
      /*int draw_pos = i + SPACE_BEFORE_START;*/

      /*if (i % self->px_per_bar == 0)*/
      /*{*/
          /*cairo_set_source_rgb (cr, 1, 1, 1);*/
          /*cairo_set_line_width (cr, 1);*/
          /*cairo_move_to (cr, draw_pos, 0);*/
          /*cairo_line_to (cr, draw_pos, height / 3);*/
          /*cairo_stroke (cr);*/
          /*cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);*/
          /*cairo_select_font_face(cr, FONT,*/
              /*CAIRO_FONT_SLANT_NORMAL,*/
              /*CAIRO_FONT_WEIGHT_NORMAL);*/
          /*cairo_set_font_size(cr, FONT_SIZE);*/
          /*gchar * label = g_strdup_printf ("%d", bar_count);*/
          /*static cairo_text_extents_t extents;*/
          /*cairo_text_extents(cr, label, &extents);*/
          /*cairo_move_to (cr,*/
                         /*(draw_pos ) - extents.width / 2,*/
                         /*(height / 2) + Y_SPACING);*/
          /*cairo_show_text(cr, label);*/
          /*bar_count++;*/
      /*}*/
      /*else if (i % self->px_per_beat == 0)*/
      /*{*/
          /*cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);*/
          /*cairo_set_line_width (cr, 0.5);*/
          /*cairo_move_to (cr, draw_pos, 0);*/
          /*cairo_line_to (cr, draw_pos, height / 4);*/
          /*cairo_stroke (cr);*/
      /*}*/
      /*else if (0)*/
        /*{*/

        /*}*/
    /*if (i == playhead_pos_in_px)*/
      /*{*/
          /*cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);*/
          /*cairo_set_line_width (cr, 2);*/
          /*cairo_move_to (cr,*/
                         /*draw_pos - PLAYHEAD_TRIANGLE_HALF_WIDTH,*/
                         /*height - PLAYHEAD_TRIANGLE_HEIGHT);*/
          /*cairo_line_to (cr, draw_pos, height);*/
          /*cairo_line_to (cr,*/
                         /*draw_pos + PLAYHEAD_TRIANGLE_HALF_WIDTH,*/
                         /*height - PLAYHEAD_TRIANGLE_HEIGHT);*/
          /*cairo_fill (cr);*/
      /*}*/
    /*if (i == cue_pos_in_px)*/
      /*{*/
          /*cairo_set_source_rgb (cr, 0, 0.6, 0.9);*/
          /*cairo_set_line_width (cr, 2);*/
          /*cairo_move_to (*/
            /*cr,*/
            /*draw_pos,*/
            /*((height - PLAYHEAD_TRIANGLE_HEIGHT) - Q_HEIGHT) - 1);*/
          /*cairo_line_to (*/
            /*cr,*/
            /*draw_pos + Q_WIDTH,*/
            /*((height - PLAYHEAD_TRIANGLE_HEIGHT) - Q_HEIGHT / 2) - 1);*/
          /*cairo_line_to (*/
            /*cr,*/
            /*draw_pos,*/
            /*(height - PLAYHEAD_TRIANGLE_HEIGHT) - 1);*/
          /*cairo_fill (cr);*/
      /*}*/
    /*if (i == start_marker_pos_px)*/
      /*{*/
          /*cairo_set_source_rgb (cr, 1, 0, 0);*/
          /*cairo_set_line_width (cr, 2);*/
          /*cairo_move_to (cr, draw_pos, 0);*/
          /*cairo_line_to (cr, draw_pos + START_MARKER_TRIANGLE_WIDTH,*/
                         /*0);*/
          /*cairo_line_to (cr, draw_pos,*/
                         /*START_MARKER_TRIANGLE_HEIGHT);*/
          /*cairo_fill (cr);*/
      /*}*/
    /*if (i == end_marker_pos_px)*/
      /*{*/
          /*cairo_set_source_rgb (cr, 1, 0, 0);*/
          /*cairo_set_line_width (cr, 2);*/
          /*cairo_move_to (cr, draw_pos - START_MARKER_TRIANGLE_WIDTH, 0);*/
          /*cairo_line_to (cr, draw_pos, 0);*/
          /*cairo_line_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT);*/
          /*cairo_fill (cr);*/
      /*}*/
    /*if (i == loop_start_pos_px)*/
      /*{*/
          /*cairo_set_source_rgb (cr, 0, 0.9, 0.7);*/
          /*cairo_set_line_width (cr, 2);*/
          /*cairo_move_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT + 1);*/
          /*cairo_line_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT * 2 + 1);*/
          /*cairo_line_to (cr, draw_pos + START_MARKER_TRIANGLE_WIDTH,*/
                         /*START_MARKER_TRIANGLE_HEIGHT + 1);*/
          /*cairo_fill (cr);*/
      /*}*/
    /*if (i == loop_end_pos_px)*/
      /*{*/
          /*cairo_set_source_rgb (cr, 0, 0.9, 0.7);*/
          /*cairo_set_line_width (cr, 2);*/
          /*cairo_move_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT + 1);*/
          /*cairo_line_to (cr, draw_pos - START_MARKER_TRIANGLE_WIDTH,*/
                         /*START_MARKER_TRIANGLE_HEIGHT + 1);*/
          /*cairo_line_to (cr, draw_pos,*/
                         /*START_MARKER_TRIANGLE_HEIGHT * 2 + 1);*/
          /*cairo_fill (cr);*/
      /*}*/

  /*}*/

 return FALSE;
}

static gboolean
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  return FALSE;
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
}

static void
piano_roll_labels_widget_class_init (PianoRollLabelsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "piano-roll-labels");
}

static void
piano_roll_labels_widget_init (PianoRollLabelsWidget * self)
{
  /* make it able to notify */
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);

  self->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));

  /* adjust for zoom level */
  self->zoom_level = 3;
  self->px_per_note = (DEFAULT_PX_PER_NOTE * self->zoom_level);
  self->total_px = self->px_per_note * 128;

  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    36,
    self->total_px);

  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
  /*g_signal_connect (G_OBJECT(self), "button_press_event",*/
                    /*G_CALLBACK (button_press_cb),  self);*/
  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
}

