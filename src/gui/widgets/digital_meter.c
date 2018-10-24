/*
 * gui/widgets/digital_meter.c - Digital meter for BPM, position, etc.
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

#include <stdlib.h>
#include <math.h>

#include "zrythm_app.h"
#include "project.h"
#include "settings_manager.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "gui/widgets/digital_meter.h"

#include <gtk/gtk.h>

#include <pango/pangofc-fontmap.h>


G_DEFINE_TYPE (DigitalMeterWidget, digital_meter_widget, GTK_TYPE_DRAWING_AREA)

#define FONT "Monospace"
#define FONT_SIZE 16
#define SPACE_BETWEEN 6
#define HALF_SPACE_BETWEEN 3
#define PADDING 2

static gboolean
draw_cb (DigitalMeterWidget * self, cairo_t *cr, gpointer data)
{
  guint width, height;
  /*GdkRGBA color;*/
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  /*double bg_txt_alpha = 0.08;*/

  cairo_text_extents_t te10, te5, te1, te3, te2;
  /*char * test_text = "8";*/
  char * text;
  int num_part, dec_part, bars, beats, quarter_beats, ticks;
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:

      /* fill text */
      num_part = (int) TRANSPORT->bpm;
      dec_part = (int) (TRANSPORT->bpm * 100) % 100;
      cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
      cairo_select_font_face (cr, "Segment7",
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size (cr, 24.0);
      cairo_text_extents (cr, "8", &te1);
      cairo_text_extents (cr, "88", &te2);
      cairo_text_extents (cr, "888", &te3);
      cairo_text_extents (cr, "88888", &te5);

      self->num_part_start_pos = ((width / 2) - te5.width / 2) - HALF_SPACE_BETWEEN;
      self->num_part_end_pos = self->num_part_start_pos + te3.width;
      self->dec_part_start_pos = self->num_part_end_pos + SPACE_BETWEEN;
      self->dec_part_end_pos = self->dec_part_start_pos + te2.width;
      self->height_start_pos = (height / 2 - te1.height / 2);
      self->height_end_pos = self->height_start_pos + te1.height;

      /* draw integer part */
      if (num_part < 100)
        text = g_strdup_printf (" %d", num_part);
      else
        text = g_strdup_printf ("%d", num_part);
      cairo_move_to (cr,
                     self->num_part_start_pos,
                     self->height_end_pos);
      cairo_show_text (cr, text);
      g_free (text);

      /* draw decimal part */
      if (dec_part < 10)
        text = g_strdup_printf ("0%d", dec_part);
      else
        text = g_strdup_printf ("%d", dec_part);
      cairo_move_to (cr,
                     self->dec_part_start_pos,
                     self->height_end_pos);
      cairo_show_text (cr, text);
      g_free (text);

      /* draw line */
      cairo_move_to (cr, 0, 0);
      cairo_line_to (cr, width, 0);
      cairo_stroke (cr);

      break;

    case DIGITAL_METER_TYPE_POSITION:

      bars = TRANSPORT->playhead_pos.bars;
      beats = TRANSPORT->playhead_pos.beats;
      quarter_beats = TRANSPORT->playhead_pos.quarter_beats;
      ticks = TRANSPORT->playhead_pos.ticks;

      /* fill text */
      cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
      cairo_select_font_face (cr, "Segment7",
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size (cr, 24.0);
      cairo_text_extents (cr, "8", &te1);
      cairo_text_extents (cr, "88", &te2);
      cairo_text_extents (cr, "888", &te3);
      cairo_text_extents (cr, "8888888888", &te10);
      self->bars_start_pos = ((width / 2) - te10.width / 2) - HALF_SPACE_BETWEEN * 3;
      self->bars_end_pos = self->bars_start_pos + te3.width;
      self->beats_start_pos = self->bars_end_pos + SPACE_BETWEEN;
      self->beats_end_pos = self->beats_start_pos + te1.width;
      self->quarter_beats_start_pos = self->beats_end_pos + SPACE_BETWEEN;
      self->quarter_beats_end_pos = self->quarter_beats_start_pos + te1.width;
      self->ticks_start_pos = self->quarter_beats_end_pos + SPACE_BETWEEN;
      self->ticks_end_pos = self->ticks_start_pos + te3.width;
      self->height_start_pos = (height / 2 - te1.height / 2);
      self->height_end_pos = self->height_start_pos + te1.height;

      if (bars < 10)
        text = g_strdup_printf ("  %d", bars);
      else if (bars < 100)
        text = g_strdup_printf (" %d", bars);
      else
        text = g_strdup_printf ("%d", bars);
      cairo_move_to (cr,
                     self->bars_start_pos,
                     self->height_end_pos);
      cairo_show_text (cr, text);
      g_free (text);

      text = g_strdup_printf ("%d", beats);
      cairo_move_to (cr,
                     self->beats_start_pos,
                     self->height_end_pos);
      cairo_show_text (cr, text);
      g_free (text);

      text = g_strdup_printf ("%d", quarter_beats);
      cairo_move_to (cr,
                     self->quarter_beats_start_pos,
                     self->height_end_pos);
      cairo_show_text (cr, text);
      g_free (text);

      if (ticks < 10)
        text = g_strdup_printf ("00%d", ticks);
      else if (ticks < 100)
        text = g_strdup_printf ("0%d", ticks);
      else
        text = g_strdup_printf ("%d", ticks);
      cairo_move_to (cr,
                     self->ticks_start_pos,
                     self->height_end_pos);
      cairo_show_text (cr, text);
      g_free (text);

      cairo_move_to (cr, 0, 0);
      cairo_line_to (cr, width, 0);
      cairo_stroke (cr);

      break;
    case DIGITAL_METER_TYPE_TIMESIG:

      /* fill text */
      cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
      /*cairo_select_font_face (cr, "Segment7",*/
                              /*CAIRO_FONT_SLANT_NORMAL,*/
                              /*CAIRO_FONT_WEIGHT_BOLD);*/
      cairo_set_font_size (cr, 16.0);
      cairo_text_extents (cr, "8", &te1);
      cairo_text_extents (cr, "88", &te2);
      cairo_text_extents (cr, "888", &te3);

      /*self->num_part_start_pos = ((width / 2) - te5.width / 2) - HALF_SPACE_BETWEEN;*/
      /*self->num_part_end_pos = self->num_part_start_pos + te3.width;*/
      /*self->dec_part_start_pos = self->num_part_end_pos + SPACE_BETWEEN;*/
      /*self->dec_part_end_pos = self->dec_part_start_pos + te2.width;*/
      self->height_start_pos = (height / 2 - te1.height / 2);
      self->height_end_pos = self->height_start_pos + te1.height;

      /* draw integer part */
      /*if (num_part < 100)*/
        /*text = g_strdup_printf (" %d", num_part);*/
      /*else*/
        /*text = g_strdup_printf ("%d", num_part);*/
      text = snap_grid_stringize (self->snap_grid);
      cairo_move_to (cr,
                     0,
                     self->height_end_pos);
      cairo_show_text (cr, text);
      g_free (text);

      /* draw line */
      cairo_move_to (cr, 0, 0);
      cairo_line_to (cr, width, 0);
      cairo_stroke (cr);

      break;
    }

 return FALSE;
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  DigitalMeterWidget * self = (DigitalMeterWidget *) user_data;
  offset_y = - offset_y;
  double diff = offset_y - self->last_y;
  int num;
  float dec;
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:
      /*g_message ("update num ? %d", self->update_num);*/
      if (self->update_num)
        {
          num = (int) diff / 4;
          /*g_message ("updating num with %d", num);*/
          if (abs (num) > 0)
            {
              transport_set_bpm (TRANSPORT->bpm + num);
              self->last_y = offset_y;
            }
        }
      else if (self->update_dec)
        {
          dec = (float) diff / 400.f;
          g_message ("%f", dec);
          if (fabs (dec) > 0)
            {
              transport_set_bpm (TRANSPORT->bpm + dec);
              self->last_y = offset_y;
            }

        }

      break;

    case DIGITAL_METER_TYPE_POSITION:
      if (self->update_bars)
        {
          num = (int) diff / 4;
          /*g_message ("updating num with %d", num);*/
          if (abs (num) > 0)
            {
              position_set_bar (&TRANSPORT->playhead_pos,
                                TRANSPORT->playhead_pos.bars + num);
              self->last_y = offset_y;
            }
        }
      else if (self->update_beats)
        {
          num = (int) diff / 4;
          /*g_message ("updating num with %d", num);*/
          if (abs (num) > 0)
            {
              position_set_beat (&TRANSPORT->playhead_pos,
                                TRANSPORT->playhead_pos.beats + num);
              self->last_y = offset_y;
            }
        }
      else if (self->update_quarter_beats)
        {
          num = (int) diff / 4;
          /*g_message ("updating num with %d", num);*/
          if (abs (num) > 0)
            {
              position_set_quarter_beat (&TRANSPORT->playhead_pos,
                                TRANSPORT->playhead_pos.quarter_beats + num);
              self->last_y = offset_y;
            }
        }
      else if (self->update_ticks)
        {
          num = (int) diff / 4;
          /*g_message ("updating num with %d", num);*/
          if (abs (num) > 0)
            {
              position_set_tick (&TRANSPORT->playhead_pos,
                                TRANSPORT->playhead_pos.ticks + num);
              self->last_y = offset_y;
            }
        }

      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      if (self->update_dens)
        {
          num = (int) diff / 4;
          /*g_message ("updating num with %d", num);*/
          if (num > 0)
            {
              for (int i = 0; i < 7; i++)
                {
                  if (self->snap_grid->grid_density == i)
                    {
                      self->snap_grid->grid_density++;
                      break;
                    }
                }
            }
          else if (num < 0)
            {
              for (int i = 1; i < 8; i++)
                {
                  if (self->snap_grid->grid_density == i)
                    {
                      self->snap_grid->grid_density--;
                      break;
                    }
                }
            }
            self->last_y = offset_y;
        }
      break;
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  DigitalMeterWidget * self = (DigitalMeterWidget *) user_data;
  self->last_y = 0;
  self->update_num = 0;
  self->update_dec = 0;
  self->update_bars = 0;
  self->update_beats = 0;
  self->update_quarter_beats = 0;
  self->update_ticks = 0;
  self->update_dens = 0;
}

static gboolean
button_press_cb (GtkWidget      * event_box,
                 GdkEventButton * event,
                 gpointer       data)
{
  DigitalMeterWidget * self = (DigitalMeterWidget *) data;
  /*g_message ("%d, %d", self->height_start_pos, self->height_end_pos);*/
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:
      if (event->y >= self->height_start_pos &&
          event->y <= self->height_end_pos)
        {

          if (event->x >= self->num_part_start_pos &&
              event->x <= self->num_part_end_pos)
            {
              self->update_num = 1;
            }
          else if (event->x >= self->dec_part_start_pos &&
                   event->x <= self->dec_part_end_pos)
            {
              self->update_dec = 1;
            }
        }
      break;

    case DIGITAL_METER_TYPE_POSITION:
      if (event->y >= self->height_start_pos &&
          event->y <= self->height_end_pos)
        {

          if (event->x >= self->bars_start_pos &&
              event->x <= self->bars_end_pos)
            {
              self->update_bars = 1;
            }
          else if (event->x >= self->beats_start_pos &&
                   event->x <= self->beats_end_pos)
            {
              self->update_beats = 1;
            }
          else if (event->x >= self->quarter_beats_start_pos &&
                   event->x <= self->quarter_beats_end_pos)
            {
              self->update_quarter_beats = 1;
            }
          else if (event->x >= self->ticks_start_pos &&
                   event->x <= self->ticks_end_pos)
            {
              self->update_ticks = 1;
            }
        }

      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      self->update_dens = 1;

      break;
    }
  return 0;
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
DigitalMeterWidget *
digital_meter_widget_new (DigitalMeterType      type,
                          SnapGrid *            sg)
{
  g_message ("Creating digital meter...");
  DigitalMeterWidget * self = g_object_new (DIGITAL_METER_WIDGET_TYPE, NULL);

  self->type = type;
  switch (type)
    {
    case DIGITAL_METER_TYPE_BPM:
      // set the size
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        90,
        -1);

      break;
    case DIGITAL_METER_TYPE_POSITION:
      // set the size
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        160,
        -1);

      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      // set the size
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        -1,
        30);
      self->snap_grid = sg;
      break;
    }

  /* make it able to notify */
  gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);
  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new (GTK_WIDGET (self)));

  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT(self), "button_press_event",
                    G_CALLBACK (button_press_cb),  self);

  return self;
}

static void
digital_meter_widget_class_init (DigitalMeterWidgetClass * klass)
{
}

static void
digital_meter_widget_init (DigitalMeterWidget * self)
{
  /* FIXME doesn't work */
  /*FcConfig * font_config;*/

  /*PangoFontMap *font_map;*/
  /*PangoFontDescription *font_desc;*/

  /*if (g_once_init_enter (&font_config))*/
    /*{*/
      /*FcConfig *config = FcInitLoadConfigAndFonts ();*/

      /*const gchar * font_path = "resources/fonts/Segment7Standard/Segment7Standard.ttf";*/

      /*if (!g_file_test (font_path, G_FILE_TEST_IS_REGULAR))*/
        /*g_warning ("Failed to locate \"%s\"", font_path);*/


      /*FcConfigAppFontAddFile (config, (const FcChar8 *)font_path);*/
      /*g_message ("aa");*/

      /*g_once_init_leave (&font_config, config);*/
    /*}*/

  /*font_map = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);*/
  /*pango_fc_font_map_set_config (PANGO_FC_FONT_MAP (font_map), font_config);*/
  /*gtk_widget_set_font_map (GTK_WIDGET (self), font_map);*/

  /*g_assert (font_config != NULL);*/
  /*g_assert (font_map != NULL);*/

  /*g_object_unref (font_map);*/

}
