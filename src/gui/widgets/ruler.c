/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/timeline.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_notebook.h"
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
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  RulerWidget, ruler_widget,
  GTK_TYPE_WIDGET)

#define Y_SPACING 5

     /* FIXME delete these, see ruler_marker.h */
#define START_MARKER_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_WIDTH 8
#define Q_HEIGHT 12
#define Q_WIDTH 7

#define TYPE(x) RULER_WIDGET_TYPE_##x

#define ACTION_IS(x) \
  (self->action == UI_OVERLAY_ACTION_##x)

double
ruler_widget_get_zoom_level (
  RulerWidget * self)
{
  if (self->type == RULER_WIDGET_TYPE_TIMELINE)
    {
      return
        PRJ_TIMELINE->editor_settings.hzoom_level;
    }
  else if (self->type ==
             RULER_WIDGET_TYPE_EDITOR)
    {
      ArrangerWidget * arr =
        clip_editor_inner_widget_get_visible_arranger (
          MW_CLIP_EDITOR_INNER);
      EditorSettings * settings =
        arranger_widget_get_editor_settings (arr);
      g_return_val_if_fail (settings, 1.f);

      return settings->hzoom_level;
    }

  g_return_val_if_reached (1.f);
}

/**
 * Returns the beat interval for drawing vertical
 * lines.
 */
int
ruler_widget_get_beat_interval (
  RulerWidget * self)
{
  int i;

  /* gather divisors of the number of beats per
   * bar */
  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
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

/**
 * Returns the sixteenth interval for drawing
 * vertical lines.
 */
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
 * Returns the 10 sec interval.
 */
int
ruler_widget_get_10sec_interval (
  RulerWidget * self)
{
  int i;

  /* gather divisors of the number of beats per
   * bar */
#define ten_secs_per_min 6
  int ten_sec_divisors[36];
  int num_ten_sec_divisors = 0;
  for (i = 1; i <= ten_secs_per_min; i++)
    {
      if (ten_secs_per_min % i == 0)
        ten_sec_divisors[num_ten_sec_divisors++] = i;
    }

  /* decide the raw interval to keep between
   * 10 secs */
  int _10sec_interval =
    MAX (
      (int)
      (RW_PX_TO_HIDE_BEATS / self->px_per_10sec),
      1);

  /* round the interval to the divisors */
  int ten_sec_interval = -1;
  for (i = 0; i < num_ten_sec_divisors; i++)
    {
      if (_10sec_interval <= ten_sec_divisors[i])
        {
          if (ten_sec_divisors[i] != ten_secs_per_min)
            ten_sec_interval = ten_sec_divisors[i];
          break;
        }
    }

  return ten_sec_interval;
}

/**
 * Returns the sec interval.
 */
int
ruler_widget_get_sec_interval (
  RulerWidget * self)
{
  int i;

  /* gather divisors of the number of sixteenths per
   * beat */
#define secs_per_10_sec 10
  int divisors[16];
  int num_divisors = 0;
  for (i = 1; i <= secs_per_10_sec; i++)
    {
      if (secs_per_10_sec % i == 0)
        divisors[num_divisors++] = i;
    }

  /* decide the raw interval to keep between secs */
  int _sec_interval =
    MAX (
      (int)
      (RW_PX_TO_HIDE_BEATS /
      self->px_per_sec), 1);

  /* round the interval to the divisors */
  int sec_interval = -1;
  for (i = 0; i < num_divisors; i++)
    {
      if (_sec_interval <= divisors[i])
        {
          if (divisors[i] != secs_per_10_sec)
            sec_interval = divisors[i];
          break;
        }
    }

  return sec_interval;
}

/**
 * Draws a region other than the editor one.
 */
static void
draw_other_region (
  RulerWidget *  self,
  GtkSnapshot *  snapshot,
  ZRegion *      region)
{
  /*g_debug (
   * "drawing other region %s", region->name);*/

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  Track * track = arranger_object_get_track (r_obj);
  GdkRGBA color = track->color;
  color.alpha = 0.5;

  int px_start, px_end;
  px_start =
    ui_pos_to_px_editor (&r_obj->pos, true);
  px_end =
    ui_pos_to_px_editor (&r_obj->end_pos, true);
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      px_start, 0,
      px_end - px_start, height / 4.f));
}

/**
 * Draws the regions in the editor ruler.
 */
static void
draw_regions (
  RulerWidget *  self,
  GtkSnapshot *  snapshot,
  GdkRectangle * rect)
{
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
  px_start =
    ui_pos_to_px_editor (
      &region_obj->pos, 1);
  px_end =
    ui_pos_to_px_editor (
      &region_obj->end_pos, 1);
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      px_start, 0,
      px_end - px_start, height / 4.f));

  /* draw its transient if copy-moving TODO */
  if (arranger_object_should_orig_be_visible (
        region_obj, NULL))
    {
      px_start =
        ui_pos_to_px_editor (
          &region_obj->pos, 1);
      px_end =
        ui_pos_to_px_editor (
          &region_obj->end_pos, 1);
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          px_start, 0,
          px_end - px_start, height / 4.f));
    }

  /* draw the other regions */
  ZRegion * other_regions[1000];
  int num_other_regions =
    track_get_regions_in_range (
      track, NULL, NULL, other_regions);
  for (int i = 0; i < num_other_regions; i++)
    {
      ZRegion * other_region = other_regions[i];
      if (region_identifier_is_equal (
            &region->id, &other_region->id) ||
          region->id.type != other_region->id.type)
        {
          continue;
        }

      draw_other_region (
        self, snapshot, other_region);
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
  GtkSnapshot *  snapshot,
  GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_loop_start_rect (self, &dr);

  cairo_t * cr =
    gtk_snapshot_append_cairo (
      snapshot,
      &GRAPHENE_RECT_INIT (
        dr.x, dr.y, dr.width, dr.height));

  if (dr.x >=
        rect->x - dr.width &&
      dr.x <=
        rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0, 0.9, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (
        cr, dr.x, dr.y);
      cairo_line_to (
        cr, dr.x, dr.y + dr.height);
      cairo_line_to (
        cr, (dr.x + dr.width), dr.y);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
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
  GtkSnapshot *  snapshot,
  GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_loop_end_rect (self, &dr);

  cairo_t * cr =
    gtk_snapshot_append_cairo (
      snapshot,
      &GRAPHENE_RECT_INIT (
        dr.x, dr.y, dr.width, dr.height));

  if (dr.x >=
        rect->x - dr.width &&
      dr.x <=
        rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0, 0.9, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width), dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width),
        dr.y + dr.height);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_punch_in_rect (
  RulerWidget *  self,
  GdkRectangle * rect)
{
  rect->x =
    ui_pos_to_px_timeline (
      &TRANSPORT->punch_in_pos, 1);
  rect->y = RW_RULER_MARKER_SIZE;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_punch_in (
  RulerWidget *  self,
  GtkSnapshot *  snapshot,
  GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_punch_in_rect (self, &dr);

  cairo_t * cr =
    gtk_snapshot_append_cairo (
      snapshot,
      &GRAPHENE_RECT_INIT (
        dr.x, dr.y, dr.width, dr.height));

  if (dr.x >=
        rect->x - dr.width &&
      dr.x <=
        rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0.9, 0.1, 0.1);
      cairo_set_line_width (cr, 2);
      cairo_move_to (
        cr, dr.x, dr.y);
      cairo_line_to (
        cr, dr.x, dr.y + dr.height);
      cairo_line_to (
        cr, (dr.x + dr.width), dr.y);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_punch_out_rect (
  RulerWidget *  self,
  GdkRectangle * rect)
{
  rect->x =
    ui_pos_to_px_timeline (
      &TRANSPORT->punch_out_pos, 1) -
    RW_RULER_MARKER_SIZE;
  rect->y = RW_RULER_MARKER_SIZE;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_punch_out (
  RulerWidget *  self,
  GtkSnapshot *  snapshot,
  GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_punch_out_rect (self, &dr);

  cairo_t * cr =
    gtk_snapshot_append_cairo (
      snapshot,
      &GRAPHENE_RECT_INIT (
        dr.x, dr.y, dr.width, dr.height));

  if (dr.x >=
        rect->x - dr.width &&
      dr.x <=
        rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0.9, 0.1, 0.1);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width), dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width),
        dr.y + dr.height);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_clip_start_rect (
  RulerWidget *  self,
  GdkRectangle * rect)
{
  /* TODO these were added to fix unused
   * warnings - check again if valid */
  rect->x = 0;
  rect->y = 0;

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
  GtkSnapshot *  snapshot,
  GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_clip_start_rect (self, &dr);

  cairo_t * cr =
    gtk_snapshot_append_cairo (
      snapshot,
      &GRAPHENE_RECT_INIT (
        dr.x, dr.y, dr.width, dr.height));

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
        cr, dr.x, dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width),
        dr.y + dr.height / 2);
      cairo_line_to (
        cr, dr.x, dr.y + dr.height);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
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
  GtkSnapshot *  snapshot,
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

      cairo_t * cr =
        gtk_snapshot_append_cairo (
          snapshot,
          &GRAPHENE_RECT_INIT (
            dr.x, dr.y, dr.width, dr.height));

      cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (
        cr, (dr.x + dr.width / 2),
        dr.y + dr.height);
      cairo_line_to (
        cr, (dr.x + dr.width), dr.y);
      cairo_fill (cr);

      cairo_destroy (cr);
    }
}

/**
 * Draws the grid lines and labels.
 *
 * @param rect Clip rectangle.
 */
static void
draw_lines_and_labels (
  RulerWidget *  self,
  GtkSnapshot *  snapshot,
  GdkRectangle * rect)
{
  /* draw lines */
  int i = 0;
  double curr_px;
  char text[40];
  int textw, texth;

  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  GdkRGBA main_color = { 1, 1, 1, 1 };
  GdkRGBA secondary_color = { 0.7f, 0.7f, 0.7f, 0.4f };
  GdkRGBA tertiary_color = { 0.5f, 0.5f, 0.5f, 0.3f };
  GdkRGBA main_text_color = { 0.8f, 0.8f, 0.8f, 1 };
  GdkRGBA secondary_text_color = {
    0.5f, 0.5f, 0.5f, 1 };
  GdkRGBA tertiary_text_color =
    secondary_text_color;

  /* if time display */
  if (g_settings_get_enum (
        S_UI, "ruler-display") ==
          TRANSPORT_DISPLAY_TIME)
    {
      /* get sec interval */
      int sec_interval =
        ruler_widget_get_sec_interval (self);

      /* get 10 sec interval */
      int ten_sec_interval =
        ruler_widget_get_10sec_interval (self);

      /* get the interval for mins */
      int min_interval =
        (int)
        MAX ((RW_PX_TO_HIDE_BEATS) /
             (double) self->px_per_min, 1.0);

      /* draw mins */
      i = - min_interval;
      while (
        (curr_px =
           self->px_per_min * (i += min_interval) +
             SPACE_BEFORE_START) <
         rect->x + rect->width + 20.0)
        {
          if (curr_px + 20.0 < rect->x)
            continue;

          gtk_snapshot_append_color (
            snapshot, &main_color,
            &GRAPHENE_RECT_INIT (
              (float) curr_px, 0,
              1, height / 3.f));

          sprintf (text, "%d", i);
          pango_layout_set_text (
            self->layout_normal, text, -1);
          pango_layout_get_pixel_size (
            self->layout_normal, &textw, &texth);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              (float) curr_px + 1 - textw / 2.f,
              height / 3.f + 2));
          gtk_snapshot_append_layout (
            snapshot, self->layout_normal,
            &main_text_color);
          gtk_snapshot_restore (snapshot);
        }
      /* draw 10secs */
      i = 0;
      if (ten_sec_interval > 0)
        {
          while ((curr_px =
                  self->px_per_10sec *
                    (i += ten_sec_interval) +
                  SPACE_BEFORE_START) <
                 rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &secondary_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, 0,
                  1, height / 4.f));

              if ((self->px_per_10sec >
                     RW_PX_TO_HIDE_BEATS * 2) &&
                  i % ten_secs_per_min != 0)
                {
                  sprintf (
                    text, "%d:%02d",
                    i / ten_secs_per_min,
                    (i % ten_secs_per_min) * 10);
                  pango_layout_set_text (
                    self->layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->layout_small,
                    &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  gtk_snapshot_translate (
                    snapshot,
                    &GRAPHENE_POINT_INIT (
                      (float) curr_px + 1 - textw / 2.f,
                      height / 4.f + 2));
                  gtk_snapshot_append_layout (
                    snapshot, self->layout_small,
                    &secondary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
      /* draw secs */
      i = 0;
      if (sec_interval > 0)
        {
          while ((curr_px =
                  self->px_per_sec *
                    (i += sec_interval) +
                  SPACE_BEFORE_START) <
                 rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &tertiary_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, 0,
                  1, height / 6.f));

              if ((self->px_per_sec >
                     RW_PX_TO_HIDE_BEATS * 2) &&
                  i % secs_per_10_sec != 0)
                {
                  int secs_per_min = 60;
                  sprintf (
                    text, "%d:%02d",
                    i / secs_per_min,
                    ((i / secs_per_10_sec) %
                      ten_secs_per_min) * 10 +
                    i % secs_per_10_sec);
                  pango_layout_set_text (
                    self->layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->layout_small,
                    &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  gtk_snapshot_translate (
                    snapshot,
                    &GRAPHENE_POINT_INIT (
                      (float) curr_px + 1 - textw / 2.f,
                      height / 4.f + 2));
                  gtk_snapshot_append_layout (
                    snapshot, self->layout_small,
                    &tertiary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
    }
  else /* else if BBT display */
    {
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
         rect->x + rect->width + 20.0)
        {
          if (curr_px + 20.0 < rect->x)
            continue;

          gtk_snapshot_append_color (
            snapshot, &main_color,
            &GRAPHENE_RECT_INIT (
              (float) curr_px, 0,
              1, height / 3.f));

          sprintf (text, "%d", i + 1);
          pango_layout_set_text (
            self->layout_normal, text, -1);
          pango_layout_get_pixel_size (
            self->layout_normal, &textw, &texth);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              (float) curr_px + 1 - textw / 2.f,
              height / 3.f + 2));
          gtk_snapshot_append_layout (
            snapshot, self->layout_normal,
            &main_text_color);
          gtk_snapshot_restore (snapshot);
        }
      /* draw beats */
      i = 0;
      if (beat_interval > 0)
        {
          while ((curr_px =
                  self->px_per_beat *
                    (i += beat_interval) +
                  SPACE_BEFORE_START) <
                 rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &secondary_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, 0,
                  1, height / 4.f));

              if ((self->px_per_beat >
                     RW_PX_TO_HIDE_BEATS * 2) &&
                  i % beats_per_bar != 0)
                {
                  sprintf (
                    text, "%d.%d",
                    i / beats_per_bar + 1,
                    i % beats_per_bar + 1);
                  pango_layout_set_text (
                    self->layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->layout_small,
                    &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  gtk_snapshot_translate (
                    snapshot,
                    &GRAPHENE_POINT_INIT (
                      (float) curr_px + 1 - textw / 2.f,
                      height / 4.f + 2));
                  gtk_snapshot_append_layout (
                    snapshot, self->layout_small,
                    &secondary_text_color);
                  gtk_snapshot_restore (snapshot);
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
                 rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &tertiary_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, 0,
                  1, height / 6.f));

              if ((self->px_per_sixteenth >
                     RW_PX_TO_HIDE_BEATS * 2) &&
                  i % sixteenths_per_beat != 0)
                {
                  sprintf (
                    text, "%d.%d.%d",
                    i / TRANSPORT->
                      sixteenths_per_bar + 1,
                    ((i / sixteenths_per_beat) %
                      beats_per_bar) + 1,
                    i % sixteenths_per_beat + 1);
                  pango_layout_set_text (
                    self->layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->layout_small,
                    &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  gtk_snapshot_translate (
                    snapshot,
                    &GRAPHENE_POINT_INIT (
                      (float) curr_px + 1 - textw / 2.f,
                      height / 4.f + 2));
                  gtk_snapshot_append_layout (
                    snapshot, self->layout_small,
                    &tertiary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
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

static void
ruler_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  RulerWidget * self = Z_RULER_WIDGET (widget);
  GtkScrolledWindow * scroll =
    get_scrolled_window (self);
  graphene_rect_t visible_rect;
  z_gtk_scrolled_window_get_visible_rect (
    scroll, &visible_rect);
  GdkRectangle visible_rect_gdk;
  z_gtk_graphene_rect_t_to_gdk_rectangle (
    &visible_rect_gdk, &visible_rect);

  /* engine is run only set after everything is set
   * up so this is a good way to decide if we
   * should  draw or not */
  if (!PROJECT || !AUDIO_ENGINE ||
      !g_atomic_int_get (&AUDIO_ENGINE->run) ||
      self->px_per_bar < 2.0)
    {
      return;
    }

  GtkStyleContext * context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self));
  gtk_snapshot_render_background (
    snapshot, context,
    visible_rect.origin.x,
    visible_rect.origin.y,
    visible_rect.size.width,
    visible_rect.size.height);

  self->last_rect = visible_rect;

  /* ----- ruler background ------- */

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

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

  GdkRGBA color;
  if (TRANSPORT->loop)
    {
      color.red = 0;
      color.green = 0.9f;
      color.blue = 0.7f;
      color.alpha = 0.25f;
    }
  else
    {
      color.red = 0.5f;
      color.green = 0.5f;
      color.blue = 0.5f;
      color.alpha = 0.25f;
    }
  const int line_width = 2;

  /* if transport loop start is within the
   * screen */
  if (start_px + 2.0 > visible_rect_gdk.x &&
      start_px <= visible_rect_gdk.x + visible_rect_gdk.width)
    {
      /* draw the loop start line */
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          (float) start_px, 0,
          line_width, visible_rect.size.height));
    }
  /* if transport loop end is within the
   * screen */
  if (end_px + 2.0 > visible_rect_gdk.x &&
      end_px <= visible_rect_gdk.x + visible_rect_gdk.width)
    {
      /* draw the loop end line */
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          (float) end_px, 0,
          line_width, visible_rect.size.height));
    }

  /* create gradient for loop area */
  GskColorStop stop1, stop2;
  if (TRANSPORT->loop)
    {
      stop1.offset = 0;
      stop1.color.red = 0;
      stop1.color.green = 0.9f;
      stop1.color.blue = 0.7f;
      stop1.color.alpha = 0.2f;
      stop2.offset = 1;
      stop2.color.red = 0;
      stop2.color.green = 0.9f;
      stop2.color.blue = 0.7f;
      stop2.color.alpha = 0.1f;
    }
  else
    {
      stop1.offset = 0;
      stop1.color.red = 0.5f;
      stop1.color.green = 0.5f;
      stop1.color.blue = 0.5f;
      stop1.color.alpha = 0.2f;
      stop2.offset = 1;
      stop2.color.red = 0.5f;
      stop2.color.green = 0.5f;
      stop2.color.blue = 0.5f;
      stop2.color.alpha = 0.1f;
    }
  GskColorStop stops[] = { stop1, stop2 };

  gtk_snapshot_append_linear_gradient (
    snapshot,
    &GRAPHENE_RECT_INIT (
      (float) start_px, 0,
      (float) (end_px - start_px), height),
    &GRAPHENE_POINT_INIT (0, 0),
    &GRAPHENE_POINT_INIT (
      0, (float) (height * 3) / 4),
    stops, G_N_ELEMENTS (stops));

  draw_lines_and_labels (
    self, snapshot, &visible_rect_gdk);

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
      gtk_snapshot_append_color (
        snapshot,
        &Z_GDK_RGBA_INIT (1, 1, 1, 0.27f),
        &GRAPHENE_RECT_INIT (
          dr.x, dr.y, dr.width, dr.height));

      /* draw edges */
      gtk_snapshot_append_color (
        snapshot,
        &Z_GDK_RGBA_INIT (1, 1, 1, 0.7f),
        &GRAPHENE_RECT_INIT (
          dr.x, dr.y, 2, dr.height));
      gtk_snapshot_append_color (
        snapshot,
        &Z_GDK_RGBA_INIT (1, 1, 1, 0.7f),
        &GRAPHENE_RECT_INIT (
          dr.x + dr.width, dr.y,
          2, dr.height - dr.y));
    }

  /* ----- draw regions --------- */

  if (self->type == TYPE (EDITOR))
    {
      draw_regions (
        self, snapshot, &visible_rect_gdk);
    }

  /* ------ draw markers ------- */

  draw_cue_point (
    self, snapshot, &visible_rect_gdk);
  draw_loop_start (
    self, snapshot, &visible_rect_gdk);
  draw_loop_end (
    self, snapshot, &visible_rect_gdk);

  if (self->type == TYPE (TIMELINE) &&
      TRANSPORT->punch_mode)
    {
      draw_punch_in (
        self, snapshot, &visible_rect_gdk);
      draw_punch_out (
        self, snapshot, &visible_rect_gdk);
    }

  /* --------- draw playhead ---------- */

  draw_playhead (self, snapshot, &visible_rect_gdk);
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

static int
is_punch_in_hit (
  RulerWidget * self,
  double        x,
  double        y)
{
  GdkRectangle rect;
  get_punch_in_rect (self, &rect);

  return
    x >= rect.x && x <= rect.x + rect.width &&
    y >= rect.y && y <= rect.y + rect.height;
}

static int
is_punch_out_hit (
  RulerWidget * self,
  double        x,
  double        y)
{
  GdkRectangle rect;
  get_punch_out_rect (self, &rect);

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
      rect->width = (int) px;
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
  if (self->type == TYPE (TIMELINE) &&
      TRANSPORT->has_range)
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
on_click_pressed (
  GtkGestureClick *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  RulerWidget *         self)
{
  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (gesture));
  if (state & GDK_SHIFT_MASK)
    self->shift_held = 1;

  if (n_press == 2)
    {
      if (self->type == TYPE (TIMELINE))
        {
          Position pos;
          ui_px_to_pos_timeline (
            x, &pos, 1);
          if (!self->shift_held
              &&
              SNAP_GRID_ANY_SNAP (
                SNAP_GRID_TIMELINE))
            {
              position_snap (
                &pos, &pos, NULL, NULL,
                SNAP_GRID_TIMELINE);
            }
          position_set_to_pos (&TRANSPORT->cue_pos,
                               &pos);
        }
      if (self->type == TYPE (EDITOR))
        {
        }
    }

  return FALSE;
}

#if 0
static void
on_display_type_changed (
  GtkCheckMenuItem * menuitem,
  RulerWidget *      self)
{
  if (menuitem == self->bbt_display_check &&
        gtk_check_menu_item_get_active (menuitem))
    {
      g_settings_set_enum (
        S_UI, "ruler-display",
        TRANSPORT_DISPLAY_BBT);
    }
  else if (menuitem == self->time_display_check &&
             gtk_check_menu_item_get_active (
               menuitem))
    {
      g_settings_set_enum (
        S_UI, "ruler-display",
        TRANSPORT_DISPLAY_TIME);
    }

  EVENTS_PUSH (ET_RULER_DISPLAY_TYPE_CHANGED, NULL);
}
#endif

static gboolean
on_right_click_pressed (
  GtkGestureClick *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  RulerWidget *         self)
{
  /* right click */
  if (n_press == 1)
    {
      GMenu * menu = g_menu_new ();

      GSimpleActionGroup * action_group =
        g_simple_action_group_new ();
      GAction * display_action =
        g_settings_create_action (
          S_UI, "ruler-display");

      g_action_map_add_action (
        G_ACTION_MAP (action_group),
        display_action);
      gtk_widget_insert_action_group (
        GTK_WIDGET (self), "ruler",
        G_ACTION_GROUP (action_group));

      g_menu_append (
        menu, _("Display"),
        "ruler.ruler-display");

      /* TODO fire event on change */

      z_gtk_show_context_menu_from_g_menu (
        self->popover_menu, x, y, menu);
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

  int punch_in_hit =
    is_punch_in_hit (self, start_x, start_y);
  int punch_out_hit =
    is_punch_out_hit (self, start_x, start_y);
  int loop_start_hit =
    is_loop_start_hit (self, start_x, start_y);
  int loop_end_hit =
    is_loop_end_hit (self, start_x, start_y);
  int clip_start_hit =
    is_clip_start_hit (self, start_x, start_y);

  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  ArrangerObject * r_obj =
    (ArrangerObject *) region;

  /* if one of the markers hit */
  if (punch_in_hit)
    {
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target =
        RW_TARGET_PUNCH_IN;
    }
  else if (punch_out_hit)
    {
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target =
        RW_TARGET_PUNCH_OUT;
    }
  else if (loop_start_hit)
    {
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target =
        RW_TARGET_LOOP_START;
      if (self->type == TYPE (EDITOR))
        {
          g_return_if_fail (region);
          self->drag_start_pos =
            r_obj->loop_start_pos;
        }
      else
        {
          self->drag_start_pos =
            TRANSPORT->loop_start_pos;
        }
    }
  else if (loop_end_hit)
    {
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target =
        RW_TARGET_LOOP_END;
      if (self->type == TYPE (EDITOR))
        {
          g_return_if_fail (region);
          self->drag_start_pos =
            r_obj->loop_end_pos;
        }
      else
        {
          self->drag_start_pos =
            TRANSPORT->loop_end_pos;
        }
    }
  else if (clip_start_hit)
    {
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = RW_TARGET_CLIP_START;
      if (self->type == TYPE (EDITOR))
        {
          g_return_if_fail (region);
          self->drag_start_pos =
            r_obj->clip_start_pos;
        }
    }
  else
    {
      if (self->type == TYPE (TIMELINE))
        {
          timeline_ruler_on_drag_begin_no_marker_hit (
            self, start_x, start_y, height);
        }
      else if (self->type == TYPE (EDITOR))
        {
          editor_ruler_on_drag_begin_no_marker_hit (
            self, start_x, start_y);
        }
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

static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  RulerWidget *              self)
{
  GdkEvent * event =
    gtk_event_controller_get_current_event (
      GTK_EVENT_CONTROLLER (motion_controller));
  GdkEventType event_type =
    gdk_event_get_event_type (event);
  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (motion_controller));

  /* drag-update didn't work so do the drag-update
   * here */
  if (self->dragging
      && event_type != GDK_LEAVE_NOTIFY)
    {
      if (state & GDK_SHIFT_MASK)
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
            self, x - self->start_x,
            y - self->start_y);
        }
      else if (self->type == TYPE (EDITOR))
        {
          editor_ruler_on_drag_update (
            self, x - self->start_x,
            y - self->start_y);
        }

      self->last_offset_x =
        x - self->start_x;

      return;
    }

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  if (event_type == GDK_MOTION_NOTIFY)
    {
      bool punch_in_hit =
        is_punch_in_hit (
          self, x, y);
      bool punch_out_hit =
        is_punch_out_hit (
          self, x, y);
      bool loop_start_hit =
        is_loop_start_hit (
          self, x, y);
      bool loop_end_hit =
        is_loop_end_hit (
          self, x, y);
      bool clip_start_hit =
        is_clip_start_hit (
          self, x, y);
      bool range_start_hit =
        ruler_widget_is_range_hit (
          self, RW_RANGE_START,
          x, y);
      bool range_end_hit =
        ruler_widget_is_range_hit (
          self, RW_RANGE_END, x, y);

      if (punch_in_hit ||
          loop_start_hit || clip_start_hit ||
          range_start_hit)
        {
          ui_set_cursor_from_name (
            GTK_WIDGET (self), "w-resize");
        }
      else if (punch_out_hit ||
               loop_end_hit || range_end_hit)
        {
          ui_set_cursor_from_name (
            GTK_WIDGET (self), "e-resize");
        }
      /* if lower 3/4ths */
      else if (y > (height * 1) / 4)
        {
          /* set cursor to normal */
          ui_set_cursor_from_name (
            GTK_WIDGET (self), "default");
        }
      else /* upper 1/4th */
        {
          if (ruler_widget_is_range_hit (
                self, RW_RANGE_FULL, x,
                y))
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
}

static void
on_leave (
  GtkEventControllerMotion * motion_controller,
  RulerWidget *              self)
{
  ui_set_cursor_from_name (
    GTK_WIDGET (self), "default");
}

void
ruler_widget_refresh (RulerWidget * self)
{
  /*adjust for zoom level*/
  self->px_per_tick =
    DEFAULT_PX_PER_TICK *
    ruler_widget_get_zoom_level (self);
  self->px_per_sixteenth =
    self->px_per_tick * TICKS_PER_SIXTEENTH_NOTE;
  self->px_per_beat =
    self->px_per_tick * TRANSPORT->ticks_per_beat;
  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  self->px_per_bar =
    self->px_per_beat * beats_per_bar;

  Position pos;
  position_from_seconds (&pos, 1.0);
  self->px_per_min =
    60.0 * pos.ticks * self->px_per_tick;
  self->px_per_10sec =
    10.0 * pos.ticks * self->px_per_tick;
  self->px_per_sec =
    pos.ticks * self->px_per_tick;
  self->px_per_100ms =
    0.1 * pos.ticks * self->px_per_tick;

  position_set_to_bar (
    &pos, TRANSPORT->total_bars + 1);
  double prev_total_px = self->total_px;
  self->total_px =
    self->px_per_tick *
    (double) position_to_ticks (&pos);

  /* if size changed */
  if (!math_doubles_equal_epsilon (
         prev_total_px, self->total_px, 0.1))
    {
      EVENTS_PUSH (
        ET_RULER_SIZE_CHANGED, self);
    }
}

GtkScrolledWindow *
ruler_widget_get_parent_scroll (
  RulerWidget * self)
{
  if (self->type == TYPE (TIMELINE))
    {
      return MW_TIMELINE_PANEL->ruler_scroll;
    }
  else if (self->type == TYPE (EDITOR))
    {
      return MW_CLIP_EDITOR_INNER->ruler_scroll;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Sets zoom level and disables/enables buttons
 * accordingly.
 *
 * @return Whether the zoom level was set.
 */
bool
ruler_widget_set_zoom_level (
  RulerWidget * self,
  double        zoom_level)
{
  if (zoom_level > MAX_ZOOM_LEVEL)
    {
      actions_set_app_action_enabled (
        "zoom-in", false);
    }
  else
    {
      actions_set_app_action_enabled (
        "zoom-in", true);
    }
  if (zoom_level < MIN_ZOOM_LEVEL)
    {
      actions_set_app_action_enabled (
        "zoom-out", false);
    }
  else
    {
      actions_set_app_action_enabled (
        "zoom-out", true);
    }

  int update = zoom_level >= MIN_ZOOM_LEVEL &&
    zoom_level <= MAX_ZOOM_LEVEL;

  if (update)
    {
      if (self->type == RULER_WIDGET_TYPE_TIMELINE)
        {
          PRJ_TIMELINE->editor_settings.
            hzoom_level =
              zoom_level;
        }
      else if (self->type ==
                 RULER_WIDGET_TYPE_EDITOR)
        {
          ArrangerWidget * arr =
            clip_editor_inner_widget_get_visible_arranger (
              MW_CLIP_EDITOR_INNER);
          EditorSettings * settings =
            arranger_widget_get_editor_settings (
              arr);
          g_return_val_if_fail (settings, false);
          settings->hzoom_level = zoom_level;
        }
      ruler_widget_refresh (self);
      return true;
    }
  else
    {
      return false;
    }
}

static guint
ruler_tick_cb (
  GtkWidget* widget,
  GtkTickCallback callback,
  gpointer user_data,
  GDestroyNotify notify)
{
  gtk_widget_queue_draw (widget);

  return 5;
}

static void
dispose (
  RulerWidget * self)
{
  gtk_widget_unparent (
    GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (
    ruler_widget_parent_class)->
      dispose (G_OBJECT (self));
}

static void
ruler_widget_init (RulerWidget * self)
{
  self->popover_menu =
    GTK_POPOVER_MENU (
      gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (
    GTK_WIDGET (self->popover_menu),
    GTK_WIDGET (self));

  self->drag =
    GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->drag));

  self->click =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (self->click), "pressed",
    G_CALLBACK (on_click_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->click));

  GtkGestureClick * right_mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_mp), "pressed",
    G_CALLBACK (on_right_click_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (right_mp));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "motion",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave",
    G_CALLBACK (on_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (motion_controller));

  self->layout_normal =
    gtk_widget_create_pango_layout (
      GTK_WIDGET (self), NULL);
  PangoFontDescription *desc =
    pango_font_description_from_string (
      "Monospace 11");
  pango_layout_set_font_description (
    self->layout_normal, desc);
  pango_font_description_free (desc);
  self->layout_small =
    gtk_widget_create_pango_layout (
      GTK_WIDGET (self), NULL);
  desc =
    pango_font_description_from_string (
      "Monospace 6");
  pango_layout_set_font_description (
    self->layout_small, desc);
  pango_font_description_free (desc);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self),
    (GtkTickCallback) ruler_tick_cb,
    self, NULL);
}

static void
ruler_widget_class_init (
  RulerWidgetClass * klass)
{
  GtkWidgetClass * wklass =
    GTK_WIDGET_CLASS (klass);
  wklass->snapshot = ruler_snapshot;
  gtk_widget_class_set_css_name (
    wklass, "ruler");

  gtk_widget_class_set_layout_manager_type (
    wklass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->dispose =
    (GObjectFinalizeFunc) dispose;
}
