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

#include <stdlib.h>
#include <math.h>

#include "audio/position.h"
#include "audio/quantize.h"
#include "audio/snap_grid.h"
#include "audio/transport.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (DigitalMeterWidget,
               digital_meter_widget,
               GTK_TYPE_DRAWING_AREA)

#define FONT_SIZE 16
#define SEG7_FONT "Segment7 Bold 16"
#define NORMAL_FONT "sans-serif Bold 11"
#define CAPTION_FONT "sans-serif 7"
#define SPACE_BETWEEN 6
#define HALF_SPACE_BETWEEN 2
#define PADDING_W 4
#define PADDING_TOP 0

#define SET_POS \
  ((*self->setter) (self->obj, &pos))
#define GET_POS \
  ((*self->getter) (self->obj, &pos))

static gboolean
draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  DigitalMeterWidget * self)
{
  guint width, height;
  /*GdkRGBA color;*/
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (widget);
  width =
    gtk_widget_get_allocated_width (widget);
  height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /* draw caption and get its extents */
  int caption_textw, caption_texth;
  /*GdkRGBA color;*/
  /*gdk_rgba_parse (&color, "#00ccff");*/
  /*cairo_set_source_rgba (*/
    /*cr, color.red, color.green, color.blue, 1.0);*/
  cairo_set_source_rgba (
    cr, 1.0, 1.0, 1.0, 1.0);
  z_cairo_get_text_extents_for_widget_full (
    GTK_WIDGET (self), self->caption,
    &caption_textw, &caption_texth,
    CAPTION_FONT);
  z_cairo_draw_text_full (
    cr, self->caption, width / 2 - caption_textw / 2,
    PADDING_TOP, CAPTION_FONT);
  /* uncomment to make text slightly thickerr */
  /*z_cairo_draw_text_full (*/
    /*cr, self->caption, width / 2 - caption_textw / 2,*/
    /*PADDING_TOP, CAPTION_FONT);*/

  char * text = NULL;
  int num_part, dec_part, bars, beats, sixteenths, ticks;
  int textw, texth;
  Position pos;
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:

      num_part = (int) TRANSPORT->bpm;
      dec_part = (int) (TRANSPORT->bpm * 100) % 100;

      z_cairo_get_text_extents_for_widget_full (
        widget, "88888", &textw, &texth,
        SEG7_FONT);
      self->num_part_start_pos =
        ((width / 2) - textw / 2) -
        HALF_SPACE_BETWEEN;
      z_cairo_get_text_extents_for_widget_full (
        widget, "888", &textw, &texth,
        SEG7_FONT);
      self->num_part_end_pos =
        self->num_part_start_pos + textw;
      self->dec_part_start_pos =
        self->num_part_end_pos + SPACE_BETWEEN;
      z_cairo_get_text_extents_for_widget_full (
        widget, "88", &textw, &texth,
        SEG7_FONT);
      self->dec_part_end_pos =
        self->dec_part_start_pos + textw;
      self->height_start_pos =
        PADDING_TOP +
        caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos =
        self->height_start_pos + texth;

      /* draw integer part */
      if (num_part < 100)
        text = g_strdup_printf (" %d", num_part);
      else
        text = g_strdup_printf ("%d", num_part);
      z_cairo_draw_text_full (
        cr, text, self->num_part_start_pos,
        self->height_start_pos, SEG7_FONT);
      g_free (text);

      /* draw decimal part */
      if (dec_part < 10)
        text = g_strdup_printf ("0%d", dec_part);
      else
        text = g_strdup_printf ("%d", dec_part);
      z_cairo_draw_text_full (
        cr, text, self->dec_part_start_pos,
        self->height_start_pos, SEG7_FONT);
      g_free (text);

      break;
    case DIGITAL_METER_TYPE_POSITION:

      GET_POS;
      bars = pos.bars;
      beats = pos.beats;
      sixteenths = pos.sixteenths;
      ticks = pos.ticks;

      z_cairo_get_text_extents_for_widget_full (
        widget, "-8888888888", &textw, &texth,
        SEG7_FONT);
      self->bars_start_pos =
        ((width / 2) - textw / 2) -
        HALF_SPACE_BETWEEN * 3;
      z_cairo_get_text_extents_for_widget_full (
        widget, "-888", &textw, &texth,
        SEG7_FONT);
      self->bars_end_pos =
        self->bars_start_pos + textw;
      self->beats_start_pos =
        self->bars_end_pos + SPACE_BETWEEN;
      z_cairo_get_text_extents_for_widget_full (
        widget, "8", &textw, &texth,
        SEG7_FONT);
      self->beats_end_pos =
        self->beats_start_pos + textw;
      self->sixteenths_start_pos =
        self->beats_end_pos + SPACE_BETWEEN;
      self->sixteenths_end_pos =
        self->sixteenths_start_pos + textw;
      self->ticks_start_pos =
        self->sixteenths_end_pos + SPACE_BETWEEN;
      z_cairo_get_text_extents_for_widget_full (
        widget, "888", &textw, &texth,
        SEG7_FONT);
      self->ticks_end_pos =
        self->ticks_start_pos + textw;
      self->height_start_pos =
        PADDING_TOP +
        caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos =
        self->height_start_pos + texth;

      if (bars < -100)
        text = g_strdup_printf ("%d", bars);
      else if (bars < -10)
        text = g_strdup_printf (" %d", bars);
      else if (bars < 0)
        text = g_strdup_printf ("  %d", bars);
      else if (bars < 10)
        text = g_strdup_printf ("   %d", bars);
      else if (bars < 100)
        text = g_strdup_printf ("  %d", bars);
      else
        text = g_strdup_printf (" %d", bars);
      z_cairo_draw_text_full (
        cr, text, self->bars_start_pos,
        self->height_start_pos, SEG7_FONT);
      g_free (text);

      text =
        g_strdup_printf ("%d", abs (beats));
      z_cairo_draw_text_full (
        cr, text, self->beats_start_pos,
        self->height_start_pos, SEG7_FONT);
      g_free (text);

      text =
        g_strdup_printf ("%d", abs (sixteenths));
      z_cairo_draw_text_full (
        cr, text, self->sixteenths_start_pos,
        self->height_start_pos, SEG7_FONT);
      g_free (text);

      if (abs (ticks) < 10)
        text = g_strdup_printf ("00%d", abs (ticks));
      else if (abs (ticks) < 100)
        text = g_strdup_printf ("0%d", abs (ticks));
      else
        text = g_strdup_printf ("%d", abs (ticks));
      z_cairo_draw_text_full (
        cr, text, self->ticks_start_pos,
        self->height_start_pos, SEG7_FONT);
      g_free (text);

      break;
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      text =
        snap_grid_stringize (
          *self->note_length,
          *self->note_type);
      z_cairo_get_text_extents_for_widget_full (
        widget, text, &textw, &texth,
        NORMAL_FONT);
      self->height_start_pos =
        PADDING_TOP +
        caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos =
        self->height_start_pos + texth;
      z_cairo_draw_text_full (
        cr, text, width / 2 - textw / 2,
        self->height_start_pos, NORMAL_FONT);
      g_free (text);

      break;
    case DIGITAL_METER_TYPE_NOTE_TYPE:
      switch (*self->note_type)
        {
        case NOTE_TYPE_NORMAL:
          text = _("normal");
          break;
        case NOTE_TYPE_DOTTED:
          text = _("dotted");
          break;
        case NOTE_TYPE_TRIPLET:
          text = _("triplet");
          break;
        }
      z_cairo_get_text_extents_for_widget_full (
        widget, text, &textw, &texth,
        NORMAL_FONT);
      self->height_start_pos =
        PADDING_TOP +
        caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos =
        self->height_start_pos + texth;
      z_cairo_draw_text_full (
        cr, text, width / 2 - textw / 2,
        self->height_start_pos, NORMAL_FONT);

      break;
    case DIGITAL_METER_TYPE_TIMESIG:

      z_cairo_get_text_extents_for_widget_full (
        widget, "16/16", &textw, &texth,
        SEG7_FONT);
      self->height_start_pos =
        PADDING_TOP +
        caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos =
        self->height_start_pos + texth;

      char * beat_unit = NULL;
      switch (TRANSPORT->beat_unit)
        {
        case 2:
          beat_unit = " 2";
          break;
        case 4:
          beat_unit = " 4";
          break;
        case 8:
          beat_unit = " 8";
          break;
        case 16:
          beat_unit = "16";
          break;
        default:
          g_warn_if_reached ();
          break;
        }
      char * beats_per_bar;
      if (TRANSPORT->beats_per_bar < 10)
        beats_per_bar =
          g_strdup_printf (" %d",
                           TRANSPORT->beats_per_bar);
      else
        beats_per_bar =
          g_strdup_printf ("%d",
                           TRANSPORT->beats_per_bar);
      text = g_strdup_printf ("%s/%s",
                              beats_per_bar,
                              beat_unit);
      g_free (beats_per_bar);
      z_cairo_draw_text_full (
        cr, text, width / 2 - textw / 2,
        self->height_start_pos, SEG7_FONT);
      g_free (text);

      break;
    }

  if (self->draw_line)
    {
      cairo_set_line_width (cr, 1.0);
      cairo_move_to (
        cr, 0,
        caption_texth +
        PADDING_TOP);
      cairo_line_to (
        cr, width,
        caption_texth +
        PADDING_TOP);
      cairo_stroke (cr);
    }

 return FALSE;
}

void
digital_meter_set_draw_line (
  DigitalMeterWidget * self,
  int                  draw_line)
{
  self->draw_line = draw_line;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
drag_start (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  DigitalMeterWidget * self = (DigitalMeterWidget *) user_data;
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      self->start_note_length =
        *self->note_length;
      break;
    case DIGITAL_METER_TYPE_NOTE_TYPE:
      self->start_note_type =
        *self->note_type;
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      self->start_timesig_top =
        TRANSPORT->beats_per_bar;
      self->start_timesig_bot =
        TRANSPORT->ebeat_unit;
      break;
    case DIGITAL_METER_TYPE_POSITION:
      if (self->on_drag_begin)
        ((*self->on_drag_begin) (self->obj));
      break;
    default:
      break;
    }
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
  Position pos;
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
      GET_POS;
      if (self->update_bars)
        {
          num = (int) diff / 4;
          if (abs (num) > 0)
            {
              position_set_bar (
                &pos, pos.bars + num);
              self->last_y = offset_y;
            }
        }
      else if (self->update_beats)
        {
          num = (int) diff / 4;
          if (abs (num) > 0)
            {
              position_set_beat (
                &pos,
                pos.beats + num);
              self->last_y = offset_y;
            }
        }
      else if (self->update_sixteenths)
        {
          num = (int) diff / 4;
          /*g_message ("updating num with %d", num);*/
          if (abs (num) > 0)
            {
              position_set_sixteenth (
                &pos,
                pos.sixteenths + num);
              self->last_y = offset_y;
            }
        }
      else if (self->update_ticks)
        {
          num = (int) diff / 4;
          /*g_message ("updating num with %d", num);*/
          if (abs (num) > 0)
            {
              position_set_tick (
                &pos,
                pos.ticks + num);
              self->last_y = offset_y;
            }
        }
      SET_POS;

      break;
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      if (self->update_note_length)
        {
          num = self->start_note_length + (int) offset_y / 24;
          if (num < 0)
            *self->note_length = 0;
          else
            *self->note_length =
              num > NOTE_LENGTH_1_128 ?
              NOTE_LENGTH_1_128 : num;
        }
      break;
    case DIGITAL_METER_TYPE_NOTE_TYPE:
      if (self->update_note_type)
        {
          num = self->start_note_type + (int) offset_y / 24;
          if (num < 0)
            *self->note_type = 0;
          else
            *self->note_type =
              num > NOTE_TYPE_TRIPLET ?
              NOTE_TYPE_TRIPLET : num;
        }
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      if (self->update_timesig_top)
        {
          num = self->start_timesig_top + (int) offset_y / 24;
          if (num < 1)
            {
              TRANSPORT->beats_per_bar = 1;
            }
          else
            {
              TRANSPORT->beats_per_bar =
                num > 16 ?
                16 : num;
            }
        }
      else if (self->update_timesig_bot)
        {
          num = self->start_timesig_bot + (int) offset_y / 24;
          if (num < 0)
            {
              transport_set_ebeat_unit (
                TRANSPORT, BEAT_UNIT_2);
            }
          else
            {
              transport_set_ebeat_unit (
                TRANSPORT,
                num > BEAT_UNIT_16 ?
                BEAT_UNIT_16 : num);
            }
        }
      if (self->update_timesig_top ||
          self->update_timesig_bot)
        {
          EVENTS_PUSH (
            ET_TIME_SIGNATURE_CHANGED,
            NULL);
        }
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
  self->update_sixteenths = 0;
  self->update_ticks = 0;
  /* FIXME super reduntant */
  if (self->update_note_length ||
      self->update_note_type)
    {
      snap_grid_update_snap_points (
        &PROJECT->snap_grid_timeline);
      snap_grid_update_snap_points (
        &PROJECT->snap_grid_midi);
      quantize_update_snap_points (
        &PROJECT->quantize_timeline);
      quantize_update_snap_points (
        &PROJECT->quantize_midi);
    }
  self->update_note_length = 0;
  self->update_note_type = 0;
  self->update_timesig_top = 0;
  self->update_timesig_bot = 0;

  switch (self->type)
    {
    case DIGITAL_METER_TYPE_POSITION:
      if (self->on_drag_end)
        ((*self->on_drag_end) (self->obj));
      break;
    default:
      break;
    }
}

static gboolean
button_press_cb (GtkWidget      * event_box,
                 GdkEventButton * event,
                 gpointer       data)
{
  DigitalMeterWidget * self = (DigitalMeterWidget *) data;
  /*g_message ("%d, %d", self->height_start_pos, self->height_end_pos);*/
  int width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));
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
          else if (event->x >= self->sixteenths_start_pos &&
                   event->x <= self->sixteenths_end_pos)
            {
              self->update_sixteenths = 1;
            }
          else if (event->x >= self->ticks_start_pos &&
                   event->x <= self->ticks_end_pos)
            {
              self->update_ticks = 1;
            }
        }

      break;
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      self->update_note_length = 1;
      break;
    case DIGITAL_METER_TYPE_NOTE_TYPE:
      self->update_note_type = 1;
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      if (event->x <= width / 2)
        {
          self->update_timesig_top = 1;
        }
      else
        {
          self->update_timesig_bot = 1;
        }
      break;
    }
  return 0;
}

static void
init_dm (
  DigitalMeterWidget * self)
{
  int caption_textw, caption_texth;
  z_cairo_get_text_extents_for_widget_full (
    GTK_WIDGET (self), self->caption, &caption_textw,
    &caption_texth, CAPTION_FONT);
  int textw, texth;
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:
      {
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self),
          _("BPM - Click and drag up/down to set"));
        z_cairo_get_text_extents_for_widget_full (
          GTK_WIDGET (self), "888888", &textw,
          &texth, SEG7_FONT);
        /* caption + padding between caption and
         * BPM + padding top/bottom */
        gtk_widget_set_size_request (
          GTK_WIDGET (self), textw + PADDING_W * 2,
          caption_texth + HALF_SPACE_BETWEEN +
          texth + PADDING_TOP * 2);
      }
      break;
    case DIGITAL_METER_TYPE_POSITION:
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self),
        _("BPM - Click and drag up/down to set"));
      z_cairo_get_text_extents_for_widget_full (
        GTK_WIDGET (self), "-888888888", &textw,
        &texth, SEG7_FONT);
      /* caption + padding between caption and
       * BPM + padding top/bottom */
      gtk_widget_set_size_request (
        GTK_WIDGET (self), textw + PADDING_W * 2,
        caption_texth + HALF_SPACE_BETWEEN +
        texth + PADDING_TOP * 2);

      break;
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        -1,
        30);
      break;

    case DIGITAL_METER_TYPE_NOTE_TYPE:
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        -1,
        30);
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      {
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self),
          _("Time Signature - Click and drag "
            "up/down to change"));
        z_cairo_get_text_extents_for_widget_full (
          GTK_WIDGET (self), "16/16", &textw,
          &texth, SEG7_FONT);
        /* caption + padding between caption and
         * BPM + padding top/bottom */
        gtk_widget_set_size_request (
          GTK_WIDGET (self), textw + PADDING_W * 2,
          caption_texth + HALF_SPACE_BETWEEN +
          texth + PADDING_TOP * 2);
      }
      break;
    }

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (draw_cb), self);
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
DigitalMeterWidget *
digital_meter_widget_new (
  DigitalMeterType  type,
  NoteLength *      note_length,
  NoteType *        note_type,
  const char *      caption)
{
  g_message ("Creating digital meter...");
  DigitalMeterWidget * self = g_object_new (DIGITAL_METER_WIDGET_TYPE, NULL);

  self->type = type;
  self->caption = g_strdup (caption);
  self->note_length = note_length;
  self->note_type = note_type;
  init_dm (self);

  return self;
}

/**
 * Creates a digital meter for an arbitrary position.
 *
 * @param obj The object to call the get/setters with.
 *
 *   E.g. Region.
 * @param get_val The getter func to get the position,
 *   passing the obj and the position to save to.
 * @param set_val The setter function to set the
 *   position.
 */
DigitalMeterWidget *
_digital_meter_widget_new_for_position (
  void * obj,
  void (*on_drag_begin)(void *),
  void (*get_val)(void *, Position *),
  void (*set_val)(void *, Position *),
  void (*on_drag_end)(void *),
  const char * caption)
{
  DigitalMeterWidget * self =
    g_object_new (DIGITAL_METER_WIDGET_TYPE, NULL);

  self->obj = obj;
  self->on_drag_begin = on_drag_begin;
  self->getter = get_val;
  self->setter = set_val;
  self->on_drag_end = on_drag_end;
  self->caption = g_strdup (caption);
  self->type = DIGITAL_METER_TYPE_POSITION;
  init_dm (self);

  return self;
}

static void
digital_meter_widget_class_init (DigitalMeterWidgetClass * klass)
{
}

static void
digital_meter_widget_init (DigitalMeterWidget * self)
{
  /* make it able to notify */
  gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);
  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));

  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_start),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT(self), "button_press_event",
    G_CALLBACK (button_press_cb),  self);

  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);
}
