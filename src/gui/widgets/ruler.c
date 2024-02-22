// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <math.h>

#include "actions/actions.h"
#include "dsp/marker_track.h"
#include "dsp/position.h"
#include "dsp/tempo_track.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/timeline.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_minimap.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (RulerWidget, ruler_widget, GTK_TYPE_WIDGET)

#define TYPE(x) RULER_WIDGET_TYPE_##x

#define ACTION_IS(x) (self->action == UI_OVERLAY_ACTION_##x)

double
ruler_widget_get_zoom_level (RulerWidget * self)
{
  EditorSettings * settings = ruler_widget_get_editor_settings (self);
  g_return_val_if_fail (settings, 1.0);

  return settings->hzoom_level;

  g_return_val_if_reached (1.f);
}

/**
 * Returns the beat interval for drawing vertical
 * lines.
 */
int
ruler_widget_get_beat_interval (RulerWidget * self)
{
  int i;

  /* gather divisors of the number of beats per
   * bar */
  int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int beat_divisors[16];
  int num_beat_divisors = 0;
  for (i = 1; i <= beats_per_bar; i++)
    {
      if (beats_per_bar % i == 0)
        beat_divisors[num_beat_divisors++] = i;
    }

  /* decide the raw interval to keep between beats */
  int _beat_interval = MAX ((int) (RW_PX_TO_HIDE_BEATS / self->px_per_beat), 1);

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
ruler_widget_get_sixteenth_interval (RulerWidget * self)
{
  int i;

  /* gather divisors of the number of sixteenths per
   * beat */
#define sixteenths_per_beat TRANSPORT->sixteenths_per_beat
  int divisors[16];
  int num_divisors = 0;
  for (i = 1; i <= sixteenths_per_beat; i++)
    {
      if (sixteenths_per_beat % i == 0)
        divisors[num_divisors++] = i;
    }

  /* decide the raw interval to keep between sixteenths */
  int _sixteenth_interval =
    MAX ((int) (RW_PX_TO_HIDE_BEATS / self->px_per_sixteenth), 1);

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
ruler_widget_get_10sec_interval (RulerWidget * self)
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
    MAX ((int) (RW_PX_TO_HIDE_BEATS / self->px_per_10sec), 1);

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
ruler_widget_get_sec_interval (RulerWidget * self)
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
  int _sec_interval = MAX ((int) (RW_PX_TO_HIDE_BEATS / self->px_per_sec), 1);

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
draw_other_region (RulerWidget * self, GtkSnapshot * snapshot, ZRegion * region)
{
  /*g_debug (
   * "drawing other region %s", region->name);*/

  int height = gtk_widget_get_height (GTK_WIDGET (self));

  ArrangerObject * r_obj = (ArrangerObject *) region;
  Track *          track = arranger_object_get_track (r_obj);
  GdkRGBA          color = track->color;
  color.alpha = 0.5;

  int px_start, px_end;
  px_start = ui_pos_to_px_editor (&r_obj->pos, true);
  px_end = ui_pos_to_px_editor (&r_obj->end_pos, true);
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      (float) px_start, 0.f, (float) px_end - (float) px_start,
      (float) height / 4.f));
}

/**
 * Draws the regions in the editor ruler.
 */
static void
draw_regions (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  int height = gtk_widget_get_height (GTK_WIDGET (self));

  /* get a visible region - the clip editor
   * region is removed temporarily while moving
   * regions so this could be NULL */
  ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    return;

  ArrangerObject * region_obj = (ArrangerObject *) region;

  Track * track = arranger_object_get_track (region_obj);

  int px_start, px_end;

  /* draw the main region */
  GdkRGBA color = track->color;
  ui_get_arranger_object_color (
    &color, MW_TIMELINE->hovered_object == region_obj,
    region_is_selected (region), false,
    arranger_object_get_muted (region_obj, true));
  px_start = ui_pos_to_px_editor (&region_obj->pos, 1);
  px_end = ui_pos_to_px_editor (&region_obj->end_pos, 1);
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      (float) px_start, 0.f, (float) px_end - (float) px_start,
      (float) height / 4.f));

  /* draw its transient if copy-moving TODO */
  if (arranger_object_should_orig_be_visible (region_obj, NULL))
    {
      px_start = ui_pos_to_px_editor (&region_obj->pos, 1);
      px_end = ui_pos_to_px_editor (&region_obj->end_pos, 1);
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          (float) px_start, 0.f, (float) px_end - (float) px_start,
          (float) height / 4.f));
    }

  /* draw the other regions */
  ZRegion * other_regions[1000];
  int       num_other_regions =
    track_get_regions_in_range (track, NULL, NULL, other_regions);
  for (int i = 0; i < num_other_regions; i++)
    {
      ZRegion * other_region = other_regions[i];
      if (
        region_identifier_is_equal (&region->id, &other_region->id)
        || region->id.type != other_region->id.type)
        {
          continue;
        }

      draw_other_region (self, snapshot, other_region);
    }
}

static void
get_loop_start_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x = 0;
  if (self->type == TYPE (EDITOR))
    {
      ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
      if (region)
        {
          ArrangerObject * region_obj = (ArrangerObject *) region;
          double           start_ticks = position_to_ticks (&region_obj->pos);
          double           loop_start_ticks =
            position_to_ticks (&region_obj->loop_start_pos) + start_ticks;
          Position tmp;
          position_from_ticks (&tmp, loop_start_ticks);
          rect->x = ui_pos_to_px_editor (&tmp, 1);
        }
      else
        {
          rect->x = 0;
        }
    }
  else if (self->type == TYPE (TIMELINE))
    {
      rect->x = ui_pos_to_px_timeline (&TRANSPORT->loop_start_pos, 1);
    }
  rect->y = 0;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_loop_start (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_loop_start_rect (self, &dr);

  cairo_t * cr = gtk_snapshot_append_cairo (
    snapshot,
    &GRAPHENE_RECT_INIT (
      (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height));

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0, 0.9, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, dr.x, dr.y + dr.height);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_loop_end_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x = 0;
  if (self->type == TYPE (EDITOR))
    {
      ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
      if (region)
        {
          ArrangerObject * region_obj = (ArrangerObject *) region;
          double           start_ticks = position_to_ticks (&region_obj->pos);
          double           loop_end_ticks =
            position_to_ticks (&region_obj->loop_end_pos) + start_ticks;
          Position tmp;
          position_from_ticks (&tmp, loop_end_ticks);
          rect->x = ui_pos_to_px_editor (&tmp, 1) - RW_RULER_MARKER_SIZE;
        }
      else
        {
          rect->x = 0;
        }
    }
  else if (self->type == TYPE (TIMELINE))
    {
      rect->x =
        ui_pos_to_px_timeline (&TRANSPORT->loop_end_pos, 1)
        - RW_RULER_MARKER_SIZE;
    }
  rect->y = 0;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_loop_end (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_loop_end_rect (self, &dr);

  cairo_t * cr = gtk_snapshot_append_cairo (
    snapshot,
    &GRAPHENE_RECT_INIT (
      (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height));

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0, 0.9, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y + dr.height);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_punch_in_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x = ui_pos_to_px_timeline (&TRANSPORT->punch_in_pos, 1);
  rect->y = RW_RULER_MARKER_SIZE;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_punch_in (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_punch_in_rect (self, &dr);

  cairo_t * cr = gtk_snapshot_append_cairo (
    snapshot,
    &GRAPHENE_RECT_INIT (
      (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height));

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0.9, 0.1, 0.1);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, dr.x, dr.y + dr.height);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_punch_out_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x =
    ui_pos_to_px_timeline (&TRANSPORT->punch_out_pos, 1) - RW_RULER_MARKER_SIZE;
  rect->y = RW_RULER_MARKER_SIZE;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_punch_out (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_punch_out_rect (self, &dr);

  cairo_t * cr = gtk_snapshot_append_cairo (
    snapshot,
    &GRAPHENE_RECT_INIT (
      (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height));

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0.9, 0.1, 0.1);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y + dr.height);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_clip_start_rect (RulerWidget * self, GdkRectangle * rect)
{
  /* TODO these were added to fix unused
   * warnings - check again if valid */
  rect->x = 0;
  rect->y = 0;

  if (self->type == TYPE (EDITOR))
    {
      ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
      if (region)
        {
          ArrangerObject * region_obj = (ArrangerObject *) region;
          double           start_ticks = position_to_ticks (&region_obj->pos);
          double           clip_start_ticks =
            position_to_ticks (&region_obj->clip_start_pos) + start_ticks;
          Position tmp;
          position_from_ticks (&tmp, clip_start_ticks);
          rect->x = ui_pos_to_px_editor (&tmp, 1);
        }
      else
        {
          rect->x = 0;
        }
      rect->y =
        ((gtk_widget_get_height (GTK_WIDGET (self)) - RW_RULER_MARKER_SIZE)
         - RW_CUE_MARKER_HEIGHT)
        - 1;
    }
  rect->width = RW_CUE_MARKER_WIDTH;
  rect->height = RW_CUE_MARKER_HEIGHT;
}

static void
get_cue_pos_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x = 0;
  rect->y = 0;

  if (self->type == TYPE (TIMELINE))
    {
      rect->x = ui_pos_to_px_timeline (&TRANSPORT->cue_pos, 1);
      rect->y = RW_RULER_MARKER_SIZE;
    }
  rect->width = RW_CUE_MARKER_WIDTH;
  rect->height = RW_CUE_MARKER_HEIGHT;
}

/**
 * Draws the cue point (or clip start if this is the editor
 * ruler.
 */
static void
draw_cue_point (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  if (self->type == TYPE (EDITOR))
    {
      get_clip_start_rect (self, &dr);
    }
  else if (self->type == TYPE (TIMELINE))
    {
      get_cue_pos_rect (self, &dr);
    }
  else
    {
      g_warn_if_reached ();
      return;
    }

  cairo_t * cr = gtk_snapshot_append_cairo (
    snapshot,
    &GRAPHENE_RECT_INIT (
      (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height));

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      if (self->type == TYPE (EDITOR))
        {
          /*cairo_set_source_rgb (cr, 0.2, 0.6, 0.9);*/
          cairo_set_source_rgb (cr, 1, 0.2, 0.2);
        }
      else if (self->type == TYPE (TIMELINE))
        {
          cairo_set_source_rgb (cr, 0, 0.6, 0.9);
        }
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y + dr.height / 2);
      cairo_line_to (cr, dr.x, dr.y + dr.height);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
draw_markers (RulerWidget * self, GtkSnapshot * snapshot, int height)
{
  Track * track = P_MARKER_TRACK;
  for (int i = 0; i < track->num_markers; i++)
    {
      const Marker * m = track->markers[i];
      const float    cur_px = (float) ui_pos_to_px_timeline (&m->base.pos, 1);
      gtk_snapshot_append_color (
        snapshot, &track->color, &GRAPHENE_RECT_INIT (cur_px, 6.f, 2.f, 12.f));

      int textw, texth;
      pango_layout_set_text (self->marker_layout, m->name, -1);
      pango_layout_get_pixel_size (self->marker_layout, &textw, &texth);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot, &GRAPHENE_POINT_INIT (cur_px + 2.f, 6.f));
      gtk_snapshot_append_layout (
        snapshot, self->marker_layout, &Z_GDK_RGBA_INIT (1, 1, 1, 1.f));
      gtk_snapshot_restore (snapshot);
    }
}

/**
 * Returns the playhead's x coordinate in absolute
 * coordinates.
 *
 * @param after_loops Whether to get the playhead px after
 *   loops are applied.
 */
int
ruler_widget_get_playhead_px (RulerWidget * self, bool after_loops)
{
  if (self->type == TYPE (EDITOR))
    {
      if (after_loops)
        {
          long      frames = 0;
          ZRegion * clip_editor_region = clip_editor_get_region (CLIP_EDITOR);
          if (!clip_editor_region)
            {
              g_warning ("no clip editor region");
              return ui_pos_to_px_editor (PLAYHEAD, 1);
            }

          ZRegion * r = NULL;

          if (clip_editor_region->id.type == REGION_TYPE_AUTOMATION)
            {
              AutomationTrack * at =
                region_get_automation_track (clip_editor_region);
              r = region_at_position (NULL, at, PLAYHEAD);
            }
          else
            {
              r = region_at_position (
                arranger_object_get_track ((ArrangerObject *) clip_editor_region),
                NULL, PLAYHEAD);
            }
          Position tmp;
          if (r)
            {
              ArrangerObject * obj = (ArrangerObject *) r;
              signed_frame_t   region_local_frames = (signed_frame_t)
                region_timeline_frames_to_local (r, PLAYHEAD->frames, 1);
              region_local_frames += obj->pos.frames;
              position_from_frames (&tmp, region_local_frames);
              frames = tmp.frames;
            }
          else
            {
              frames = PLAYHEAD->frames;
            }
          Position pos;
          position_from_frames (&pos, frames);
          return ui_pos_to_px_editor (&pos, 1);
        }
      else /* else if not after loops */
        {
          return ui_pos_to_px_editor (PLAYHEAD, 1);
        }
    }
  else if (self->type == TYPE (TIMELINE))
    {
      return ui_pos_to_px_timeline (PLAYHEAD, 1);
    }
  g_return_val_if_reached (-1);
}

static void
draw_playhead (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  int px = ruler_widget_get_playhead_px (self, true);

  if (
    px >= rect->x - RW_PLAYHEAD_TRIANGLE_WIDTH / 2
    && px <= (rect->x + rect->width) - RW_PLAYHEAD_TRIANGLE_WIDTH / 2)
    {
      self->last_playhead_px = px;

      /* draw rect */
      GdkRectangle dr = { 0, 0, 0, 0 };
      dr.x = px - (RW_PLAYHEAD_TRIANGLE_WIDTH / 2);
      dr.y =
        gtk_widget_get_height (GTK_WIDGET (self)) - RW_PLAYHEAD_TRIANGLE_HEIGHT;
      dr.width = RW_PLAYHEAD_TRIANGLE_WIDTH;
      dr.height = RW_PLAYHEAD_TRIANGLE_HEIGHT;

      cairo_t * cr = gtk_snapshot_append_cairo (
        snapshot,
        &GRAPHENE_RECT_INIT (
          (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height));

      cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, (dr.x + dr.width / 2), dr.y + dr.height);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
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
  int    i = 0;
  double curr_px;
  char   text[40];
  int    textw, texth;

  int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int height = gtk_widget_get_height (GTK_WIDGET (self));

  GdkRGBA main_color = { 1, 1, 1, 1 };
  GdkRGBA secondary_color = { 0.7f, 0.7f, 0.7f, 0.4f };
  GdkRGBA tertiary_color = { 0.5f, 0.5f, 0.5f, 0.3f };
  GdkRGBA main_text_color = { 0.8f, 0.8f, 0.8f, 1 };
  GdkRGBA secondary_text_color = { 0.5f, 0.5f, 0.5f, 1 };
  GdkRGBA tertiary_text_color = secondary_text_color;

  /* if time display */
  if (g_settings_get_enum (S_UI, "ruler-display") == TRANSPORT_DISPLAY_TIME)
    {
      /* get sec interval */
      int sec_interval = ruler_widget_get_sec_interval (self);

      /* get 10 sec interval */
      int ten_sec_interval = ruler_widget_get_10sec_interval (self);

      /* get the interval for mins */
      int min_interval =
        (int) MAX ((RW_PX_TO_HIDE_BEATS) / (double) self->px_per_min, 1.0);

      /* draw mins */
      i = -min_interval;
      while (
        (curr_px = self->px_per_min * (i += min_interval) + SPACE_BEFORE_START)
        < (double) (rect->x + rect->width) + 20.0)
        {
          if (curr_px + 20.0 < rect->x)
            continue;

          gtk_snapshot_append_color (
            snapshot, &main_color,
            &GRAPHENE_RECT_INIT (
              (float) curr_px, 0.f, 1.f, (float) height / 3.f));

          sprintf (text, "%d", i);
          pango_layout_set_text (self->layout_normal, text, -1);
          pango_layout_get_pixel_size (self->layout_normal, &textw, &texth);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              (float) curr_px + 1.f - (float) textw / 2.f,
              (float) height / 3.f + 2.f));
          gtk_snapshot_append_layout (
            snapshot, self->layout_normal, &main_text_color);
          gtk_snapshot_restore (snapshot);
        }
      /* draw 10secs */
      i = 0;
      if (ten_sec_interval > 0)
        {
          while (
            (curr_px =
               self->px_per_10sec * (i += ten_sec_interval) + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &secondary_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, 0.f, 1.f, (float) height / 4.f));

              if (
                (self->px_per_10sec > RW_PX_TO_HIDE_BEATS * 2)
                && i % ten_secs_per_min != 0)
                {
                  sprintf (
                    text, "%d:%02d", i / ten_secs_per_min,
                    (i % ten_secs_per_min) * 10);
                  pango_layout_set_text (self->monospace_layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->monospace_layout_small, &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  gtk_snapshot_translate (
                    snapshot,
                    &GRAPHENE_POINT_INIT (
                      (float) curr_px + 1.f - (float) textw / 2.f,
                      (float) height / 4.f + 2.f));
                  gtk_snapshot_append_layout (
                    snapshot, self->monospace_layout_small,
                    &secondary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
      /* draw secs */
      i = 0;
      if (sec_interval > 0)
        {
          while (
            (curr_px = self->px_per_sec * (i += sec_interval) + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &tertiary_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, 0.f, 1.f, (float) height / 6.f));

              if (
                (self->px_per_sec > RW_PX_TO_HIDE_BEATS * 2)
                && i % secs_per_10_sec != 0)
                {
                  int secs_per_min = 60;
                  sprintf (
                    text, "%d:%02d", i / secs_per_min,
                    ((i / secs_per_10_sec) % ten_secs_per_min) * 10
                      + i % secs_per_10_sec);
                  pango_layout_set_text (self->monospace_layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->monospace_layout_small, &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  gtk_snapshot_translate (
                    snapshot,
                    &GRAPHENE_POINT_INIT (
                      (float) curr_px + 1.f - (float) textw / 2.f,
                      (float) height / 4.f + 2.f));
                  gtk_snapshot_append_layout (
                    snapshot, self->monospace_layout_small,
                    &tertiary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
    }
  else /* else if BBT display */
    {
      /* get sixteenth interval */
      int sixteenth_interval = ruler_widget_get_sixteenth_interval (self);

      /* get beat interval */
      int beat_interval = ruler_widget_get_beat_interval (self);

      /* get the interval for bars */
      int bar_interval =
        (int) MAX ((RW_PX_TO_HIDE_BEATS) / (double) self->px_per_bar, 1.0);

      /* draw bars */
      i = -bar_interval;
      while (
        (curr_px = self->px_per_bar * (i += bar_interval) + SPACE_BEFORE_START)
        < (double) (rect->x + rect->width) + 20.0)
        {
          if (curr_px + 20.0 < rect->x)
            continue;

          gtk_snapshot_append_color (
            snapshot, &main_color,
            &GRAPHENE_RECT_INIT (
              (float) curr_px, 0.f, 1.f, (float) height / 3.f));

          sprintf (text, "%d", i + 1);
          pango_layout_set_text (self->layout_normal, text, -1);
          pango_layout_get_pixel_size (self->layout_normal, &textw, &texth);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              (float) curr_px + 1.f - (float) textw / 2.f,
              (float) height / 3.f + 2.f));
          gtk_snapshot_append_layout (
            snapshot, self->layout_normal, &main_text_color);
          gtk_snapshot_restore (snapshot);
        }
      /* draw beats */
      i = 0;
      if (beat_interval > 0)
        {
          while (
            (curr_px =
               self->px_per_beat * (i += beat_interval) + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &secondary_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, 0.f, 1.f, (float) height / 4.f));

              if (
                (self->px_per_beat > RW_PX_TO_HIDE_BEATS * 2)
                && i % beats_per_bar != 0)
                {
                  sprintf (
                    text, "%d.%d", i / beats_per_bar + 1, i % beats_per_bar + 1);
                  pango_layout_set_text (self->monospace_layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->monospace_layout_small, &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  gtk_snapshot_translate (
                    snapshot,
                    &GRAPHENE_POINT_INIT (
                      (float) curr_px + 1.f - (float) textw / 2.f,
                      (float) height / 4.f + 2.f));
                  gtk_snapshot_append_layout (
                    snapshot, self->monospace_layout_small,
                    &secondary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
      /* draw sixteenths */
      i = 0;
      if (sixteenth_interval > 0)
        {
          while (
            (curr_px =
               self->px_per_sixteenth * (i += sixteenth_interval)
               + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &tertiary_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, 0.f, 1.f, (float) height / 6.f));

              if (
                (self->px_per_sixteenth > RW_PX_TO_HIDE_BEATS * 2)
                && i % sixteenths_per_beat != 0)
                {
                  sprintf (
                    text, "%d.%d.%d", i / TRANSPORT->sixteenths_per_bar + 1,
                    ((i / sixteenths_per_beat) % beats_per_bar) + 1,
                    i % sixteenths_per_beat + 1);
                  pango_layout_set_text (self->monospace_layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->monospace_layout_small, &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  gtk_snapshot_translate (
                    snapshot,
                    &GRAPHENE_POINT_INIT (
                      (float) curr_px + 1.f - (float) textw / 2.f,
                      (float) height / 4.f + 2.f));
                  gtk_snapshot_append_layout (
                    snapshot, self->monospace_layout_small,
                    &tertiary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
    }
}

static void
ruler_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  RulerWidget * self = Z_RULER_WIDGET (widget);

  GdkRectangle visible_rect_gdk;
  ruler_widget_get_visible_rect (self, &visible_rect_gdk);
  graphene_rect_t visible_rect;
  z_gdk_rectangle_to_graphene_rect_t (&visible_rect, &visible_rect_gdk);

  /* engine is run only set after everything is set
   * up so this is a good way to decide if we
   * should  draw or not */
  if (
    !PROJECT || !AUDIO_ENGINE || !g_atomic_int_get (&AUDIO_ENGINE->run)
    || self->px_per_bar < 2.0)
    {
      return;
    }

  self->last_rect = visible_rect;

  /* pretend we're drawing from 0, 0 */
  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (-visible_rect.origin.x, -visible_rect.origin.y));

  /* ----- ruler background ------- */

  int height = gtk_widget_get_height (GTK_WIDGET (self));

  /* if timeline, draw loop background */
  /* FIXME use rect */
  double start_px = 0, end_px = 0;
  if (self->type == TYPE (TIMELINE))
    {
      start_px = ui_pos_to_px_timeline (&TRANSPORT->loop_start_pos, 1);
      end_px = ui_pos_to_px_timeline (&TRANSPORT->loop_end_pos, 1);
    }
  else if (self->type == TYPE (EDITOR))
    {
      start_px = ui_pos_to_px_editor (&TRANSPORT->loop_start_pos, 1);
      end_px = ui_pos_to_px_editor (&TRANSPORT->loop_end_pos, 1);
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
  if (
    start_px + 2.0 > visible_rect_gdk.x
    && start_px <= visible_rect_gdk.x + visible_rect_gdk.width)
    {
      /* draw the loop start line */
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          (float) start_px, 0.f, (float) line_width,
          (float) visible_rect.size.height));
    }
  /* if transport loop end is within the
   * screen */
  if (
    end_px + 2.0 > visible_rect_gdk.x
    && end_px <= visible_rect_gdk.x + visible_rect_gdk.width)
    {
      /* draw the loop end line */
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          (float) end_px, 0.f, (float) line_width,
          (float) visible_rect.size.height));
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
      (float) start_px, 0, (float) (end_px - start_px), (float) height),
    &GRAPHENE_POINT_INIT (0, 0),
    &GRAPHENE_POINT_INIT (0.f, ((float) height * 3.f) / 4.f), stops,
    G_N_ELEMENTS (stops));

  draw_lines_and_labels (self, snapshot, &visible_rect_gdk);

  /* ----- draw range --------- */

  if (TRANSPORT->has_range)
    {
      int range1_first =
        position_is_before_or_equal (&TRANSPORT->range_1, &TRANSPORT->range_2);

      GdkRectangle dr;
      if (range1_first)
        {
          dr.x = ui_pos_to_px_timeline (&TRANSPORT->range_1, true);
          dr.width = ui_pos_to_px_timeline (&TRANSPORT->range_2, true) - dr.x;
        }
      else
        {
          dr.x = ui_pos_to_px_timeline (&TRANSPORT->range_2, true);
          dr.width = ui_pos_to_px_timeline (&TRANSPORT->range_1, true) - dr.x;
        }
      dr.y = 0;
      dr.height =
        gtk_widget_get_height (GTK_WIDGET (self)) / RW_RANGE_HEIGHT_DIVISOR;

      /* fill */
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.27f),
        &GRAPHENE_RECT_INIT (
          (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height));

      /* draw edges */
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.7f),
        &GRAPHENE_RECT_INIT ((float) dr.x, (float) dr.y, 2.f, (float) dr.height));
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.7f),
        &GRAPHENE_RECT_INIT (
          (float) dr.x + (float) dr.width, (float) dr.y, 2.f,
          (float) dr.height - (float) dr.y));
    }

  /* ----- draw regions --------- */

  if (self->type == TYPE (EDITOR))
    {
      draw_regions (self, snapshot, &visible_rect_gdk);
    }

  /* ------ draw markers ------- */

  draw_cue_point (self, snapshot, &visible_rect_gdk);
  draw_loop_start (self, snapshot, &visible_rect_gdk);
  draw_loop_end (self, snapshot, &visible_rect_gdk);

  if (self->type == TYPE (TIMELINE) && TRANSPORT->punch_mode)
    {
      draw_punch_in (self, snapshot, &visible_rect_gdk);
      draw_punch_out (self, snapshot, &visible_rect_gdk);
    }

  if (self->type == TYPE (EDITOR))
    {
      draw_markers (self, snapshot, height);
    }

  /* --------- draw playhead ---------- */

  draw_playhead (self, snapshot, &visible_rect_gdk);

  gtk_snapshot_restore (snapshot);
}

#undef beats_per_bar
#undef sixteenths_per_beat

static int
is_loop_start_hit (RulerWidget * self, double x, double y)
{
  GdkRectangle rect;
  get_loop_start_rect (self, &rect);

  return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
         && y <= rect.y + rect.height;
}

static int
is_loop_end_hit (RulerWidget * self, double x, double y)
{
  GdkRectangle rect;
  get_loop_end_rect (self, &rect);

  return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
         && y <= rect.y + rect.height;
}

static int
is_punch_in_hit (RulerWidget * self, double x, double y)
{
  GdkRectangle rect;
  get_punch_in_rect (self, &rect);

  return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
         && y <= rect.y + rect.height;
}

static int
is_punch_out_hit (RulerWidget * self, double x, double y)
{
  GdkRectangle rect;
  get_punch_out_rect (self, &rect);

  return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
         && y <= rect.y + rect.height;
}

static bool
is_clip_start_hit (RulerWidget * self, double x, double y)
{
  if (self->type == TYPE (EDITOR))
    {
      GdkRectangle rect;
      get_clip_start_rect (self, &rect);

      return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
             && y <= rect.y + rect.height;
    }
  else
    return false;
}

static void
get_range_rect (RulerWidget * self, RulerWidgetRangeType type, GdkRectangle * rect)
{
  g_return_if_fail (self->type == TYPE (TIMELINE));

  Position tmp;
  transport_get_range_pos (TRANSPORT, type == RW_RANGE_END ? false : true, &tmp);
  rect->x = ui_pos_to_px_timeline (&tmp, true);
  if (type == RW_RANGE_END)
    {
      rect->x -= RW_CUE_MARKER_WIDTH;
    }
  rect->y = 0;
  if (type == RW_RANGE_FULL)
    {
      transport_get_range_pos (TRANSPORT, false, &tmp);
      double px = ui_pos_to_px_timeline (&tmp, true);
      rect->width = (int) px;
    }
  else
    {
      rect->width = RW_CUE_MARKER_WIDTH;
    }
  rect->height =
    gtk_widget_get_height (GTK_WIDGET (self)) / RW_RANGE_HEIGHT_DIVISOR;
}

bool
ruler_widget_is_range_hit (
  RulerWidget *        self,
  RulerWidgetRangeType type,
  double               x,
  double               y)
{
  if (self->type == TYPE (TIMELINE) && TRANSPORT->has_range)
    {
      GdkRectangle rect;
      memset (&rect, 0, sizeof (GdkRectangle));
      get_range_rect (self, type, &rect);

      return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
             && y <= rect.y + rect.height;
    }
  else
    {
      return false;
    }
}

static gboolean
on_click_pressed (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  RulerWidget *     self)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  if (state & GDK_SHIFT_MASK)
    self->shift_held = 1;
  if (state & GDK_ALT_MASK)
    self->alt_held = true;

  if (n_press == 2)
    {
      if (self->type == TYPE (TIMELINE))
        {
          Position pos;
          ui_px_to_pos_timeline (x, &pos, 1);
          if (!self->shift_held && SNAP_GRID_ANY_SNAP (SNAP_GRID_TIMELINE))
            {
              position_snap (&pos, &pos, NULL, NULL, SNAP_GRID_TIMELINE);
            }
          position_set_to_pos (&TRANSPORT->cue_pos, &pos);
        }
      if (self->type == TYPE (EDITOR))
        {
        }
    }

  return FALSE;
}

static gboolean
on_right_click_pressed (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  RulerWidget *     self)
{
  /* right click */
  if (n_press == 1)
    {
      GMenu * menu = g_menu_new ();

      GMenu * section = g_menu_new ();
      g_menu_append (section, _ ("BBT"), "ruler.ruler-display::bbt");
      g_menu_append (section, _ ("Time"), "ruler.ruler-display::time");
      g_menu_append_section (menu, _ ("Display"), G_MENU_MODEL (section));

      z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
    }

  return FALSE;
}

/**
 * Sets the appropriate cursor based on the current
 * drag/hover/action status.
 */
static void
set_cursor (RulerWidget * self)
{
  if (self->action == UI_OVERLAY_ACTION_NONE)
    {
      if (self->hovering)
        {
          double x = self->hover_x;
          double y = self->hover_y;
          bool   punch_in_hit = is_punch_in_hit (self, x, y);
          bool   punch_out_hit = is_punch_out_hit (self, x, y);
          bool   loop_start_hit = is_loop_start_hit (self, x, y);
          bool   loop_end_hit = is_loop_end_hit (self, x, y);
          bool   clip_start_hit = is_clip_start_hit (self, x, y);
          bool   range_start_hit =
            ruler_widget_is_range_hit (self, RW_RANGE_START, x, y);
          bool range_end_hit =
            ruler_widget_is_range_hit (self, RW_RANGE_END, x, y);

          int height = gtk_widget_get_height (GTK_WIDGET (self));
          if (self->alt_held)
            {
              ui_set_cursor_from_name (GTK_WIDGET (self), "all-scroll");
            }
          else if (
            punch_in_hit || loop_start_hit || clip_start_hit || range_start_hit)
            {
              ui_set_cursor_from_name (GTK_WIDGET (self), "w-resize");
            }
          else if (punch_out_hit || loop_end_hit || range_end_hit)
            {
              ui_set_cursor_from_name (GTK_WIDGET (self), "e-resize");
            }
          /* if lower 3/4ths */
          else if (y > (height * 1) / 4)
            {
              /* set cursor to normal */
              /*g_debug ("lower 3/4ths - setting default");*/
              ui_set_cursor_from_name (GTK_WIDGET (self), "default");
            }
          else /* upper 1/4th */
            {
              if (ruler_widget_is_range_hit (self, RW_RANGE_FULL, x, y))
                {
                  /* set cursor to movable */
                  ui_set_hand_cursor (self);
                }
              else
                {
                  /* set cursor to range selection */
                  ui_set_time_select_cursor (self);
                }
            }
        }
      else
        {
          /*g_debug ("no hover - setting default");*/
          ui_set_cursor_from_name (GTK_WIDGET (self), "default");
        }
    }
  else
    {
      switch (self->action)
        {
        case UI_OVERLAY_ACTION_STARTING_PANNING:
        case UI_OVERLAY_ACTION_PANNING:
          ui_set_cursor_from_name (GTK_WIDGET (self), "all-scroll");
          break;
        case UI_OVERLAY_ACTION_STARTING_MOVING:
        case UI_OVERLAY_ACTION_MOVING:
          ui_set_cursor_from_name (GTK_WIDGET (self), "grabbing");
          break;
        default:
          /*g_debug ("no known action - setting default");*/
          ui_set_cursor_from_name (GTK_WIDGET (self), "default");
          break;
        }
    }
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  RulerWidget *    self)
{
  EditorSettings * settings = ruler_widget_get_editor_settings (self);
  g_return_if_fail (settings);
  start_x += settings->scroll_start_x;
  self->start_x = start_x;
  self->start_y = start_y;

  guint drag_start_btn =
    gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));

  if (self->type == TYPE (TIMELINE))
    {
      self->range1_first =
        position_is_before_or_equal (&TRANSPORT->range_1, &TRANSPORT->range_2);
    }

  int height = gtk_widget_get_height (GTK_WIDGET (self));

  int punch_in_hit = is_punch_in_hit (self, start_x, start_y);
  int punch_out_hit = is_punch_out_hit (self, start_x, start_y);
  int loop_start_hit = is_loop_start_hit (self, start_x, start_y);
  int loop_end_hit = is_loop_end_hit (self, start_x, start_y);
  int clip_start_hit = is_clip_start_hit (self, start_x, start_y);

  ZRegion *        region = clip_editor_get_region (CLIP_EDITOR);
  ArrangerObject * r_obj = (ArrangerObject *) region;

  /* if alt held down, start panning */
  if (self->alt_held || drag_start_btn == GDK_BUTTON_MIDDLE)
    {
      self->action = UI_OVERLAY_ACTION_STARTING_PANNING;
    }
  /* else if one of the markers hit */
  else if (punch_in_hit)
    {
      self->action = UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = RW_TARGET_PUNCH_IN;
    }
  else if (punch_out_hit)
    {
      self->action = UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = RW_TARGET_PUNCH_OUT;
    }
  else if (loop_start_hit)
    {
      self->action = UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = RW_TARGET_LOOP_START;
      if (self->type == TYPE (EDITOR))
        {
          g_return_if_fail (region);
          self->drag_start_pos = r_obj->loop_start_pos;
        }
      else
        {
          self->drag_start_pos = TRANSPORT->loop_start_pos;
        }
    }
  else if (loop_end_hit)
    {
      self->action = UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = RW_TARGET_LOOP_END;
      if (self->type == TYPE (EDITOR))
        {
          g_return_if_fail (region);
          self->drag_start_pos = r_obj->loop_end_pos;
        }
      else
        {
          self->drag_start_pos = TRANSPORT->loop_end_pos;
        }
    }
  else if (clip_start_hit)
    {
      self->action = UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = RW_TARGET_CLIP_START;
      if (self->type == TYPE (EDITOR))
        {
          g_return_if_fail (region);
          self->drag_start_pos = r_obj->clip_start_pos;
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
          editor_ruler_on_drag_begin_no_marker_hit (self, start_x, start_y);
        }
    }

  self->last_offset_x = 0;
  self->last_offset_y = 0;
  self->dragging = 1;
  self->vertical_panning_started = false;

  set_cursor (self);
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  RulerWidget *    self)
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

  set_cursor (self);
}

static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  RulerWidget *              self)
{
  GdkEvent * event = gtk_event_controller_get_current_event (
    GTK_EVENT_CONTROLLER (motion_controller));
  GdkEventType    event_type = gdk_event_get_event_type (event);
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (motion_controller));

  EditorSettings * settings = ruler_widget_get_editor_settings (self);
  g_return_if_fail (settings);
  x += settings->scroll_start_x;
  self->hover_x = x;
  self->hover_y = y;
  self->hovering = event_type != GDK_LEAVE_NOTIFY;

  if (state & GDK_SHIFT_MASK)
    self->shift_held = 1;
  else
    self->shift_held = 0;
  if (state & GDK_ALT_MASK)
    self->alt_held = 1;
  else
    self->alt_held = 0;

  /* drag-update didn't work so do the drag-update
   * here */
  if (self->dragging && event_type != GDK_LEAVE_NOTIFY)
    {
      if (ACTION_IS (STARTING_MOVING))
        {
          self->action = UI_OVERLAY_ACTION_MOVING;
        }
      else if (ACTION_IS (STARTING_PANNING))
        {
          self->action = UI_OVERLAY_ACTION_PANNING;
        }

      /* panning is common */
      double total_offset_x = x - self->start_x;
      double offset_x = total_offset_x - self->last_offset_x;
      double total_offset_y = y - self->start_y;
      double offset_y = total_offset_y - self->last_offset_y;
      if (self->action == UI_OVERLAY_ACTION_PANNING)
        {
          if (!math_doubles_equal_epsilon (offset_x, 0.0, 0.1))
            {
              /* pan horizontally */
              editor_settings_append_scroll (
                settings, (int) -offset_x, 0, F_VALIDATE);

              /* these are also affected */
              self->last_offset_x = MAX (0, self->last_offset_x - offset_x);
              self->hover_x = MAX (0, self->hover_x - offset_x);
              self->start_x = MAX (0, self->start_x - offset_x);
            }
          int drag_threshold;
          g_object_get (
            zrythm_app->default_settings, "gtk-dnd-drag-threshold",
            &drag_threshold, NULL);
          if (
            !math_doubles_equal_epsilon (offset_y, 0.0, 0.1)
            && (fabs (total_offset_y) >= drag_threshold || self->vertical_panning_started))
            {
              /* get position of cursor */
              Position cursor_pos;
              ruler_widget_px_to_pos (
                self, self->hover_x, &cursor_pos, F_PADDING);

              /* get px diff so we can calculate the new
               * adjustment later */
              double diff = self->hover_x - (double) settings->scroll_start_x;

              double scroll_multiplier = 1.0 - 0.02 * offset_y;
              ruler_widget_set_zoom_level (
                self, ruler_widget_get_zoom_level (self) * scroll_multiplier);

              int new_x = ruler_widget_pos_to_px (self, &cursor_pos, F_PADDING);

              editor_settings_set_scroll_start_x (
                settings, new_x - (int) diff, F_VALIDATE);

              /* also update hover x since we're using it here */
              self->hover_x = new_x;
              self->start_x = self->hover_x - total_offset_x;

              self->vertical_panning_started = true;
            }
        }
      else if (self->type == TYPE (TIMELINE))
        {
          timeline_ruler_on_drag_update (self, total_offset_x, total_offset_y);
        }
      else if (self->type == TYPE (EDITOR))
        {
          editor_ruler_on_drag_update (self, total_offset_x, total_offset_y);
        }

      self->last_offset_x = total_offset_x;
      self->last_offset_y = total_offset_y;

      set_cursor (self);

      return;
    }

  set_cursor (self);
}

static void
on_leave (GtkEventControllerMotion * motion_controller, RulerWidget * self)
{
  self->hovering = false;
}

void
ruler_widget_refresh (RulerWidget * self)
{
  /*adjust for zoom level*/
  self->px_per_tick = DEFAULT_PX_PER_TICK * ruler_widget_get_zoom_level (self);
  self->px_per_sixteenth = self->px_per_tick * TICKS_PER_SIXTEENTH_NOTE;
  self->px_per_beat = self->px_per_tick * TRANSPORT->ticks_per_beat;
  int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  self->px_per_bar = self->px_per_beat * beats_per_bar;

  Position pos;
  position_from_seconds (&pos, 1.0);
  self->px_per_min = 60.0 * pos.ticks * self->px_per_tick;
  self->px_per_10sec = 10.0 * pos.ticks * self->px_per_tick;
  self->px_per_sec = pos.ticks * self->px_per_tick;
  self->px_per_100ms = 0.1 * pos.ticks * self->px_per_tick;

  position_set_to_bar (&pos, TRANSPORT->total_bars + 1);
  double prev_total_px = self->total_px;
  self->total_px = self->px_per_tick * (double) position_to_ticks (&pos);

  /* if size changed */
  if (!math_doubles_equal_epsilon (prev_total_px, self->total_px, 0.1))
    {
      EVENTS_PUSH (ET_RULER_SIZE_CHANGED, self);
    }
}

/**
 * Gets the pointer to the EditorSettings associated with the
 * arranger this ruler is for.
 */
EditorSettings *
ruler_widget_get_editor_settings (RulerWidget * self)
{
  if (self->type == RULER_WIDGET_TYPE_TIMELINE)
    {
      return &PRJ_TIMELINE->editor_settings;
    }
  else if (self->type == RULER_WIDGET_TYPE_EDITOR)
    {
      ArrangerWidget * arr =
        clip_editor_inner_widget_get_visible_arranger (MW_CLIP_EDITOR_INNER);
      EditorSettings * settings = arranger_widget_get_editor_settings (arr);
      return settings;
    }
  g_return_val_if_reached (NULL);
}

/**
 * Fills in the visible rectangle.
 */
void
ruler_widget_get_visible_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->width = gtk_widget_get_width (GTK_WIDGET (self));
  rect->height = gtk_widget_get_height (GTK_WIDGET (self));
  const EditorSettings * settings = ruler_widget_get_editor_settings (self);
  g_return_if_fail (settings);
  rect->x = settings->scroll_start_x;
  rect->y = 0;
}

/**
 * Sets zoom level and disables/enables buttons
 * accordingly.
 *
 * @return Whether the zoom level was set.
 */
bool
ruler_widget_set_zoom_level (RulerWidget * self, double zoom_level)
{
  if (zoom_level > MAX_ZOOM_LEVEL)
    {
      actions_set_app_action_enabled ("zoom-in", false);
    }
  else
    {
      actions_set_app_action_enabled ("zoom-in", true);
    }
  if (zoom_level < MIN_ZOOM_LEVEL)
    {
      actions_set_app_action_enabled ("zoom-out", false);
    }
  else
    {
      actions_set_app_action_enabled ("zoom-out", true);
    }

  int update = zoom_level >= MIN_ZOOM_LEVEL && zoom_level <= MAX_ZOOM_LEVEL;

  if (update)
    {
      EditorSettings * settings = ruler_widget_get_editor_settings (self);
      g_return_val_if_fail (settings, false);
      settings->hzoom_level = zoom_level;
      ruler_widget_refresh (self);
      return true;
    }
  else
    {
      return false;
    }
}

void
ruler_widget_px_to_pos (
  RulerWidget * self,
  double        px,
  Position *    pos,
  bool          has_padding)
{
  if (self->type == TYPE (TIMELINE))
    {
      ui_px_to_pos_timeline (px, pos, has_padding);
    }
  else
    {
      ui_px_to_pos_editor (px, pos, has_padding);
    }
}

int
ruler_widget_pos_to_px (RulerWidget * self, Position * pos, int use_padding)
{
  if (self->type == TYPE (TIMELINE))
    {
      return ui_pos_to_px_timeline (pos, use_padding);
    }
  else
    {
      return ui_pos_to_px_editor (pos, use_padding);
    }
}

static gboolean
on_scroll (
  GtkEventControllerScroll * scroll_controller,
  gdouble                    dx,
  gdouble                    dy,
  gpointer                   user_data)
{
  RulerWidget * self = Z_RULER_WIDGET (user_data);

  double x = self->hover_x;

  GdkModifierType modifier_type = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (scroll_controller));

  bool ctrl_held = modifier_type & GDK_CONTROL_MASK;
  bool shift_held = modifier_type & GDK_SHIFT_MASK;
  if (ctrl_held)
    {

      if (!shift_held)
        {
          EditorSettings * settings = ruler_widget_get_editor_settings (self);
          g_return_val_if_fail (settings, false);

          /* get position of cursor */
          Position cursor_pos;
          ruler_widget_px_to_pos (self, x, &cursor_pos, F_PADDING);

          /* scroll down, zoom out */
          if (dy > 0)
            {
              ruler_widget_set_zoom_level (
                self, ruler_widget_get_zoom_level (self) / 1.3);
            }
          else /* scroll up, zoom in */
            {
              ruler_widget_set_zoom_level (
                self, ruler_widget_get_zoom_level (self) * 1.3);
            }

          int new_x = ruler_widget_pos_to_px (self, &cursor_pos, F_PADDING);

          /* refresh relevant widgets */
          if (self->type == TYPE (TIMELINE))
            {
              arranger_minimap_widget_refresh (MW_TIMELINE_MINIMAP);
            }

          /* also update hover x since we're using it here */
          self->hover_x = new_x;
        }
    }
  else /* else if not ctrl held */
    {
      /* scroll normally */
      if (modifier_type & GDK_SHIFT_MASK)
        {
          const int        scroll_amt = RW_SCROLL_SPEED;
          int              scroll_x = (int) dy;
          EditorSettings * settings = ruler_widget_get_editor_settings (self);
          editor_settings_append_scroll (
            settings, scroll_x * scroll_amt, 0, F_VALIDATE);
        }
    }

  return true;
}

static guint
ruler_tick_cb (
  GtkWidget *     widget,
  GtkTickCallback callback,
  gpointer        user_data,
  GDestroyNotify  notify)
{
  gtk_widget_queue_draw (widget);

  return 5;
}

static void
dispose (RulerWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  object_free_w_func_and_null (g_object_unref, self->layout_normal);
  object_free_w_func_and_null (g_object_unref, self->monospace_layout_small);
  object_free_w_func_and_null (g_object_unref, self->marker_layout);

  G_OBJECT_CLASS (ruler_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
ruler_widget_init (RulerWidget * self)
{
  self->popover_menu = GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  gtk_widget_set_hexpand (GTK_WIDGET (self), true);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  /* allow all buttons for drag */
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->drag), 0);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  self->click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (self->click), "pressed", G_CALLBACK (on_click_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->click));

  GtkGestureClick * right_mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_mp), "pressed", G_CALLBACK (on_right_click_pressed), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (right_mp));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "motion", G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (motion_controller));

  GtkEventControllerScroll * scroll_controller = GTK_EVENT_CONTROLLER_SCROLL (
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll", G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (scroll_controller));

  self->layout_normal = gtk_widget_create_pango_layout (GTK_WIDGET (self), NULL);
  PangoFontDescription * desc =
    pango_font_description_from_string ("Monospace 11");
  pango_layout_set_font_description (self->layout_normal, desc);
  pango_font_description_free (desc);

  self->monospace_layout_small =
    gtk_widget_create_pango_layout (GTK_WIDGET (self), NULL);
  desc = pango_font_description_from_string ("Monospace 6");
  pango_layout_set_font_description (self->monospace_layout_small, desc);
  pango_font_description_free (desc);

  self->marker_layout = gtk_widget_create_pango_layout (GTK_WIDGET (self), NULL);
  desc = pango_font_description_from_string ("7");
  pango_layout_set_font_description (self->marker_layout, desc);
  pango_font_description_free (desc);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) ruler_tick_cb, self, NULL);

  /* add action group for right click menu */
  GSimpleActionGroup * action_group = g_simple_action_group_new ();
  GAction * display_action = g_settings_create_action (S_UI, "ruler-display");
  g_action_map_add_action (G_ACTION_MAP (action_group), display_action);
  gtk_widget_insert_action_group (
    GTK_WIDGET (self), "ruler", G_ACTION_GROUP (action_group));

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
}

static void
ruler_widget_class_init (RulerWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = ruler_snapshot;
  gtk_widget_class_set_css_name (wklass, "ruler");

  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
