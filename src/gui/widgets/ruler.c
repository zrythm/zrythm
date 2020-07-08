/*
 * Copyright (C) 2018-2020 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "actions/actions.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_marker.h"
#include "gui/widgets/ruler_range.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  RulerWidget,
  ruler_widget,
  GTK_TYPE_DRAWING_AREA)

#define Y_SPACING 5
#define FONT "Monospace"
#define FONT_SIZE 14

     /* FIXME delete these, see ruler_marker.h */
#define START_MARKER_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_WIDTH 8
#define Q_HEIGHT 12
#define Q_WIDTH 7

#define TYPE(x) RULER_WIDGET_TYPE_##x

#define ACTION_IS(x) \
  (self->action == UI_OVERLAY_ACTION_##x)

int
ruler_widget_get_beat_interval (
  RulerWidget * self)
{
  int i;

  /* gather divisors of the number of beats per
   * bar */
#define beats_per_bar TRANSPORT->beats_per_bar
  int beat_divisors[16];
  int num_beat_divisors = 0;
  for (i = 1; i <= beats_per_bar; i++)
    {
      if (beats_per_bar % i == 0)
        beat_divisors[num_beat_divisors++] = i;
    }

  /* decide the raw interval to keep between beats */
  int _beat_interval =
    MAX (
      (int)
      (RW_PX_TO_HIDE_BEATS / self->px_per_beat),
      1);

  /* round the interval to the divisors */
  int beat_interval = -1;
  for (i = 0; i < num_beat_divisors; i++)
    {
      if (_beat_interval <= beat_divisors[i])
        {
          if (beat_divisors[i] != beats_per_bar)
            beat_interval = beat_divisors[i];
          break;
        }
    }

  return beat_interval;
}

int
ruler_widget_get_sixteenth_interval (
  RulerWidget * self)
{
  int i;

  /* gather divisors of the number of sixteenths per
   * beat */
#define sixteenths_per_beat \
  TRANSPORT->sixteenths_per_beat
  int divisors[16];
  int num_divisors = 0;
  for (i = 1; i <= sixteenths_per_beat; i++)
    {
      if (sixteenths_per_beat % i == 0)
        divisors[num_divisors++] = i;
    }

  /* decide the raw interval to keep between sixteenths */
  int _sixteenth_interval =
    MAX (
      (int)
      (RW_PX_TO_HIDE_BEATS /
      self->px_per_sixteenth), 1);

  /* round the interval to the divisors */
  int sixteenth_interval = -1;
  for (i = 0; i < num_divisors; i++)
    {
      if (_sixteenth_interval <= divisors[i])
        {
          if (divisors[i] != sixteenths_per_beat)
            sixteenth_interval = divisors[i];
          break;
        }
    }

  return sixteenth_interval;
}

/**
 * Draws the regions in the editor ruler.
 */
static void
draw_regions (
  RulerWidget *  self,
  GdkRectangle * rect)
{
  cairo_t * cr = self->cached_cr;

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  /* get a visible region - the clip editor
   * region is removed temporarily while moving
   * regions so this could be NULL */
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    return;

  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  Track * track =
    arranger_object_get_track (region_obj);

  int px_start, px_end;

  /* draw the main region */
  GdkRGBA color = track->color;
  ui_get_arranger_object_color (
    &color,
    MW_TIMELINE->hovered_object == region_obj,
    region_is_selected (region),
    false, arranger_object_get_muted (region_obj));
  gdk_cairo_set_source_rgba (cr, &color);
  px_start =
    ui_pos_to_px_editor (
      &region_obj->pos, 1);
  px_end =
    ui_pos_to_px_editor (
      &region_obj->end_pos, 1);
  cairo_rectangle (
    cr, px_start - rect->x, - rect->y,
    px_end - px_start, height / 4.0);
  cairo_fill (cr);

  /* draw its transient if copy-moving TODO */
  if (arranger_object_should_orig_be_visible (
        region_obj))
    {
      px_start =
        ui_pos_to_px_editor (
          &region_obj->pos, 1);
      px_end =
        ui_pos_to_px_editor (
          &region_obj->end_pos, 1);
      cairo_rectangle (
        cr, px_start - rect->x, - rect->y,
        px_end - px_start, height / 4.0);
      cairo_fill (cr);
    }

  /* draw the other regions */
  cairo_set_source_rgba (
    cr,
    track->color.red,
    track->color.green,
    track->color.blue,
    0.5);
  ZRegion * other_region;
  TrackLane * lane;
  for (int j = 0; j < track->num_lanes; j++)
    {
      lane = track->lanes[j];

      for (int i = 0; i < lane->num_regions; i++)
        {
          other_region = lane->regions[i];
          ArrangerObject * other_region_obj =
            (ArrangerObject *) other_region;
          if (!g_strcmp0 (region->name,
                         other_region->name))
            continue;

          px_start =
            ui_pos_to_px_editor (
              &other_region_obj->pos, 1);
          px_end =
            ui_pos_to_px_editor (
              &other_region_obj->end_pos, 1);
          cairo_rectangle (
            cr, px_start - rect->x, - rect->y,
            px_end - px_start, height / 4.0);
          cairo_fill (cr);
        }
    }
}

static void
get_loop_start_rect (
  RulerWidget *  self,
  GdkRectangle * rect)
{
  rect->x = 0;
  if (self->type == TYPE (EDITOR))
    {
      ZRegion * region =
        clip_editor_get_region (CLIP_EDITOR);
      if (region)
        {
          ArrangerObject * region_obj =
            (ArrangerObject *) region;
          double start_ticks =
            position_to_ticks (&region_obj->pos);
          double loop_start_ticks =
            position_to_ticks (
              &region_obj->loop_start_pos) +
            start_ticks;
          Position tmp;
          position_from_ticks (
            &tmp, loop_start_ticks);
          rect->x =
            ui_pos_to_px_editor (&tmp, 1);
        }
      else
        {
          rect->x = 0;
        }
    }
  else if (self->type == TYPE (TIMELINE))
    {
      rect->x =
        ui_pos_to_px_timeline (
          &TRANSPORT->loop_start_pos, 1);
    }
  rect->y = 0;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_loop_start (
  RulerWidget *  self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_loop_start_rect (self, &dr);

  if (dr.x >=
        rect->x - dr.width &&
      dr.x <=
        rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0, 0.9, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (
        cr, dr.x - rect->x, dr.y);
      cairo_line_to (
        cr, dr.x - rect->x, dr.y + dr.height);
      cairo_line_to (
        cr, (dr.x + dr.width) - rect->x, dr.y);
      cairo_fill (cr);
    }
}

static void
get_loop_end_rect (
  RulerWidget *  self,
  GdkRectangle * rect)
{
  rect->x = 0;
  if (self->type == TYPE (EDITOR))
    {
      ZRegion * region =
        clip_editor_get_region (CLIP_EDITOR);
      if (region)
        {
          ArrangerObject * region_obj =
            (ArrangerObject *) region;
          double start_ticks =
            position_to_ticks (
              &region_obj->pos);
          double loop_end_ticks =
            position_to_ticks (
              &region_obj->loop_end_pos) +
            start_ticks;
          Position tmp;
          position_from_ticks (
            &tmp, loop_end_ticks);
          rect->x =
            ui_pos_to_px_editor (
              &tmp, 1) - RW_RULER_MARKER_SIZE;
        }
      else
        {
          rect->x = 0;
        }
    }
  else if (self->type == TYPE (TIMELINE))
    {
      rect->x =
        ui_pos_to_px_timeline (
          &TRANSPORT->loop_end_pos, 1) -
        RW_RULER_MARKER_SIZE;
    }
  rect->y = 0;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_loop_end (
  RulerWidget *  self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_loop_end_rect (self, &dr);

  if (dr.x >=
        rect->x - dr.width &&
      dr.x <=
        rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0, 0.9, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x - rect->x, dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width) - rect->x, dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width) - rect->x,
        dr.y + dr.height);
      cairo_fill (cr);
    }
}

static void
get_clip_start_rect (
  RulerWidget *  self,
  GdkRectangle * rect)
{
  if (self->type == TYPE (EDITOR))
    {
      ZRegion * region =
        clip_editor_get_region (CLIP_EDITOR);
      if (region)
        {
          ArrangerObject * region_obj =
            (ArrangerObject *) region;
          double start_ticks =
            position_to_ticks (
              &region_obj->pos);
          double clip_start_ticks =
            position_to_ticks (
              &region_obj->clip_start_pos) +
            start_ticks;
          Position tmp;
          position_from_ticks (
            &tmp, clip_start_ticks);
          rect->x =
            ui_pos_to_px_editor (&tmp, 1);
        }
      else
        {
          rect->x = 0;
        }
      rect->y =
        ((gtk_widget_get_allocated_height (
          GTK_WIDGET (self)) -
            RW_RULER_MARKER_SIZE) -
         RW_CUE_MARKER_HEIGHT) - 1;
    }
  rect->width = RW_CUE_MARKER_WIDTH;
  rect->height = RW_CUE_MARKER_HEIGHT;
}

/**
 * Draws the cue point (or clip start if this is
 * the editor ruler.
 */
static void
draw_cue_point (
  RulerWidget *  self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_clip_start_rect (self, &dr);

  if (self->type == TYPE (TIMELINE))
    {
      dr.x =
        ui_pos_to_px_timeline (
          &TRANSPORT->cue_pos,
          1);
      dr.y =
        RW_RULER_MARKER_SIZE;
    }

  if (dr.x >=
        rect->x - dr.width &&
      dr.x <=
        rect->x + rect->width)
    {
      if (self->type == TYPE (EDITOR))
        {
          cairo_set_source_rgb (cr, 0.2, 0.6, 0.9);
        }
      else if (self->type == TYPE (TIMELINE))
        {
          cairo_set_source_rgb (cr, 0, 0.6, 0.9);
        }
      cairo_set_line_width (cr, 2);
      cairo_move_to (
        cr, dr.x - rect->x, dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width) - rect->x,
        dr.y + dr.height / 2);
      cairo_line_to (
        cr, dr.x - rect->x, dr.y + dr.height);
      cairo_fill (cr);
    }
}

/**
 * Returns the playhead's x coordinate in absolute
 * coordinates.
 */
static int
get_playhead_px (
  RulerWidget * self)
{
  if (self->type == TYPE (EDITOR))
    {
      return
        ui_pos_to_px_editor (PLAYHEAD, 1);
    }
  else if (self->type == TYPE (TIMELINE))
    {
      return
        ui_pos_to_px_timeline (PLAYHEAD, 1);
    }
  g_return_val_if_reached (-1);
}

static void
draw_playhead (
  RulerWidget *  self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  int px = get_playhead_px (self);

  if (px >=
        rect->x - RW_PLAYHEAD_TRIANGLE_WIDTH / 2 &&
      px <=
        (rect->x + rect->width) -
        RW_PLAYHEAD_TRIANGLE_WIDTH / 2)
    {
      self->last_playhead_px = px;

      /* draw rect */
      GdkRectangle dr = { 0, 0, 0, 0 };
      dr.x =
        px -
        (RW_PLAYHEAD_TRIANGLE_WIDTH / 2);
      dr.y =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self)) -
          RW_PLAYHEAD_TRIANGLE_HEIGHT;
      dr.width = RW_PLAYHEAD_TRIANGLE_WIDTH;
      dr.height = RW_PLAYHEAD_TRIANGLE_HEIGHT;

      cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x - rect->x, dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width / 2) - rect->x,
        dr.y + dr.height);
      cairo_line_to (
        cr, (dr.x + dr.width) - rect->x, dr.y);
      cairo_fill (cr);
    }
}

static gboolean
ruler_draw_cb (
  GtkWidget *   widget,
  cairo_t *     cr,
  RulerWidget * self)
{
  /* engine is run only set after everything is set
   * up so this is a good way to decide if we
   * should  draw or not */
  if (!PROJECT || !AUDIO_ENGINE ||
      !g_atomic_int_get (&AUDIO_ENGINE->run) ||
      self->px_per_bar < 2.0)
    {
      return FALSE;
    }

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (
    cr, &rect);

  if (self->redraw ||
      !gdk_rectangle_equal (
         &rect, &self->last_rect))
    {
      self->last_rect = rect;

      GtkStyleContext *context =
        gtk_widget_get_style_context (
          GTK_WIDGET (self));

      z_cairo_reset_caches (
        &self->cached_cr,
        &self->cached_surface, rect.width,
        rect.height, cr);

      cairo_t * cr_to_use = self->cached_cr;

      /* ----- ruler background ------- */

      int height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));

      gtk_render_background (
        context, cr_to_use,
        0, 0, rect.width, rect.height);

      /* if timeline, draw loop background */
      /* FIXME use rect */
      double start_px = 0, end_px = 0;
      if (self->type == TYPE (TIMELINE))
        {
          start_px =
            ui_pos_to_px_timeline (
              &TRANSPORT->loop_start_pos, 1);
          end_px =
            ui_pos_to_px_timeline (
              &TRANSPORT->loop_end_pos, 1);
        }
      else if (self->type == TYPE (EDITOR))
        {
          start_px =
            ui_pos_to_px_editor (
              &TRANSPORT->loop_start_pos, 1);
          end_px =
            ui_pos_to_px_editor (
              &TRANSPORT->loop_end_pos, 1);
        }

      if (TRANSPORT->loop)
        cairo_set_source_rgba (
          cr_to_use, 0, 0.9, 0.7, 0.25);
      else
        cairo_set_source_rgba (
          cr_to_use, 0.5, 0.5, 0.5, 0.25);
      cairo_set_line_width (cr_to_use, 2);

      /* if transport loop start is within the
       * screen */
      if (start_px + 2.0 > rect.x &&
          start_px <= rect.x + rect.width)
        {
          /* draw the loop start line */
          double x =
            (start_px - rect.x);
          cairo_move_to (
            cr_to_use, x, 0);
          cairo_line_to (
            cr_to_use, x, rect.height);
          cairo_stroke (cr_to_use);
        }
      /* if transport loop end is within the
       * screen */
      if (end_px + 2.0 > rect.x &&
          end_px <= rect.x + rect.width)
        {
          /* draw the loop end line */
          double x =
            (end_px - rect.x);
          cairo_move_to (
            cr_to_use, x, 0);
          cairo_line_to (
            cr_to_use, x, rect.height);
          cairo_stroke (cr_to_use);
        }

      /* create gradient for loop area */
      cairo_pattern_t * pat =
        cairo_pattern_create_linear (
          0.0, 0.0, 0.0, (height * 3) / 4);
      if (TRANSPORT->loop)
        {
          cairo_pattern_add_color_stop_rgba (
            pat, 1, 0, 0.9, 0.7, 0.1);
          cairo_pattern_add_color_stop_rgba (
            pat, 0, 0, 0.9, 0.7, 0.2);
        }
      else
        {
          cairo_pattern_add_color_stop_rgba (
            pat, 1, 0.5, 0.5, 0.5, 0.1);
          cairo_pattern_add_color_stop_rgba (
            pat, 0, 0.5, 0.5, 0.5, 0.2);
        }

      double loop_start_local_x =
        MAX (0, start_px - rect.x);
      cairo_rectangle (
        cr_to_use,
        loop_start_local_x, 0,
        end_px - MAX (rect.x, start_px),
        rect.height);
      cairo_set_source (cr_to_use, pat);
      cairo_fill (cr_to_use);
      cairo_pattern_destroy (pat);

      PangoLayout * layout =
        pango_cairo_create_layout (
          cr_to_use);
      PangoFontDescription *desc =
        pango_font_description_from_string (
          "Monospace 11");
      pango_layout_set_font_description (
        layout, desc);
      pango_font_description_free (desc);

      /* draw lines */
      int i = 0;
      double curr_px;
      char text[40];
      int textw, texth;

      /* get sixteenth interval */
      int sixteenth_interval =
        ruler_widget_get_sixteenth_interval (self);

      /* get beat interval */
      int beat_interval =
        ruler_widget_get_beat_interval (self);

      /* get the interval for bars */
      int bar_interval =
        (int)
        MAX ((RW_PX_TO_HIDE_BEATS) /
             (double) self->px_per_bar, 1.0);

      /* draw bars */
      i = - bar_interval;
      while (
        (curr_px =
           self->px_per_bar * (i += bar_interval) +
             SPACE_BEFORE_START) <
         rect.x + rect.width + 20.0)
        {
          if (curr_px + 20.0 < rect.x)
            continue;

          cairo_set_source_rgb (
            cr_to_use, 1, 1, 1);
          cairo_set_line_width (
            cr_to_use, 1);
          double x = curr_px - rect.x;
          cairo_move_to (
            cr_to_use, x, 0);
          cairo_line_to (
            cr_to_use, x, height / 3);
          cairo_stroke (cr_to_use);
          cairo_set_source_rgb (
            cr_to_use, 0.8, 0.8, 0.8);
          sprintf (text, "%d", i + 1);
          pango_layout_set_markup (layout, text, -1);
          pango_layout_get_pixel_size (
            layout, &textw, &texth);
          cairo_move_to (
            cr_to_use,
            x - textw / 2, height / 3 + 2);
          pango_cairo_update_layout (
            cr_to_use, layout);
          pango_cairo_show_layout (
            cr_to_use, layout);
        }
      /* draw beats */
      desc =
        pango_font_description_from_string (
          "Monospace 6");
      pango_layout_set_font_description (
        layout, desc);
      pango_font_description_free (desc);
      i = 0;
      if (beat_interval > 0)
        {
          while ((curr_px =
                  self->px_per_beat *
                    (i += beat_interval) +
                  SPACE_BEFORE_START) <
                 rect.x + rect.width)
            {
              if (curr_px < rect.x)
                continue;

              cairo_set_source_rgb (
                cr_to_use, 0.7, 0.7, 0.7);
              cairo_set_line_width (
                cr_to_use, 0.5);
              double x = curr_px - rect.x;
              cairo_move_to (
                cr_to_use, x, 0);
              cairo_line_to (
                cr_to_use, x, height / 4);
              cairo_stroke (cr_to_use);
              if ((self->px_per_beat >
                     RW_PX_TO_HIDE_BEATS * 2) &&
                  i % beats_per_bar != 0)
                {
                  cairo_set_source_rgb (
                    cr_to_use,
                    0.5, 0.5, 0.5);
                  sprintf (
                    text, "%d.%d",
                    i / beats_per_bar + 1,
                    i % beats_per_bar + 1);
                  pango_layout_set_markup (
                    layout, text, -1);
                  pango_layout_get_pixel_size (
                    layout, &textw, &texth);
                  cairo_move_to (
                    cr_to_use, x - textw / 2,
                    height / 4 + 2);
                  pango_cairo_update_layout (
                    cr_to_use, layout);
                  pango_cairo_show_layout (
                    cr_to_use, layout);
                }
            }
        }
      /* draw sixteenths */
      i = 0;
      if (sixteenth_interval > 0)
        {
          while ((curr_px =
                  self->px_per_sixteenth *
                    (i += sixteenth_interval) +
                  SPACE_BEFORE_START) <
                 rect.x + rect.width)
            {
              if (curr_px < rect.x)
                continue;

              cairo_set_source_rgb (
                cr_to_use, 0.6, 0.6, 0.6);
              cairo_set_line_width (
                cr_to_use, 0.3);
              double x = curr_px - rect.x;
              cairo_move_to (
                cr_to_use, x, 0);
              cairo_line_to (
                cr_to_use, x, height / 6);
              cairo_stroke (cr_to_use);

              if ((self->px_per_sixteenth >
                     RW_PX_TO_HIDE_BEATS * 2) &&
                  i % sixteenths_per_beat != 0)
                {
                  cairo_set_source_rgb (
                    cr_to_use, 0.5, 0.5, 0.5);
                  sprintf (
                    text, "%d.%d.%d",
                    i / TRANSPORT->
                      sixteenths_per_bar + 1,
                    ((i / sixteenths_per_beat) %
                      beats_per_bar) + 1,
                    i % sixteenths_per_beat + 1);
                  pango_layout_set_markup (
                    layout, text, -1);
                  pango_layout_get_pixel_size (
                    layout, &textw, &texth);
                  cairo_move_to (
                    cr_to_use, x - textw / 2,
                    height / 4 + 2);
                  pango_cairo_update_layout (
                    cr_to_use, layout);
                  pango_cairo_show_layout (
                    cr_to_use, layout);
                }
            }
        }
      g_object_unref (layout);

      /* ----- draw range --------- */

      if (TRANSPORT->has_range)
        {
          int range1_first =
            position_is_before_or_equal (
              &TRANSPORT->range_1,
              &TRANSPORT->range_2);

          GdkRectangle dr;
          if (range1_first)
            {
              dr.x =
                ui_pos_to_px_timeline (
                  &TRANSPORT->range_1, true);
              dr.width =
                ui_pos_to_px_timeline (
                  &TRANSPORT->range_2, true) - dr.x;
            }
          else
            {
              dr.x =
                ui_pos_to_px_timeline (
                  &TRANSPORT->range_2, true);
              dr.width =
                ui_pos_to_px_timeline (
                  &TRANSPORT->range_1, true) - dr.x;
            }
          dr.y = 0;
          dr.height =
            gtk_widget_get_allocated_height (
              GTK_WIDGET (self)) /
            RW_RANGE_HEIGHT_DIVISOR;

          /* fill */
          cairo_set_source_rgba (
            cr_to_use, 1, 1, 1, 0.27);
          cairo_rectangle (
            cr_to_use, dr.x, dr.y,
            dr.width, dr.height);
          cairo_fill (cr_to_use);

          /* draw edges */
          cairo_set_source_rgba (
            cr_to_use, 1, 1, 1, 0.7);
          cairo_rectangle (
            cr_to_use, dr.x, dr.y, 2, dr.height);
          cairo_fill (cr_to_use);
          cairo_rectangle (
            cr_to_use, dr.x + dr.width, dr.y, 2,
            dr.height - dr.y);
          cairo_fill (cr_to_use);
        }

      /* ----- draw regions --------- */

      if (self->type == TYPE (EDITOR))
        {
          draw_regions (self, &rect);
        }

      /* ------ draw markers ------- */

      draw_cue_point (
        self, cr_to_use, &rect);
      draw_loop_start (
        self, cr_to_use, &rect);
      draw_loop_end (
        self, cr_to_use, &rect);

      /* --------- draw playhead ---------- */

      draw_playhead (self, cr_to_use, &rect);

      self->redraw = 0;
    }

  cairo_set_source_surface (
    cr, self->cached_surface, rect.x, rect.y);
  cairo_paint (cr);

 return FALSE;
}

#undef beats_per_bar
#undef sixteenths_per_beat

static int
is_loop_start_hit (
  RulerWidget * self,
  double        x,
  double        y)
{
  GdkRectangle rect;
  get_loop_start_rect (self, &rect);

  return
    x >= rect.x && x <= rect.x + rect.width &&
    y >= rect.y && y <= rect.y + rect.height;
}

static int
is_loop_end_hit (
  RulerWidget * self,
  double        x,
  double        y)
{
  GdkRectangle rect;
  get_loop_end_rect (self, &rect);

  return
    x >= rect.x && x <= rect.x + rect.width &&
    y >= rect.y && y <= rect.y + rect.height;
}

static bool
is_clip_start_hit (
  RulerWidget * self,
  double        x,
  double        y)
{
  if (self->type == TYPE (EDITOR))
    {
      GdkRectangle rect;
      get_clip_start_rect (self, &rect);

      return
        x >= rect.x && x <= rect.x + rect.width &&
        y >= rect.y && y <= rect.y + rect.height;
    }
  else
    return false;
}

static void
get_range_rect (
  RulerWidget *  self,
  RulerWidgetRangeType type,
  GdkRectangle * rect)
{
  g_return_if_fail (self->type == TYPE (TIMELINE));

  Position tmp;
  transport_get_range_pos (
    TRANSPORT,
    type == RW_RANGE_END ? false : true,
    &tmp);
  rect->x = ui_pos_to_px_timeline (&tmp, true);
  if (type == RW_RANGE_END)
    {
      rect->x -= RW_CUE_MARKER_WIDTH;
    }
  rect->y = 0;
  if (type == RW_RANGE_FULL)
    {
      transport_get_range_pos (
        TRANSPORT, false, &tmp);
      double px =
        ui_pos_to_px_timeline (&tmp, true);
      rect->width = (int) px - rect->x;
    }
  else
    {
      rect->width = RW_CUE_MARKER_WIDTH;
    }
  rect->height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self)) /
      RW_RANGE_HEIGHT_DIVISOR;
}

bool
ruler_widget_is_range_hit (
  RulerWidget *        self,
  RulerWidgetRangeType type,
  double               x,
  double               y)
{
  if (self->type == TYPE (TIMELINE))
    {
      GdkRectangle rect;
      memset (&rect, 0, sizeof (GdkRectangle));
      get_range_rect (self, type, &rect);

      return
        x >= rect.x && x <= rect.x + rect.width &&
        y >= rect.y && y <= rect.y + rect.height;
    }
  else
    {
      return false;
    }
}

static gboolean
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  RulerWidget *         self)
{
  GdkModifierType state_mask;
  ui_get_modifier_type_from_gesture (
    GTK_GESTURE_SINGLE (gesture),
    &state_mask);
  if (state_mask & GDK_SHIFT_MASK)
    self->shift_held = 1;

  if (n_press == 2)
    {
      if (self->type == TYPE (TIMELINE))
        {
          Position pos;
          ui_px_to_pos_timeline (
            x, &pos, 1);
          if (!self->shift_held)
            position_snap_simple (
              &pos, SNAP_GRID_TIMELINE);
          position_set_to_pos (&TRANSPORT->cue_pos,
                               &pos);
        }
      if (self->type == TYPE (EDITOR))
        {
        }
    }

  return FALSE;
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  RulerWidget *    self)
{
  self->start_x = start_x;
  self->start_y = start_y;

  if (self->type == TYPE (TIMELINE))
    {
      self->range1_first =
        position_is_before_or_equal (
          &TRANSPORT->range_1,
          &TRANSPORT->range_2);
    }

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  int loop_start_hit =
    is_loop_start_hit (self, start_x, start_y);
  int loop_end_hit =
    is_loop_end_hit (self, start_x, start_y);
  int clip_start_hit =
    is_clip_start_hit (self, start_x, start_y);

  /* if one of the markers hit */
  if (loop_start_hit)
    {
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target =
        RW_TARGET_LOOP_START;
    }
  else if (loop_end_hit)
    {
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target =
        RW_TARGET_LOOP_END;
    }
  else if (clip_start_hit)
    {
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target =
        RW_TARGET_CLIP_START;
    }
  else
    {
      if (self->type == TYPE (TIMELINE))
        timeline_ruler_on_drag_begin_no_marker_hit (
          self, start_x, start_y, height);
      else if (self->type == TYPE (EDITOR))
        editor_ruler_on_drag_begin_no_marker_hit (
          self, start_x, start_y);
    }

  self->last_offset_x = 0;
  self->dragging = 1;
}

static void
drag_end (GtkGestureDrag *gesture,
          gdouble         offset_x,
          gdouble         offset_y,
          RulerWidget *   self)
{
  self->start_x = 0;
  self->start_y = 0;
  self->shift_held = 0;
  self->dragging = 0;

  if (self->type == TYPE (TIMELINE))
    timeline_ruler_on_drag_end (self);
  else if (self->type == TYPE (EDITOR))
    editor_ruler_on_drag_end (self);

  self->action = UI_OVERLAY_ACTION_NONE;
}

static gboolean
on_grab_broken (
  GtkWidget *widget,
  GdkEvent  *event,
  gpointer   user_data)
{
  g_warning ("ruler grab broken");
  return FALSE;
}

static void
on_motion (
  GtkDrawingArea * da,
  GdkEventMotion *event,
  RulerWidget *    self)
{
  /* drag-update didn't work so do the drag-update
   * stuff here */
  if (self->dragging)
    {
      GdkModifierType state_mask =
        event->state;

      if (state_mask & GDK_SHIFT_MASK)
        self->shift_held = 1;
      else
        self->shift_held = 0;

      if (ACTION_IS (STARTING_MOVING))
        {
          self->action = UI_OVERLAY_ACTION_MOVING;
        }

      if (self->type == TYPE (TIMELINE))
        {
          timeline_ruler_on_drag_update (
            self, event->x - self->start_x,
            event->y - self->start_y);
        }
      else if (self->type == TYPE (EDITOR))
        {
          editor_ruler_on_drag_update (
            self, event->x - self->start_x,
            event->y - self->start_y);
        }

      self->last_offset_x =
        event->x - self->start_x;

      return;
    }

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  if (event->type == GDK_MOTION_NOTIFY)
    {
      bool loop_start_hit =
        is_loop_start_hit (
          self, event->x, event->y);
      bool loop_end_hit =
        is_loop_end_hit (
          self, event->x, event->y);
      bool clip_start_hit =
        is_clip_start_hit (
          self, event->x, event->y);
      bool range_start_hit =
        ruler_widget_is_range_hit (
          self, RW_RANGE_START,
          event->x, event->y);
      bool range_end_hit =
        ruler_widget_is_range_hit (
          self, RW_RANGE_END, event->x, event->y);

      if (loop_start_hit || clip_start_hit ||
          range_start_hit)
        {
          ui_set_cursor_from_name (
            GTK_WIDGET (self), "w-resize");
        }
      else if (loop_end_hit || range_end_hit)
        {
          ui_set_cursor_from_name (
            GTK_WIDGET (self), "e-resize");
        }
      /* if lower 3/4ths */
      else if (event->y > (height * 1) / 4)
        {
          /* set cursor to normal */
          ui_set_cursor_from_name (
            GTK_WIDGET (self), "default");
        }
      else /* upper 1/4th */
        {
          if (ruler_widget_is_range_hit (
                self, RW_RANGE_FULL, event->x,
                event->y))
            {
              /* set cursor to movable */
              ui_set_hand_cursor (self);
            }
          else
            {
              /* set cursor to range selection */
              ui_set_cursor_from_name (
                GTK_WIDGET (self), "text");
            }
        }
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      ui_set_cursor_from_name (
        GTK_WIDGET (self), "default");
    }
}

static GtkScrolledWindow *
get_scrolled_window (
  RulerWidget * self)
{
  switch (self->type)
    {
    case TYPE (TIMELINE):
      return MW_TIMELINE_PANEL->ruler_scroll;
    case TYPE (EDITOR):
      return MW_CLIP_EDITOR_INNER->ruler_scroll;
    }

  return NULL;
}

/**
 * Returns the current rectangle to draw in.
 *
 * @param rect The rectangle to fill in.
 */
static void
get_current_rect (
  RulerWidget * self,
  GdkRectangle *   rect)
{
  GtkScrolledWindow * scroll =
    get_scrolled_window (self);
  GtkAdjustment * xadj =
    gtk_scrolled_window_get_hadjustment (
      scroll);
  rect->x =
    (int) gtk_adjustment_get_value (xadj);
  GtkAdjustment * yadj =
    gtk_scrolled_window_get_vadjustment (scroll);
  rect->y =
    (int) gtk_adjustment_get_value (yadj);
  rect->height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (scroll));
  rect->width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (scroll));
}

/**
 * Queues a redraw of the whole visible ruler.
 */
void
ruler_widget_redraw_whole (
  RulerWidget * self)
{
  GdkRectangle rect;
  get_current_rect (self, &rect);

  /* redraw visible area */
  self->redraw = 1;
  gtk_widget_queue_draw_area (
    GTK_WIDGET (self), rect.x, rect.y,
    rect.width, rect.height);
}

/**
 * Only redraws the playhead part.
 */
void
ruler_widget_redraw_playhead (
  RulerWidget * self)
{
  GdkRectangle rect;
  get_current_rect (self, &rect);

  int playhead_x = get_playhead_px (self);
  int min_x =
    MIN (self->last_playhead_px, playhead_x);
  min_x =
    MAX (
      min_x - (RW_PLAYHEAD_TRIANGLE_WIDTH + 40),
      rect.x);
  int max_x =
    MAX (self->last_playhead_px, playhead_x);
  max_x =
    MIN (
      max_x + RW_PLAYHEAD_TRIANGLE_WIDTH,
      rect.x + rect.width);

  /* skip if playhead is not in the visible
   * rectangle */
  int width = max_x - min_x;
  if (width < 0)
    {
      g_message (
        "playhead not currently visible in ruler, "
        "skipping redraw");
      return;
    }

  gtk_widget_queue_draw_area (
    GTK_WIDGET (self), min_x, rect.y,
    width, rect.height);
}

void
ruler_widget_refresh (RulerWidget * self)
{
  /*adjust for zoom level*/
  self->px_per_tick =
    DEFAULT_PX_PER_TICK * self->zoom_level;
  self->px_per_sixteenth =
    self->px_per_tick * TICKS_PER_SIXTEENTH_NOTE;
  self->px_per_beat =
    self->px_per_tick * TRANSPORT->ticks_per_beat;
  self->px_per_bar =
    self->px_per_beat * TRANSPORT->beats_per_bar;

  Position pos;
  position_set_to_bar (
    &pos, TRANSPORT->total_bars + 1);
  self->total_px =
    self->px_per_tick *
    (double) position_to_ticks (&pos);

  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    (int) self->total_px,
    -1);

  if (self->type == TYPE (TIMELINE))
    {
      /*gtk_widget_set_visible (*/
        /*GTK_WIDGET (timeline_ruler->range),*/
        /*PROJECT->has_range);*/
    }

  gtk_widget_queue_allocate (
    GTK_WIDGET (self));
  EVENTS_PUSH (
    ET_RULER_SIZE_CHANGED, self);

  ruler_widget_redraw_whole (self);
}

/**
 * FIXME move to somewhere else, maybe project.
 * Sets zoom level and disables/enables buttons
 * accordingly.
 *
 * Returns if the zoom level was set or not.
 */
int
ruler_widget_set_zoom_level (
  RulerWidget * self,
  double        zoom_level)
{
  if (zoom_level > MAX_ZOOM_LEVEL)
    {
      action_disable_window_action ("zoom-in");
    }
  else
    {
      action_enable_window_action ("zoom-in");
    }
  if (zoom_level < MIN_ZOOM_LEVEL)
    {
      action_disable_window_action ("zoom-out");
    }
  else
    {
      action_enable_window_action ("zoom-out");
    }

  int update = zoom_level >= MIN_ZOOM_LEVEL &&
    zoom_level <= MAX_ZOOM_LEVEL;

  if (update)
    {
      self->zoom_level = zoom_level;
      ruler_widget_refresh (self);
      return 1;
    }
  else
    {
      return 0;
    }
}

static void
ruler_widget_init (RulerWidget * self)
{
  self->zoom_level = RW_DEFAULT_ZOOM_LEVEL;

  /* make the widget able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  self->drag = GTK_GESTURE_DRAG (
    gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
    gtk_gesture_multi_press_new (
      GTK_WIDGET (self)));

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (ruler_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  /*g_signal_connect (*/
    /*G_OBJECT(self->drag), "drag-update",*/
    /*G_CALLBACK (drag_update),  self);*/
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (self), "grab-broken-event",
    G_CALLBACK (on_grab_broken), self);
  g_signal_connect (
    G_OBJECT (self->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
}

static void
ruler_widget_class_init (RulerWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "ruler");
}
