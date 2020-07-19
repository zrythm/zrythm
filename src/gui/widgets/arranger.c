/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm.h"
#include "actions/actions.h"
#include "actions/arranger_selections.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/control_port.h"
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/midi_region.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_playhead.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker_dialog.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_bot_box.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ArrangerWidget,
  arranger_widget,
  GTK_TYPE_DRAWING_AREA)

#define FOREACH_TYPE(func) \
  func (TIMELINE, timeline); \
  func (MIDI, midi); \
  func (AUDIO, audio); \
  func (AUTOMATION, automation); \
  func (MIDI_MODIFIER, midi_modifier); \
  func (CHORD, chord)

#define ACTION_IS(x) \
  (self->action == UI_OVERLAY_ACTION_##x)

#define TYPE(x) ARRANGER_WIDGET_TYPE_##x

#define TYPE_IS(x) \
  (self->type == TYPE (x))

#define SCROLL_PADDING 8.0

static void
draw_selections (
  ArrangerWidget * self,
  cairo_t *        cr,
  GdkRectangle *   rect)
{
  double offset_x, offset_y;
  offset_x =
    self->start_x + self->last_offset_x > 0 ?
    self->last_offset_x :
    1 - self->start_x;
  offset_y =
    self->start_y + self->last_offset_y > 0 ?
    self->last_offset_y :
    1 - self->start_y;

  /* if action is selecting and not selecting range
   * (in the case of timeline */
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_SELECTING:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      z_cairo_draw_selection (
        cr, self->start_x - rect->x,
        self->start_y - rect->y,
        offset_x, offset_y);
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      cairo_set_source_rgba (
        cr, 0.9, 0.9, 0.9, 1.0);
      cairo_set_line_width (cr, 2.0);
      cairo_move_to (
        cr, self->start_x -  rect->x,
        self->start_y - rect->y);
      cairo_line_to (
        cr,
        (self->start_x + self->last_offset_x) -
          rect->x,
        (self->start_y + self->last_offset_y) -
          rect->y);
      cairo_stroke (cr);
      break;
    default:
      break;
    }
}

/**
 * @param rect Arranger's rectangle.
 */
static void
draw_arranger_object (
  ArrangerWidget * self,
  ArrangerObject * obj,
  cairo_t *        cr,
  GdkRectangle *   rect)
{
  /* loop once or twice (2nd time for transient) */
  for (int i = 0;
       i < 1 +
         (arranger_object_should_orig_be_visible (
           obj) &&
          arranger_object_is_selected (obj));
       i++)
    {
      /* if looping 2nd time (transient) */
      if (i == 1)
        {
          g_return_if_fail (obj->transient);
          obj = obj->transient;
        }

      arranger_object_set_full_rectangle (
        obj, self);

      /* only draw if the object's rectangle is
       * hit by the drawable region */
      if (ui_rectangle_overlap (
            &obj->full_rect, rect))
        {
          arranger_object_draw (
            obj, self, cr, rect);
        }
    }
}

/**
 * Returns the playhead's x coordinate in absolute
 * coordinates.
 */
static int
get_playhead_px (
  ArrangerWidget * self)
{
  ZRegion * clip_editor_region =
    clip_editor_get_region (CLIP_EDITOR);

  /* get frames */
  long frames = 0;
  if (self->type == TYPE (TIMELINE))
    {
      frames = PLAYHEAD->frames;
    }
  else if (clip_editor_region)
    {
      ZRegion * r = NULL;

      if (self->type ==
            ARRANGER_WIDGET_TYPE_AUTOMATION)
        {
          /* for some reason hidden arrangers
           * try to call this */
          if (clip_editor_region->id.type !=
                REGION_TYPE_AUTOMATION)
            {
              /*g_warning (*/
                /*"%p clip editor region currently "*/
                /*"being changed", self);*/
              return 0;
            }

          AutomationTrack * at =
            region_get_automation_track (
              clip_editor_region);
          r =
            region_at_position (
              NULL, at, PLAYHEAD);
        }
      else
        {
          r =
            region_at_position (
              arranger_object_get_track (
                (ArrangerObject *)
                clip_editor_region),
              NULL, PLAYHEAD);
        }
      Position tmp;
      if (r)
        {
          ArrangerObject * obj =
            (ArrangerObject *) r;
          long region_local_frames =
            region_timeline_frames_to_local (
              r, PLAYHEAD->frames, 1);
          region_local_frames +=
            obj->pos.frames;
          position_from_frames (
            &tmp, region_local_frames);
          frames = tmp.frames;
        }
      else
        {
          frames = PLAYHEAD->frames;
        }
    }

  Position pos;
  position_from_frames (&pos, frames);
  return
    arranger_widget_pos_to_px (self, &pos, 1);
}

/**
 * @param rect Arranger draw rectangle.
 */
static void
draw_playhead (
  ArrangerWidget * self,
  cairo_t *        cr,
  GdkRectangle *   rect)
{
  int px = get_playhead_px (self);

  if (px >= rect->x && px <= rect->x + rect->width)
    {
      cairo_set_source_rgba (
        cr, 1, 0, 0, 1);
      switch (ui_get_detail_level ())
        {
        case UI_DETAIL_HIGH:
          cairo_rectangle (
            cr, (px - rect->x) - 1, 0, 2,
            rect->height);
          break;
        case UI_DETAIL_NORMAL:
        case UI_DETAIL_LOW:
          cairo_rectangle (
            cr, (int) (px - rect->x) - 1, 0, 2,
            (int) rect->height);
          break;
        }
      cairo_fill (cr);
      self->last_playhead_px = px;
    }
}

static void
draw_timeline_bg (
  ArrangerWidget * self,
  cairo_t *        cr,
  GdkRectangle *   rect)
{
  /* handle horizontal drawing for tracks */
  GtkWidget * tw_widget;
  gint track_start_offset;
  Track * track;
  TrackWidget * tw;
  int line_y, i, j;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      /* skip tracks in the other timeline (pinned/
       * non-pinned) */
      if (!track->visible ||
          (!self->is_pinned &&
           track->pinned) ||
          (self->is_pinned &&
           !track->pinned))
        continue;

      /* draw line below track */
      tw = track->widget;
      if (!GTK_IS_WIDGET (tw))
        continue;
      tw_widget = (GtkWidget *) tw;

      int full_track_height =
        track_get_full_visible_height (track);

      gtk_widget_translate_coordinates (
        tw_widget,
        GTK_WIDGET (
          self->is_pinned ?
            MW_TRACKLIST->pinned_box :
            MW_TRACKLIST->unpinned_box),
        0, 0, NULL, &track_start_offset);

      line_y =
        track_start_offset + full_track_height;

      if (line_y >= rect->y &&
          line_y < rect->y + rect->height)
        {
          cairo_set_source_rgb (
            self->cached_cr, 0.3, 0.3, 0.3);
          cairo_rectangle (
            cr, 0, (line_y - rect->y) - 1,
            rect->width, 2);
          cairo_fill (cr);
        }

      int total_height = track->main_height;

#define OFFSET_PLUS_TOTAL_HEIGHT \
  (track_start_offset + total_height)

      /* --- draw lanes --- */

      if (track->lanes_visible)
        {
          for (j = 0; j < track->num_lanes; j++)
            {
              TrackLane * lane = track->lanes[j];

              /* horizontal line above lane */
              if (OFFSET_PLUS_TOTAL_HEIGHT >
                    rect->y &&
                  OFFSET_PLUS_TOTAL_HEIGHT  <
                    rect->y + rect->height)
                {
                  z_cairo_draw_horizontal_line (
                    cr,
                    OFFSET_PLUS_TOTAL_HEIGHT -
                      rect->y,
                    0, rect->width, 0.5, 0.4);
                }

              total_height += lane->height;
            }
        }

      /* --- draw automation related stuff --- */

      /* skip tracks without visible automation */
      if (!track->automation_visible)
        continue;

      AutomationTracklist * atl =
        track_get_automation_tracklist (track);
      if (atl)
        {
          AutomationTrack * at;
          for (j = 0; j < atl->num_ats; j++)
            {
              at = atl->ats[j];

              if (!at->created || !at->visible)
                continue;

              /* horizontal line above automation
               * track */
              if (OFFSET_PLUS_TOTAL_HEIGHT >
                    rect->y &&
                  OFFSET_PLUS_TOTAL_HEIGHT  <
                    rect->y + rect->height)
                {
                  z_cairo_draw_horizontal_line (
                    cr,
                    OFFSET_PLUS_TOTAL_HEIGHT -
                      rect->y,
                    0, rect->width, 0.5, 0.2);
                }

              float normalized_val =
                automation_track_get_val_at_pos (
                  at, PLAYHEAD, true);
              Port * port =
                automation_track_get_port (at);
              AutomationPoint * ap =
                automation_track_get_ap_before_pos (
                  at, PLAYHEAD);
              if (!ap)
                {
                  normalized_val =
                    control_port_real_val_to_normalized (
                      port,
                      control_port_get_val (port));
                }

              int y_px =
                automation_track_get_y_px_from_normalized_val (
                  at,
                  normalized_val);

              /* line at current val */
              cairo_set_source_rgba (
                cr,
                track->color.red,
                track->color.green,
                track->color.blue,
                0.3);
              cairo_rectangle (
                cr, 0,
                (OFFSET_PLUS_TOTAL_HEIGHT + y_px) -
                  rect->y,
                rect->width, 1);
              cairo_fill (cr);

              /* show shade under the line */
              /*cairo_set_source_rgba (*/
                /*cr,*/
                /*track->color.red,*/
                /*track->color.green,*/
                /*track->color.blue,*/
                /*0.06);*/
              /*cairo_rectangle (*/
                /*cr,*/
                /*0,*/
                /*(OFFSET_PLUS_TOTAL_HEIGHT + y_px) -*/
                  /*rect->y,*/
                /*rect->width,*/
                /*at->height - y_px);*/
              /*cairo_fill (cr);*/

              total_height += at->height;
            }
        }
    }
}

static void
draw_borders (
  ArrangerWidget * self,
  cairo_t *        cr,
  int              x_from,
  int              x_to,
  double           y_offset)
{
  cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
  cairo_rectangle (
    cr, x_from, (int) y_offset, x_to - x_from, 0.5);
  cairo_fill (cr);
}

static void
draw_midi_bg (
  ArrangerWidget * self,
  cairo_t *        cr,
  GdkRectangle *   rect)
{
  /* px per key adjusted for border width */
  double adj_px_per_key =
    MW_PIANO_ROLL_KEYS->px_per_key + 1.0;
  /*double adj_total_key_px =*/
    /*MW_PIANO_ROLL->total_key_px + 126;*/

  /*handle horizontal drawing*/
  double y_offset;
  for (int i = 0; i < 128; i++)
    {
      y_offset =
        adj_px_per_key * i;
      /* if key is visible */
      if (y_offset > rect->y &&
          y_offset < (rect->y + rect->height))
        {
          draw_borders (
            self, cr, 0, rect->width,
            y_offset - rect->y);
          if (piano_roll_is_key_black (
                PIANO_ROLL->piano_descriptors[i].
                  value))
            {
              cairo_set_source_rgba (
                cr, 0, 0, 0, 0.2);
              cairo_rectangle (
                cr, 0,
                /* + 1 since the border is
                 * bottom */
                (int) ((y_offset - rect->y) + 1),
                rect->width, (int) adj_px_per_key);
              cairo_fill (cr);
            }
        }
      if ((PIANO_ROLL->drum_mode &&
          PIANO_ROLL->drum_descriptors[i].value ==
            MW_MIDI_ARRANGER->hovered_note) ||
          (!PIANO_ROLL->drum_mode &&
           PIANO_ROLL->piano_descriptors[i].value ==
             MW_MIDI_ARRANGER->hovered_note))
        {
          cairo_set_source_rgba (
            cr, 1, 1, 1, 0.06);
          cairo_rectangle (
            cr, 0,
            /* + 1 since the border is bottom */
            (y_offset - rect->y) + 1,
            rect->width, adj_px_per_key);
          cairo_fill (cr);
        }
    }
}

static void
draw_audio_bg (
  ArrangerWidget * self,
  cairo_t *        cr,
  GdkRectangle *   rect)
{
  ZRegion * ar =
    clip_editor_get_region (CLIP_EDITOR);
  if (ar->stretching)
    {
      arranger_widget_redraw_whole (self);
      return;
    }
  ArrangerObject * obj = (ArrangerObject *) ar;
  TrackLane * lane = region_get_lane (ar);
  Track * track =
    track_lane_get_track (lane);
  g_return_if_fail (lane);
  GdkRGBA * color = &track->color;
  cairo_set_source_rgba (
    cr, color->red + 0.3, color->green + 0.3,
    color->blue + 0.3, 0.9);
  cairo_set_line_width (cr, 1);

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  AudioClip * clip =
    AUDIO_POOL->clips[ar->pool_id];

  double local_start_x =
    (double) rect->x;
  double local_end_x =
    local_start_x +
    (double) rect->width;

  /* frames in the clip to start drawing from */
  long prev_frames =
    MAX (
      ui_px_to_frames_editor (local_start_x, 1) -
        obj->pos.frames,
      0);

  UiDetail detail = ui_get_detail_level ();
  double increment = 1;
  double width = 1;
  switch (detail)
    {
    case UI_DETAIL_HIGH:
      increment = 0.5;
      width = 1;
      break;
    case UI_DETAIL_NORMAL:
      increment = 1;
      width = 1;
      break;
    case UI_DETAIL_LOW:
      increment = 2;
      width = 2;
      break;
    }

  for (double i = local_start_x;
       i < local_end_x; i += increment)
    {
      long curr_frames =
        ui_px_to_frames_editor (i, 1) -
          obj->pos.frames;
      if (curr_frames < 0)
        continue;

      float min = 0.f, max = 0.f;
      for (long j = prev_frames;
           j < curr_frames; j++)
        {
          if (j >= (long) ar->num_frames)
            break;
          for (unsigned int k = 0;
               k < clip->channels; k++)
            {
              long index =
                j * (long) clip->channels + (long) k;
              g_return_if_fail (
                index >= 0 &&
                index <
                  (long)
                  (ar->num_frames *
                     clip->channels));
              float val =
                ar->frames[index];
              if (val > max)
                max = val;
              if (val < min)
                min = val;
            }
        }
#define DRAW_VLINE(cr,x,from_y,_height) \
  switch (detail) \
    { \
    case UI_DETAIL_HIGH: \
      cairo_rectangle ( \
        cr, x, from_y, \
        width, _height); \
      break; \
    case UI_DETAIL_NORMAL: \
    case UI_DETAIL_LOW: \
      cairo_rectangle ( \
        cr, (int) (x), (int) (from_y), \
        width, (int) _height); \
      break; \
    } \
  cairo_fill (cr)

      min = (min + 1.f) / 2.f; /* normallize */
      max = (max + 1.f) / 2.f; /* normalize */
      double from_y =
        MAX (
          (double) min * (double) height, 0.0) - rect->y;
      double draw_height =
        (MIN (
          (double) max * (double) height,
          (double) height) - rect->y) - from_y;
      DRAW_VLINE (
        cr,
        /* x */
        i - rect->x,
        /* from y */
        from_y,
        /* to y */
        draw_height);

      if (curr_frames >= clip->num_frames)
        break;

      prev_frames = curr_frames;
    }
#undef DRAW_VLINE
  cairo_fill (cr);
}

static gboolean
arranger_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  ArrangerWidget * self)
{
  /*g_message ("drawing arranger %p", self);*/
  RulerWidget * ruler =
    self->type == TYPE (TIMELINE) ?
    MW_RULER : EDITOR_RULER;
  if (ruler->px_per_bar < 2.0)
    return FALSE;

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (
    cr, &rect);

  if (self->redraw ||
      !gdk_rectangle_equal (
         &rect, &self->last_rect))
    {
      /*g_message (*/
        /*"redrawing arranger in rect: "*/
        /*"(%d, %d) width: %d height %d)",*/
        /*rect.x, rect.y, rect.width, rect.height);*/
      self->last_rect = rect;
      int i = 0;

      GtkStyleContext *context =
        gtk_widget_get_style_context (widget);

      z_cairo_reset_caches (
        &self->cached_cr,
        &self->cached_surface, rect.width,
        rect.height, cr);

      gtk_render_background (
        context, self->cached_cr, 0, 0,
        rect.width, rect.height);

      /* draw loop background */
      if (TRANSPORT->loop)
        {
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
          else
            {
              start_px =
                ui_pos_to_px_editor (
                  &TRANSPORT->loop_start_pos, 1);
              end_px =
                ui_pos_to_px_editor (
                  &TRANSPORT->loop_end_pos, 1);
            }
          cairo_set_source_rgba (
            self->cached_cr, 0, 0.9, 0.7, 0.08);
          cairo_set_line_width (
            self->cached_cr, 2);

          /* if transport loop start is within the
           * screen */
          if (start_px > rect.x &&
              start_px <= rect.x + rect.width)
            {
              /* draw the loop start line */
              double x =
                (start_px - rect.x) + 1.0;
              cairo_rectangle (
                self->cached_cr,
                (int) x, 0, 2, rect.height);
              cairo_fill (self->cached_cr);
            }
          /* if transport loop end is within the
           * screen */
          if (end_px > rect.x &&
              end_px < rect.x + rect.width)
            {
              double x =
                (end_px - rect.x) - 1.0;
              cairo_rectangle (
                self->cached_cr,
                (int) x, 0, 2, rect.height);
              cairo_fill (self->cached_cr);
            }

          /* draw transport loop area */
          cairo_set_source_rgba (
            self->cached_cr, 0, 0.9, 0.7, 0.02);
          double loop_start_local_x =
            MAX (0, start_px - rect.x);
          cairo_rectangle (
            self->cached_cr,
            (int) loop_start_local_x, 0,
            (int) (end_px - MAX (rect.x, start_px)),
            rect.height);
          cairo_fill (self->cached_cr);
        }

      /* --- handle vertical drawing --- */

      /* get sixteenth interval */
      int sixteenth_interval =
        ruler_widget_get_sixteenth_interval (
          ruler);

      /* get the beat interval */
      int beat_interval =
        ruler_widget_get_beat_interval (
          ruler);

      /* get the interval for bars */
      int bar_interval =
        (int)
        MAX ((RW_PX_TO_HIDE_BEATS) /
             (double) ruler->px_per_bar, 1.0);

      i = 0;
      double curr_px;
      while (
        (curr_px =
           ruler->px_per_bar * (i += bar_interval) +
             SPACE_BEFORE_START) <
         rect.x + rect.width)
        {
          if (curr_px < rect.x)
            continue;

          cairo_set_source_rgb (
            self->cached_cr, 0.3, 0.3, 0.3);
          double x = curr_px - rect.x;
          cairo_rectangle (
            self->cached_cr, (int) x, 0,
            1, rect.height);
          cairo_fill (self->cached_cr);
        }
      i = 0;
      if (beat_interval > 0)
        {
          while ((curr_px =
                  ruler->px_per_beat *
                    (i += beat_interval) +
                  SPACE_BEFORE_START) <
                 rect.x + rect.width)
            {
              if (curr_px < rect.x)
                continue;

              cairo_set_source_rgba (
                self->cached_cr, 0.25, 0.25, 0.25,
                0.6);
              double x = curr_px - rect.x;
              cairo_rectangle (
                self->cached_cr, (int) x, 0,
                1, rect.height);
              cairo_fill (self->cached_cr);
            }
        }
      i = 0;
      if (sixteenth_interval > 0)
        {
          while ((curr_px =
                  ruler->px_per_sixteenth *
                    (i += sixteenth_interval) +
                  SPACE_BEFORE_START) <
                 rect.x + rect.width)
            {
              if (curr_px < rect.x)
                continue;

              cairo_set_source_rgb (
                self->cached_cr, 0.2, 0.2, 0.2);
              cairo_set_line_width (
                self->cached_cr, 0.5);
              double x = curr_px - rect.x;
              cairo_move_to (
                self->cached_cr, x, 0);
              cairo_line_to (
                self->cached_cr, x, rect.height);
              cairo_stroke (self->cached_cr);
            }
        }

      /* draw range */
      if (TRANSPORT->has_range)
        {
          /* in order they appear */
          Position * range_first_pos,
                   * range_second_pos;
          if (position_is_before_or_equal (
                &TRANSPORT->range_1,
                &TRANSPORT->range_2))
            {
              range_first_pos = &TRANSPORT->range_1;
              range_second_pos =
                &TRANSPORT->range_2;
            }
          else
            {
              range_first_pos = &TRANSPORT->range_2;
              range_second_pos =
                &TRANSPORT->range_1;
            }

          int range_first_px, range_second_px;
          range_first_px =
            ui_pos_to_px_timeline (
              range_first_pos, 1);
          range_second_px =
            ui_pos_to_px_timeline (
              range_second_pos, 1);
          cairo_set_source_rgba (
            self->cached_cr, 0.3, 0.3, 0.3, 0.3);
          cairo_rectangle (
            self->cached_cr,
            MAX (0, range_first_px - rect.x), 0,
            range_second_px -
              MAX (rect.x, range_first_px),
            rect.height);
          cairo_fill (self->cached_cr);
          cairo_set_source_rgba (
            self->cached_cr, 0.8, 0.8, 0.8, 0.4);
          cairo_set_line_width (self->cached_cr, 2);

          /* if start is within the screen */
          if (range_first_px > rect.x &&
              range_first_px <= rect.x + rect.width)
            {
              /* draw the start line */
              double x =
                (range_first_px - rect.x) + 1.0;
              cairo_move_to (
                self->cached_cr, x, 0);
              cairo_line_to (
                self->cached_cr, x, rect.height);
              cairo_stroke (self->cached_cr);
            }
          /* if end is within the screen */
          if (range_second_px > rect.x &&
              range_second_px < rect.x + rect.width)
            {
              double x =
                (range_second_px - rect.x) - 1.0;
              cairo_move_to (
                self->cached_cr, x, 0);
              cairo_line_to (
                self->cached_cr, x, rect.height);
              cairo_stroke (self->cached_cr);
            }
        }

      if (self->type == TYPE (TIMELINE))
        {
          draw_timeline_bg (
            self, self->cached_cr, &rect);
        }
      else if (self->type == TYPE (MIDI))
        {
          draw_midi_bg (
            self, self->cached_cr, &rect);
        }
      else if (self->type == TYPE (AUDIO))
        {
          draw_audio_bg (
            self, self->cached_cr, &rect);
        }

      /* draw each arranger object */
      ArrangerObject * objs[2000];
      int num_objs;
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_ALL, &rect,
        objs, &num_objs);

      /*g_message (*/
        /*"objects found: %d (is pinned %d)",*/
        /*num_objs, self->is_pinned);*/
      /* note: these are only project objects */
      for (int j = 0; j < num_objs; j++)
        {
          draw_arranger_object (
            self, objs[j], self->cached_cr,
            &rect);
        }

      /* draw selections */
      draw_selections (
        self, self->cached_cr, &rect);

      draw_playhead (self, self->cached_cr, &rect);

      self->redraw--;
    }

  cairo_set_source_surface (
    cr, self->cached_surface, rect.x, rect.y);
  cairo_paint (cr);

  return FALSE;
}

/**
 * Sets the cursor on the arranger and all of its
 * children.
 */
void
arranger_widget_set_cursor (
  ArrangerWidget * self,
  ArrangerCursor   cursor)
{
#define SET_X_CURSOR(x) \
  ui_set_##x##_cursor (self); \

#define SET_CURSOR_FROM_NAME(name) \
  ui_set_cursor_from_name ( \
    GTK_WIDGET (self), name); \

  switch (cursor)
    {
    case ARRANGER_CURSOR_SELECT:
      SET_X_CURSOR (pointer);
      break;
    case ARRANGER_CURSOR_EDIT:
      SET_X_CURSOR (pencil);
      break;
    case ARRANGER_CURSOR_CUT:
      SET_X_CURSOR (cut_clip);
      break;
    case ARRANGER_CURSOR_RAMP:
      SET_X_CURSOR (line);
      break;
    case ARRANGER_CURSOR_ERASER:
      SET_X_CURSOR (eraser);
      break;
    case ARRANGER_CURSOR_AUDITION:
      SET_X_CURSOR (speaker);
      break;
    case ARRANGER_CURSOR_GRAB:
      SET_X_CURSOR (hand);
      break;
    case ARRANGER_CURSOR_GRABBING:
      SET_CURSOR_FROM_NAME ("grabbing");
      break;
    case ARRANGER_CURSOR_GRABBING_COPY:
      SET_CURSOR_FROM_NAME ("copy");
      break;
    case ARRANGER_CURSOR_GRABBING_LINK:
      SET_CURSOR_FROM_NAME ("link");
      break;
    case ARRANGER_CURSOR_RESIZING_L:
      SET_X_CURSOR (left_resize);
      break;
    case ARRANGER_CURSOR_STRETCHING_L:
      SET_X_CURSOR (left_stretch);
      break;
    case ARRANGER_CURSOR_RESIZING_L_LOOP:
      SET_X_CURSOR (left_resize_loop);
      break;
    case ARRANGER_CURSOR_RESIZING_R:
      SET_X_CURSOR (right_resize);
      break;
    case ARRANGER_CURSOR_STRETCHING_R:
      SET_X_CURSOR (right_stretch);
      break;
    case ARRANGER_CURSOR_RESIZING_R_LOOP:
      SET_X_CURSOR (right_resize_loop);
      break;
    case ARRANGER_CURSOR_RESIZING_UP:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ARRANGER_CURSOR_RESIZING_UP_FADE_IN:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ARRANGER_CURSOR_RESIZING_UP_FADE_OUT:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ARRANGER_CURSOR_RANGE:
      SET_CURSOR_FROM_NAME ("text");
      break;
    case ARRANGER_CURSOR_FADE_IN:
      SET_X_CURSOR (fade_in);
      break;
    case ARRANGER_CURSOR_FADE_OUT:
      SET_X_CURSOR (fade_out);
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

SnapGrid *
arranger_widget_get_snap_grid (
  ArrangerWidget * self)
{
  if (self == MW_MIDI_MODIFIER_ARRANGER ||
      self == MW_MIDI_ARRANGER ||
      self == MW_AUTOMATION_ARRANGER ||
      self == MW_AUDIO_ARRANGER ||
      self == MW_CHORD_ARRANGER)
    {
      return SNAP_GRID_MIDI;
    }
  else if (self == MW_TIMELINE ||
           self == MW_PINNED_TIMELINE)
    {
      return SNAP_GRID_TIMELINE;
    }
  g_return_val_if_reached (NULL);
}

#if 0
/**
 * Returns the number of regions inside the given
 * editor arranger.
 */
static int
get_regions_in_editor_rect (
  ArrangerWidget * self,
  GdkRectangle *   rect,
  ZRegion **       regions)
{
  return
    editor_ruler_get_regions_in_range (
      EDITOR_RULER, rect->x, rect->x + rect->width,
      regions);
}
#endif

/**
 * Adds the object to the array if it or its
 * transient overlaps with the rectangle, or with
 * \ref x \ref y if \ref rect is NULL.
 *
 * When rect is NULL, this is a special case for
 * automation points. The object will only
 * be added if the cursor is on the automation
 * point or within n px from the curve.
 *
 * @return Whether the object was added or not.
 */
static bool
add_object_if_overlap (
  ArrangerWidget *   self,
  GdkRectangle *     rect,
  double             x,
  double             y,
  ArrangerObject **  array,
  int *              array_size,
  ArrangerObject *   obj)
{
  if (obj->deleted_temporarily)
    {
      return false;
    }

  arranger_object_set_full_rectangle (
    obj, self);
  bool is_same_arranger =
    arranger_object_get_arranger (obj) == self;
  bool add = false;
  if (rect)
    {
      if (is_same_arranger &&
          (ui_rectangle_overlap (
             &obj->full_rect, rect) ||
           /* also check original (transient) */
           (arranger_object_should_orig_be_visible (
              obj) &&
            obj->transient &&
            ui_rectangle_overlap (
              &obj->transient->full_rect, rect))))
        {
          add = true;
        }
    }
  else if (
    is_same_arranger &&
    (ui_is_point_in_rect_hit (
      &obj->full_rect, true, true, x, y,
      0, 0) ||
    /* also check original (transient) */
    (arranger_object_should_orig_be_visible (
       obj) &&
     obj->transient &&
     ui_is_point_in_rect_hit (
       &obj->transient->full_rect, true,
       true, x, y, 0, 0))))
    {
      /* object to check for automation point
       * curve cross (either main object or
       * transient) */
      ArrangerObject * obj_to_check =
        (arranger_object_should_orig_be_visible (
          obj) && obj->transient &&
         ui_is_point_in_rect_hit (
           &obj->transient->full_rect, true,
           true, x, y, 0, 0)) ?
        obj->transient : obj;

      /** handle special case for automation
       * points */
      if (obj->type ==
            ARRANGER_OBJECT_TYPE_AUTOMATION_POINT &&
          !automation_point_is_point_hit (
            (AutomationPoint *) obj_to_check,
            x, y) &&
          !automation_point_is_curve_hit (
            (AutomationPoint *) obj_to_check,
            x, y, 16.0))
        {
          return false;
        }

      add = true;
    }

  if (add)
    {
      array[*array_size] = obj;
      (*array_size)++;
    }

  return add;
}

/**
 * Fills in the given array with the
 * ArrangerObject's of the given type that appear
 * in the given rect, or at the given coords if
 * \ref rect is NULL.
 *
 * @param rect The rectangle to search in.
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 * @param array The array to fill.
 * @param array_size The size of the array to fill.
 */
static void
get_hit_objects (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  GdkRectangle *     rect,
  double             x,
  double             y,
  ArrangerObject **  array,
  int *              array_size)
{
  g_return_if_fail (self && array);

  *array_size = 0;
  ArrangerObject * obj = NULL;

  switch (self->type)
    {
    case TYPE (TIMELINE):
      if (type != ARRANGER_OBJECT_TYPE_ALL &&
          type != ARRANGER_OBJECT_TYPE_REGION &&
          type != ARRANGER_OBJECT_TYPE_MARKER &&
          type != ARRANGER_OBJECT_TYPE_SCALE_OBJECT)
        break;

      /* add overlapping scales */
      if (type == ARRANGER_OBJECT_TYPE_ALL ||
          type ==
            ARRANGER_OBJECT_TYPE_SCALE_OBJECT)
        {
          for (int i = 0;
               i < P_CHORD_TRACK->num_scales; i++)
            {
              obj =
                (ArrangerObject *)
                P_CHORD_TRACK->scales[i];
              add_object_if_overlap (
                self, rect, x, y, array,
                array_size, obj);
            }
        }

      /* add overlapping regions */
      if (type == ARRANGER_OBJECT_TYPE_ALL ||
          type == ARRANGER_OBJECT_TYPE_REGION)
        {
          /* midi and audio regions */
          for (int i = 0;
               i < TRACKLIST->num_tracks;
               i++)
            {
              Track * track =
                TRACKLIST->tracks[i];

              for (int j = 0;
                   j < track->num_lanes; j++)
                {
                  TrackLane * lane =
                    track->lanes[j];
                  for (int k = 0;
                       k < lane->num_regions;
                       k++)
                    {
                      ZRegion *r =
                        lane->regions[k];
                      obj =
                        (ArrangerObject *) r;
                      bool ret =
                        add_object_if_overlap (
                          self, rect, x, y, array,
                          array_size, obj);
                      if (!ret)
                        {
                          /* check lanes */
                          if (!track->
                                lanes_visible)
                            continue;
                          GdkRectangle lane_rect;
                          region_get_lane_full_rect (
                            lane->regions[k],
                            &lane_rect);
                          if (((rect &&
                               ui_rectangle_overlap (
                                &lane_rect,
                                rect)) ||
                               (!rect &&
                                ui_is_point_in_rect_hit (
                                  &lane_rect,
                                  true, true, x, y,
                                  0, 0))) &&
                              arranger_object_get_arranger (obj) ==  self &&
                              !obj->deleted_temporarily)
                            {
                              array[*array_size] =
                                obj;
                              (*array_size)++;
                            }
                        }
                    }
                }

              /* chord regions */
              for (int j = 0;
                   j < P_CHORD_TRACK->num_chord_regions;
                   j++)
                {
                  ZRegion * cr =
                    P_CHORD_TRACK->chord_regions[j];
                  obj =
                    (ArrangerObject *) cr;
                  add_object_if_overlap (
                    self, rect, x, y, array,
                    array_size, obj);
                }

              /* automation regions */
              AutomationTracklist * atl =
                track_get_automation_tracklist (
                  track);
              if (atl &&
                  track->automation_visible)
                {
                  for (int j = 0;
                       j < atl->num_ats;
                       j++)
                    {
                      AutomationTrack * at =
                        atl->ats[j];

                      if (!at->visible)
                        continue;

                      for (int k = 0;
                           k < at->num_regions;
                           k++)
                        {
                          obj =
                            (ArrangerObject *)
                            at->regions[k];
                          add_object_if_overlap (
                            self, rect, x, y, array,
                            array_size, obj);
                        }
                    }
                }
            }
        }

      /* add overlapping scales */
      if (type == ARRANGER_OBJECT_TYPE_ALL ||
          type == ARRANGER_OBJECT_TYPE_SCALE_OBJECT)
        {
          for (int j = 0;
               j < P_CHORD_TRACK->num_scales;
               j++)
            {
              ScaleObject * scale =
                P_CHORD_TRACK->scales[j];
              obj =
                (ArrangerObject *) scale;
              add_object_if_overlap (
                self, rect, x, y, array,
                array_size, obj);
            }
        }

      /* add overlapping markers */
      if (type == ARRANGER_OBJECT_TYPE_ALL ||
          type == ARRANGER_OBJECT_TYPE_MARKER)
        {
          for (int j = 0;
               j < P_MARKER_TRACK->num_markers;
               j++)
            {
              Marker * marker =
                P_MARKER_TRACK->markers[j];
              obj =
                (ArrangerObject *) marker;
              add_object_if_overlap (
                self, rect, x, y, array,
                array_size, obj);
            }
        }
      break;
    case TYPE (MIDI):
      /* add overlapping midi notes */
      if (type == ARRANGER_OBJECT_TYPE_ALL ||
          type == ARRANGER_OBJECT_TYPE_MIDI_NOTE)
        {
          ZRegion * r =
            clip_editor_get_region (CLIP_EDITOR);
          if (!r)
            break;

          for (int i = 0; i < r->num_midi_notes;
               i++)
            {
              MidiNote * mn = r->midi_notes[i];
              obj =
                (ArrangerObject *)
                mn;
              add_object_if_overlap (
                self, rect, x, y, array,
                array_size, obj);
            }
        }
      break;
    case TYPE (MIDI_MODIFIER):
      /* add overlapping midi notes */
      if (type == ARRANGER_OBJECT_TYPE_ALL ||
          type == ARRANGER_OBJECT_TYPE_VELOCITY)
        {
          ZRegion * r =
            clip_editor_get_region (CLIP_EDITOR);
          if (!r)
            break;

          for (int i = 0; i < r->num_midi_notes;
               i++)
            {
              MidiNote * mn = r->midi_notes[i];
              Velocity * vel = mn->vel;
              obj =
                (ArrangerObject *) vel;
              add_object_if_overlap (
                self, rect, x, y, array,
                array_size, obj);
            }
        }
      break;
    case TYPE (CHORD):
      /* add overlapping midi notes */
      if (type == ARRANGER_OBJECT_TYPE_ALL ||
          type == ARRANGER_OBJECT_TYPE_CHORD_OBJECT)
        {
          ZRegion * r =
            clip_editor_get_region (CLIP_EDITOR);
          if (!r)
            break;

          for (int i = 0; i < r->num_chord_objects;
               i++)
            {
              ChordObject * co =
                r->chord_objects[i];
              obj =
                (ArrangerObject *) co;
              g_return_if_fail (
                co->chord_index <
                CHORD_EDITOR->num_chords);
              add_object_if_overlap (
                self, rect, x, y, array,
                array_size, obj);
            }
        }
      break;
    case TYPE (AUTOMATION):
      /* add overlapping midi notes */
      if (type == ARRANGER_OBJECT_TYPE_ALL ||
          type == ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
        {
          ZRegion * r =
            clip_editor_get_region (CLIP_EDITOR);
          if (!r)
            break;

          for (int i = 0; i < r->num_aps; i++)
            {
              AutomationPoint * ap =  r->aps[i];
              obj = (ArrangerObject *) ap;
              add_object_if_overlap (
                self, rect, x, y, array,
                array_size, obj);
            }
        }
      break;
    case TYPE (AUDIO):
      /* no objects in audio arranger yet */
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

/**
 * Fills in the given array with the ArrangerObject's
 * of the given type that appear in the given
 * range.
 *
 * @param rect The rectangle to search in.
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 * @param array The array to fill.
 * @param array_size The size of the array to fill.
 */
void
arranger_widget_get_hit_objects_in_rect (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  GdkRectangle *     rect,
  ArrangerObject **  array,
  int *              array_size)
{
  get_hit_objects (
    self, type, rect, 0, 0, array, array_size);
}

/**
 * Fills in the given array with the ArrangerObject's
 * of the given type that appear in the given
 * ranger.
 *
 * @param x Global x.
 * @param y Global y.
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 * @param array The array to fill.
 * @param array_size The size of the array to fill.
 */
void
arranger_widget_get_hit_objects_at_point (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  double             x,
  double             y,
  ArrangerObject **  array,
  int *              array_size)
{
  get_hit_objects (
    self, type, NULL, x, y, array, array_size);
}

/**
 * Returns if the arranger is in a moving-related
 * operation or starting a moving-related operation.
 *
 * Useful to know if we need transient widgets or
 * not.
 */
bool
arranger_widget_is_in_moving_operation (
  ArrangerWidget * self)
{
  if (self->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING ||
      self->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING_COPY ||
      self->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING_LINK ||
      self->action ==
        UI_OVERLAY_ACTION_MOVING ||
      self->action ==
        UI_OVERLAY_ACTION_MOVING_COPY ||
      self->action ==
        UI_OVERLAY_ACTION_MOVING_LINK)
    return true;

  return false;
}

/**
 * Moves the ArrangerSelections by the given
 * amount of ticks.
 *
 * @param ticks_diff Ticks to move by.
 * @param copy_moving 1 if copy-moving.
 */
static void
move_items_x (
  ArrangerWidget * self,
  const double     ticks_diff)
{
  ArrangerSelections * sel =
    arranger_widget_get_selections (self);
  arranger_selections_add_ticks (
    sel, ticks_diff);

  if (sel->type ==
        ARRANGER_SELECTIONS_TYPE_AUTOMATION)
    {
      /* re-sort the automation region */
      ZRegion * region =
        clip_editor_get_region (CLIP_EDITOR);
      g_return_if_fail (region);
      automation_region_force_sort (region);
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_IN_TRANSIT, sel);
}

/**
 * Gets the float value at the given Y coordinate
 * relative to the automation arranger.
 */
static float
get_fvalue_at_y (
  ArrangerWidget * self,
  double           y)
{
  float height =
    (float)
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  g_return_val_if_fail (
    region &&
      region->id.type == REGION_TYPE_AUTOMATION,
    -1.f);
  AutomationTrack * at =
    region_get_automation_track (region);

  /* get ratio from widget */
  float widget_value = height - (float) y;
  float widget_ratio =
    CLAMP (
      widget_value / height,
      0.f, 1.f);
  Port * port = automation_track_get_port (at);
  float automatable_value =
    control_port_normalized_val_to_real (
      port, widget_ratio);

  return automatable_value;
}

static void
move_items_y (
  ArrangerWidget * self,
  double           offset_y)
{
  switch (self->type)
    {
    case TYPE (AUTOMATION):
      if (AUTOMATION_SELECTIONS->
            num_automation_points)
        {
          double offset_y_normalized =
            - offset_y /
            (double)
            gtk_widget_get_allocated_height (
              GTK_WIDGET (self));
          g_warn_if_fail (self->sel_at_start);
          (void) get_fvalue_at_y;
          for (int i = 0;
               i < AUTOMATION_SELECTIONS->
                 num_automation_points; i++)
            {
              AutomationPoint * ap =
                AUTOMATION_SELECTIONS->
                  automation_points[i];
              AutomationSelections * automation_sel =
                (AutomationSelections *)
                self->sel_at_start;
              AutomationPoint * start_ap =
                automation_sel->
                  automation_points[i];

              automation_point_set_fvalue (
                ap,
                start_ap->normalized_val +
                  (float) offset_y_normalized,
                true);
            }
          ArrangerObject * start_ap_obj =
            self->start_object;
          g_return_if_fail (start_ap_obj);
          /*arranger_object_widget_update_tooltip (*/
            /*Z_ARRANGER_OBJECT_WIDGET (*/
              /*start_ap_obj->widget), 1);*/
        }
      break;
    case TYPE (TIMELINE):
      {
        /* get old track, track where last change
         * happened, and new track */
        Track * track =
          timeline_arranger_widget_get_track_at_y (
          self, self->start_y + offset_y);
        Track * old_track =
          timeline_arranger_widget_get_track_at_y (
          self, self->start_y);
        Track * last_track =
          tracklist_get_visible_track_after_delta (
            TRACKLIST, old_track,
            self->visible_track_diff);

        /* TODO automations and other lanes */
        TrackLane * lane =
          timeline_arranger_widget_get_track_lane_at_y (
          self, self->start_y + offset_y);
        TrackLane * old_lane =
          timeline_arranger_widget_get_track_lane_at_y (
          self, self->start_y);
        TrackLane * last_lane = NULL;
        if (old_lane)
          {
            Track * old_lane_track =
              track_lane_get_track (old_lane);
            last_lane =
              old_lane_track->lanes[
                old_lane->pos + self->lane_diff];
          }

        /* if new track is equal, move lanes or
         * automation lanes */
        if (track && last_track &&
            track == last_track &&
            self->visible_track_diff == 0 &&
            old_lane && lane && last_lane)
          {
            int cur_diff =
              lane->pos - old_lane->pos;
            int delta =
              lane->pos - last_lane->pos;
            if (delta != 0)
              {
                int moved =
                  timeline_arranger_move_regions_to_new_lanes (
                    self, delta);
                if (moved)
                  {
                    self->lane_diff =
                      cur_diff;
                  }
              }
          }
        /* otherwise move tracks */
        else if (track && last_track && old_track &&
                 track != last_track)
          {
            int cur_diff =
              tracklist_get_visible_track_diff (
                TRACKLIST, old_track, track);
            int delta =
              tracklist_get_visible_track_diff (
                TRACKLIST, last_track, track);
            if (delta != 0)
              {
                int moved =
                  timeline_arranger_move_regions_to_new_tracks (
                    self, delta);
                if (moved)
                  {
                    self->visible_track_diff =
                      cur_diff;
                  }
              }
          }
      }
      break;
    case TYPE (MIDI):
      {
        int y_delta;
        /* first note selected */
        int first_note_selected =
           ((MidiNote *) self->start_object)->val;
        /* note at cursor */
        int note_at_cursor =
          piano_roll_keys_widget_get_key_from_y (
            MW_PIANO_ROLL_KEYS,
            self->start_y + offset_y);

        y_delta = note_at_cursor - first_note_selected;
        y_delta =
          midi_arranger_calc_deltamax_for_note_movement (y_delta);

        for (int i = 0;
             i < MA_SELECTIONS->num_midi_notes; i++)
          {
            MidiNote * midi_note =
              MA_SELECTIONS->midi_notes[i];
            /*ArrangerObject * mn_obj =*/
              /*(ArrangerObject *) midi_note;*/
            midi_note_set_val (
              midi_note,
              (midi_byte_t)
                ((int) midi_note->val + y_delta));
            /*if (Z_IS_ARRANGER_OBJECT_WIDGET (*/
                  /*mn_obj->widget))*/
              /*{*/
                /*arranger_object_widget_update_tooltip (*/
                  /*Z_ARRANGER_OBJECT_WIDGET (*/
                    /*mn_obj->widget), 0);*/
              /*}*/
          }

        /*midi_arranger_listen_notes (*/
          /*self, 1);*/
      }
      break;
    default:
      break;
    }
}

static void
select_all_timeline (
  ArrangerWidget * self,
  int              select)
{
  int i,j,k;

  /* select everything else */
  ZRegion * r;
  Track * track;
  AutomationTrack * at;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (!track->visible)
        continue;

      AutomationTracklist * atl =
        track_get_automation_tracklist (
          track);

      if (track->lanes_visible)
        {
          TrackLane * lane;
          for (j = 0; j < track->num_lanes; j++)
            {
              lane = track->lanes[j];

              for (k = 0;
                   k < lane->num_regions; k++)
                {
                  r = lane->regions[k];
                  arranger_object_select (
                    (ArrangerObject *) r,
                    select, F_APPEND);
                }
            }
        }

      if (!track->automation_visible)
        continue;

      for (j = 0; j < atl->num_ats; j++)
        {
          at = atl->ats[j];

          if (!at->created ||
              !at->visible)
            continue;

          for (k = 0; k < at->num_regions; k++)
            {
              r = at->regions[k];
              arranger_object_select (
                (ArrangerObject *) r,
                select, F_APPEND);
            }
        }
    }

  /* select scales */
  ScaleObject * scale;
  for (i = 0;
       i < P_CHORD_TRACK->num_scales; i++)
    {
      scale = P_CHORD_TRACK->scales[i];
      arranger_object_select (
        (ArrangerObject *) scale,
        select, F_APPEND);
    }

  /**
   * Deselect range if deselecting all.
   */
  if (!select)
    {
      transport_set_has_range (TRANSPORT, false);
    }
}

static void
select_all_midi (
  ArrangerWidget *  self,
  int               select)
{
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    return;

  /* select midi notes */
  for (int i = 0; i < region->num_midi_notes; i++)
    {
      MidiNote * midi_note = region->midi_notes[i];
      arranger_object_select (
        (ArrangerObject *) midi_note,
        select, F_APPEND);
    }
  /*arranger_widget_refresh(&self->parent_instance);*/
}

static void
select_all_chord (
  ArrangerWidget *  self,
  int                    select)
{
  /* select everything else */
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);
  ChordObject * chord;
  for (int i = 0; i < r->num_chord_objects; i++)
    {
      chord = r->chord_objects[i];
      arranger_object_select (
        (ArrangerObject *) chord, select,
        F_APPEND);
    }
}

static void
select_all_midi_modifier (
  ArrangerWidget *  self,
  int                           select)
{
  /* TODO */
}

static void
select_all_audio (
  ArrangerWidget *  self,
  int                    select)
{
  /* TODO */
}

static void
select_all_automation (
  ArrangerWidget *  self,
  int                         select)
{
  int i;

  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);

  /* select everything else */
  AutomationPoint * ap;
  for (i = 0; i < region->num_aps; i++)
    {
      ap = region->aps[i];
      arranger_object_select (
        (ArrangerObject *) ap, select,
        F_APPEND);
    }

  /**
   * Deselect range if deselecting all.
   */
  if (!select)
    {
      transport_set_has_range (TRANSPORT, false);
    }
}

void
arranger_widget_select_all (
  ArrangerWidget *  self,
  int               select)
{
#define SELECT_ALL(cc,sc) \
  case TYPE (cc): \
    { \
      ArrangerSelections * sel = \
        arranger_widget_get_selections ( \
          (ArrangerWidget *) self); \
      arranger_selections_clear (sel); \
      select_all_##sc ( \
        self, select); \
    } \
    break

  switch (self->type)
    {
      SELECT_ALL (TIMELINE, timeline);
      SELECT_ALL (MIDI, midi);
      SELECT_ALL (MIDI_MODIFIER, midi_modifier);
      SELECT_ALL (AUTOMATION, automation);
      SELECT_ALL (AUDIO, audio);
      SELECT_ALL (CHORD, chord);
    }

#undef SELECT_ALL
}

static void
show_context_menu_audio (
  ArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem =
    gtk_menu_item_new_with_label("Do something");

  gtk_menu_shell_append (
    GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
show_context_menu_chord (
  ArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
}

static void
show_context_menu_midi_modifier (
  ArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
}

static void
show_context_menu (
  ArrangerWidget * self,
  gdouble          x,
  gdouble          y)
{
  switch (self->type)
    {
    case TYPE (TIMELINE):
      timeline_arranger_widget_show_context_menu (
        self, x, y);
      break;
    case TYPE (MIDI):
      midi_arranger_show_context_menu (self, x, y);
      break;
    case TYPE (MIDI_MODIFIER):
      show_context_menu_midi_modifier (self, x, y);
      break;
    case TYPE (CHORD):
      show_context_menu_chord (self, x, y);
      break;
    case TYPE (AUTOMATION):
      automation_arranger_widget_show_context_menu (
        self, x, y);
      break;
    case TYPE (AUDIO):
      show_context_menu_audio (self, x, y);
      break;
    }
}

static void
on_right_click (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ArrangerWidget *      self)
{
  if (n_press != 1)
    return;

  MAIN_WINDOW->last_focused = GTK_WIDGET (self);

  /* if object clicked and object is unselected,
   * select it */
  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_ALL, x, y);
  if (obj)
    {
      if (!arranger_object_is_selected (obj))
        {
          arranger_object_select (
            obj, F_SELECT, F_NO_APPEND);
        }
    }

  show_context_menu (self, x, y);
}

static void
auto_scroll (
  ArrangerWidget * self,
  int              x,
  int              y)
{
  /* figure out if we should scroll */
  int scroll_h = 0, scroll_v = 0;
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_MOVING:
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_SELECTING:
    case UI_OVERLAY_ACTION_RAMPING:
      scroll_h = 1;
      scroll_v = 1;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_RESIZING_L:
    case UI_OVERLAY_ACTION_STRETCHING_L:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
    case UI_OVERLAY_ACTION_STRETCHING_R:
    case UI_OVERLAY_ACTION_AUTOFILLING:
    case UI_OVERLAY_ACTION_AUDITIONING:
      scroll_h = 1;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      scroll_v = 1;
      break;
    default:
      break;
    }

  if (!scroll_h && !scroll_v)
    return;

  GtkScrolledWindow *scroll =
    arranger_widget_get_scrolled_window (self);
  g_return_if_fail (scroll);
  int h_scroll_speed = 20;
  int v_scroll_speed = 10;
  int border_distance = 5;
  int scroll_width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (scroll));
  int scroll_height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (scroll));
  GtkAdjustment *hadj =
    gtk_scrolled_window_get_hadjustment (
      GTK_SCROLLED_WINDOW (scroll));
  GtkAdjustment *vadj =
    gtk_scrolled_window_get_vadjustment (
      GTK_SCROLLED_WINDOW (scroll));
  int v_delta = 0;
  int h_delta = 0;
  int adj_x =
    (int)
    gtk_adjustment_get_value (hadj);
  int adj_y =
    (int)
    gtk_adjustment_get_value (vadj);
  if (y + border_distance
        >= adj_y + scroll_height)
    {
      v_delta = v_scroll_speed;
    }
  else if (y - border_distance <= adj_y)
    {
      v_delta = - v_scroll_speed;
    }
  if (x + border_distance >= adj_x + scroll_width)
    {
      h_delta = h_scroll_speed;
    }
  else if (x - border_distance <= adj_x)
    {
      h_delta = - h_scroll_speed;
    }
  if (h_delta != 0 && scroll_h)
  {
    gtk_adjustment_set_value (
      hadj,
      gtk_adjustment_get_value (hadj) + h_delta);
  }
  if (v_delta != 0 && scroll_v)
  {
    gtk_adjustment_set_value (
      vadj,
      gtk_adjustment_get_value (vadj) + v_delta);
  }

  return;
}

/**
 * Called from MainWindowWidget because the
 * events don't reach here.
 */
gboolean
arranger_widget_on_key_release (
  GtkWidget *widget,
  GdkEventKey *event,
  ArrangerWidget * self)
{
  self->key_is_pressed = 0;

  const guint keyval = event->keyval;

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      self->ctrl_held = 0;
    }

  if (z_gtk_keyval_is_shift (keyval))
    {
      self->shift_held = 0;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      self->alt_held = 0;
    }

  if (ACTION_IS (STARTING_MOVING))
    {
      if (self->alt_held && self->can_link)
        self->action =
          UI_OVERLAY_ACTION_MOVING_LINK;
      else if (self->ctrl_held)
        self->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        self->action =
          UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (MOVING) &&
           self->alt_held &&
           self->can_link)
    {
      self->action =
        UI_OVERLAY_ACTION_MOVING_LINK;
    }
  else if (ACTION_IS (MOVING) &&
           self->ctrl_held)
    {
      self->action =
        UI_OVERLAY_ACTION_MOVING_COPY;
    }
  else if (ACTION_IS (MOVING_LINK) &&
           !self->alt_held &&
           self->can_link)
    {
      self->action =
        self->ctrl_held ?
        UI_OVERLAY_ACTION_MOVING_COPY :
        UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (MOVING_COPY) &&
           !self->ctrl_held)
    {
      self->action =
        UI_OVERLAY_ACTION_MOVING;
    }

  if (self->type == TYPE (TIMELINE))
    {
      timeline_arranger_widget_set_cut_lines_visible (
        self);
    }

  /*arranger_widget_update_visibility (self);*/

  arranger_widget_refresh_cursor (
    self);

  return TRUE;
}

/**
 * Called from MainWindowWidget because some
 * events don't reach here.
 */
gboolean
arranger_widget_on_key_action (
  GtkWidget *widget,
  GdkEventKey *event,
  ArrangerWidget * self)
{
  const guint keyval = event->keyval;

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      self->ctrl_held = 1;
    }

  if (z_gtk_keyval_is_shift (keyval))
    {
      self->shift_held = 1;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      self->alt_held = 1;
    }

  if (ACTION_IS (STARTING_MOVING))
    {
      if (self->ctrl_held)
        self->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        self->action =
          UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (MOVING) &&
           self->alt_held &&
           self->can_link)
    {
      self->action =
        UI_OVERLAY_ACTION_MOVING_LINK;
    }
  else if (ACTION_IS (MOVING) && self->ctrl_held)
    {
      self->action =
        UI_OVERLAY_ACTION_MOVING_COPY;
    }
  else if (ACTION_IS (MOVING_LINK) &&
           !self->alt_held)
    {
      self->action =
        self->ctrl_held ?
        UI_OVERLAY_ACTION_MOVING_COPY :
        UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (MOVING_COPY) &&
           !self->ctrl_held)
    {
      self->action =
        UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (NONE))
    {
      ArrangerSelections * sel =
        arranger_widget_get_selections (self);

      if (arranger_selections_has_any (sel))
        {
          double move_ticks = 0.0;
          if (self->ctrl_held)
            {
              Position tmp;
              position_set_to_bar (&tmp, 2);
              move_ticks = tmp.total_ticks;
            }
          else
            {
              SnapGrid * sg =
                arranger_widget_get_snap_grid (
                  self);
              move_ticks =
                (double)
                snap_grid_get_note_ticks (
                  sg->note_length, sg->note_type);
            }

          /* check arrow movement */
          if (keyval == GDK_KEY_Left)
            {
              Position min_possible_pos;
              arranger_widget_get_min_possible_position (
                self, &min_possible_pos);

              /* get earliest object */
              ArrangerObject * obj =
                arranger_selections_get_first_object (
                  sel);

              if (obj->pos.total_ticks - move_ticks >= min_possible_pos.total_ticks)
                {
                  UndoableAction * action =
                    arranger_selections_action_new_move (
                      sel, - move_ticks, 0, 0,
                      0, 0, 0, F_NOT_ALREADY_MOVED);
                  undo_manager_perform (
                    UNDO_MANAGER, action);

                  /* scroll left if needed */
                  arranger_widget_scroll_until_obj (
                    self, obj,
                    1, 0, 1, SCROLL_PADDING);
                }
            }
          else if (keyval == GDK_KEY_Right)
            {
              UndoableAction * action =
                arranger_selections_action_new_move (
                  sel, move_ticks, 0, 0, 0, 0, 0,
                  F_NOT_ALREADY_MOVED);
              undo_manager_perform (
                UNDO_MANAGER, action);

              /* get latest object */
              ArrangerObject * obj =
                arranger_selections_get_last_object (
                  sel);

              /* scroll right if needed */
              arranger_widget_scroll_until_obj (
                self, obj,
                1, 0, 0, SCROLL_PADDING);
            }
          else if (keyval == GDK_KEY_Down)
            {
              UndoableAction * action;
              if (self == MW_MIDI_ARRANGER ||
                  self == MW_MIDI_MODIFIER_ARRANGER)
                {
                  int pitch_delta = 0;
                  MidiNote * mn =
                    midi_arranger_selections_get_lowest_note (
                      MA_SELECTIONS);
                  ArrangerObject * obj =
                    (ArrangerObject *) mn;

                  if (self->ctrl_held)
                    {
                      if (mn->val - 12 >= 0)
                        pitch_delta = - 12;
                    }
                  else
                    {
                      if (mn->val - 1 >= 0)
                        pitch_delta = - 1;
                    }

                  if (pitch_delta)
                    {
                      action =
                        arranger_selections_action_new_move (
                          sel, 0, 0, pitch_delta,
                          0, 0, 0,
                          F_NOT_ALREADY_MOVED);
                      undo_manager_perform (
                        UNDO_MANAGER, action);

                      /* scroll down if needed */
                      arranger_widget_scroll_until_obj (
                        self, obj,
                        0, 0, 0, SCROLL_PADDING);
                    }
                }
              else if (self == MW_CHORD_ARRANGER)
                {
                  action =
                    arranger_selections_action_new_move (
                      sel, 0, -1, 0, 0, 0, 0,
                      F_NOT_ALREADY_MOVED);
                  undo_manager_perform (
                    UNDO_MANAGER, action);
                }
              else if (self == MW_TIMELINE)
                {
                  /* TODO check if can be moved */
                  /*action =*/
                    /*arranger_selections_action_new_move (*/
                      /*sel, 0, -1, 0, 0, 0);*/
                  /*undo_manager_perform (*/
                    /*UNDO_MANAGER, action);*/
                }
            }
          else if (keyval == GDK_KEY_Up)
            {
              UndoableAction * action;
              if (self == MW_MIDI_ARRANGER ||
                  self == MW_MIDI_MODIFIER_ARRANGER)
                {
                  int pitch_delta = 0;
                  MidiNote * mn =
                    midi_arranger_selections_get_highest_note (
                      MA_SELECTIONS);
                  ArrangerObject * obj =
                    (ArrangerObject *) mn;

                  if (self->ctrl_held)
                    {
                      if (mn->val + 12 < 128)
                        pitch_delta = 12;
                    }
                  else
                    {
                      if (mn->val + 1 < 128)
                        pitch_delta = 1;
                    }

                  if (pitch_delta)
                    {
                      action =
                        arranger_selections_action_new_move_midi (
                          sel, 0, pitch_delta,
                          F_NOT_ALREADY_MOVED);
                      undo_manager_perform (
                        UNDO_MANAGER, action);

                      /* scroll up if needed */
                      arranger_widget_scroll_until_obj (
                        self, obj,
                        0, 1, 0, SCROLL_PADDING);
                    }
                }
              else if (self == MW_CHORD_ARRANGER)
                {
                  action =
                    arranger_selections_action_new_move_chord (
                      sel, 0, 1,
                      F_NOT_ALREADY_MOVED);
                  undo_manager_perform (
                    UNDO_MANAGER, action);
                }
              else if (self == MW_TIMELINE)
                {
                  /* TODO check if can be moved */
                  /*action =*/
                    /*arranger_selections_action_new_move (*/
                      /*sel, 0, 1, 0, 0, 0);*/
                  /*undo_manager_perform (*/
                    /*UNDO_MANAGER, action);*/
                }
            }
        }
    }

  if (self->type == TYPE (TIMELINE))
    {
      timeline_arranger_widget_set_cut_lines_visible (
        self);
    }

  /*arranger_widget_update_visibility (self);*/

  arranger_widget_refresh_cursor (
    self);

  /*if (num > 0)*/
    /*auto_scroll (self);*/

  return FALSE;
}

/**
 * On button press.
 *
 * This merely sets the number of clicks and
 * objects clicked on. It is always called
 * before drag_begin, so the logic is done in drag_begin.
 */
static void
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ArrangerWidget *      self)
{
  /* set number of presses */
  self->n_press = n_press;

  /* set modifier button states */
  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));
  if (state_mask & GDK_SHIFT_MASK)
    self->shift_held = 1;
  if (state_mask & GDK_CONTROL_MASK)
    self->ctrl_held = 1;
}

/**
 * Called when an item needs to be created at the
 * given position.
 */
static void
create_item (
  ArrangerWidget * self,
  double           start_x,
  double           start_y)
{
  /* something will be created */
  Position pos;
  Track * track = NULL;
  AutomationTrack * at = NULL;
  int note, chord_index;
  ZRegion * region = NULL;

  /* get the position */
  arranger_widget_px_to_pos (
    self, start_x, &pos, 1);

  /* snap it */
  if (!self->shift_held &&
      SNAP_GRID_ANY_SNAP (self->snap_grid))
    position_snap (
      NULL, &pos, NULL, NULL,
      self->snap_grid);

  switch (self->type)
    {
    case TYPE (TIMELINE):
      /* figure out if we are creating a region or
       * automation point */
      at =
        timeline_arranger_widget_get_at_at_y (
          self, start_y);
      track =
        timeline_arranger_widget_get_track_at_y (
          self, start_y);

      /* creating automation region */
      if (at)
        {
          timeline_arranger_widget_create_region (
            self,
            REGION_TYPE_AUTOMATION, track, NULL, at,
            &pos);
        }
      /* double click inside a track */
      else if (track)
        {
          TrackLane * lane =
            timeline_arranger_widget_get_track_lane_at_y (
              self, start_y);
          switch (track->type)
            {
            case TRACK_TYPE_INSTRUMENT:
              timeline_arranger_widget_create_region (
                self, REGION_TYPE_MIDI, track,
                lane, NULL, &pos);
              break;
            case TRACK_TYPE_MIDI:
              timeline_arranger_widget_create_region (
                self, REGION_TYPE_MIDI, track,
                lane, NULL, &pos);
              break;
            case TRACK_TYPE_AUDIO:
              break;
            case TRACK_TYPE_MASTER:
              break;
            case TRACK_TYPE_CHORD:
              timeline_arranger_widget_create_chord_or_scale (
                self, track, start_y, &pos);
              break;
            case TRACK_TYPE_AUDIO_BUS:
              break;
            case TRACK_TYPE_AUDIO_GROUP:
              break;
            case TRACK_TYPE_MARKER:
              timeline_arranger_widget_create_marker (
                self, track, &pos);
              break;
            default:
              /* TODO */
              break;
            }
        }
      break;
    case TYPE (MIDI):
      /* find the note and region at x,y */
      note =
        piano_roll_keys_widget_get_key_from_y (
          MW_PIANO_ROLL_KEYS, start_y);
      region =
        clip_editor_get_region (CLIP_EDITOR);

      /* create a note */
      if (region)
        {
          midi_arranger_widget_create_note (
            self, &pos, note, (ZRegion *) region);
        }
      break;
    case TYPE (MIDI_MODIFIER):
    case TYPE (AUDIO):
      break;
    case TYPE (CHORD):
      /* find the chord and region at x,y */
      chord_index =
        chord_arranger_widget_get_chord_at_y (
          start_y);
      region =
        clip_editor_get_region (CLIP_EDITOR);

      /* create a chord object */
      if (region && chord_index <
            CHORD_EDITOR->num_chords)
        {
          chord_arranger_widget_create_chord (
            self, &pos, chord_index,
            region);
        }
      break;
    case TYPE (AUTOMATION):
      region =
        clip_editor_get_region (CLIP_EDITOR);

      if (region)
        {
          automation_arranger_widget_create_ap (
            self, &pos, start_y, region);
        }
      break;
    }

  /* set the start selections */
  self->sel_at_start =
    arranger_selections_clone (
      arranger_widget_get_selections (self));
}

static void
drag_cancel (
  GtkGesture *       gesture,
  GdkEventSequence * sequence,
  ArrangerWidget *   self)
{
  g_message ("drag cancelled");
}

/**
 * Sets the start pos of the earliest object and
 * the flag whether the earliest object exists.
 */
static void
set_earliest_obj (
  ArrangerWidget * self)
{
  ArrangerSelections * sel =
    arranger_widget_get_selections (self);
  if (arranger_selections_has_any (sel))
    {
      arranger_selections_get_start_pos (
        sel, &self->earliest_obj_start_pos,
        F_GLOBAL);
      self->earliest_obj_exists = 1;
    }
  else
    {
      self->earliest_obj_exists = 0;
    }
}

/**
 * Checks for the first object hit, sets the
 * appropriate action and selects it.
 *
 * @return If an object was handled or not.
 */
static bool
on_drag_begin_handle_hit_object (
  ArrangerWidget * self,
  const double     x,
  const double     y)
{
  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      self, ARRANGER_OBJECT_TYPE_ALL, x, y);

  if (!obj)
    {
      return false;
    }

  /* get x as local to the object */
  int wx =
    (int) x - obj->full_rect.x;

  /* TODO get y as local to the object */
  int wy =
    (int) y - obj->full_rect.y;

  /* remember object and pos */
  self->start_object = obj;
  self->start_pos_px = x;

  /* get flags */
  int is_fade_in_point =
    arranger_object_is_fade_in (obj, wx, wy, 1, 0);
  int is_fade_out_point =
    arranger_object_is_fade_out (obj, wx, wy, 1, 0);
  int is_fade_in_outer =
    arranger_object_is_fade_in (obj, wx, wy, 0, 1);
  int is_fade_out_outer =
    arranger_object_is_fade_out (obj, wx, wy, 0, 1);
  int is_resize_l =
    arranger_object_is_resize_l (obj, wx);
  int is_resize_r =
    arranger_object_is_resize_r (obj, wx);
  int is_resize_up =
    arranger_object_is_resize_up (obj, wx, wy);
  int is_resize_loop =
    arranger_object_is_resize_loop (obj, wy);
  int show_cut_lines =
    arranger_object_should_show_cut_lines (
      obj, self->alt_held);
  int is_selected =
    arranger_object_is_selected (obj);
  self->start_object_was_selected = is_selected;

  /* select object if unselected */
  switch (P_TOOL)
    {
    case TOOL_SELECT_NORMAL:
    case TOOL_SELECT_STRETCH:
    case TOOL_EDIT:
      /* if ctrl held & not selected, add to
       * selections */
      if (self->ctrl_held && !is_selected)
        {
          arranger_object_select (
            obj, F_SELECT, F_APPEND);
        }
      /* if ctrl not held & not selected, make it
       * the only
       * selection */
      else if (!self->ctrl_held && !is_selected)
        {
          arranger_object_select (
            obj, F_SELECT, F_NO_APPEND);
        }
      break;
    case TOOL_CUT:
      /* only select this object */
      arranger_object_select (
        obj, F_SELECT, F_NO_APPEND);
      break;
    default:
      break;
    }

  /* set editor region and show editor if double
   * click */
  if (obj->type == ARRANGER_OBJECT_TYPE_REGION)
    {
      clip_editor_set_region (
        CLIP_EDITOR, (ZRegion *) obj, true);

      /* if double click bring up piano roll */
      if (self->n_press == 2 &&
          !self->ctrl_held)
        {
          /* set the bot panel visible */
          gtk_widget_set_visible (
            GTK_WIDGET (
              MW_BOT_DOCK_EDGE), 1);
          foldable_notebook_widget_set_visibility (
            MW_BOT_FOLDABLE_NOTEBOOK, 1);

          SHOW_CLIP_EDITOR;
        }
    }
  /* if open marker dialog if double click on
   * marker */
  else if (obj->type == ARRANGER_OBJECT_TYPE_MARKER)
    {
      if (self->n_press == 2 && !self->ctrl_held)
        {
          MarkerDialogWidget * dialog =
            marker_dialog_widget_new (
              (Marker *) obj);
          gtk_widget_show_all (GTK_WIDGET (dialog));
        }
    }

#define SET_ACTION(x) \
  self->action = UI_OVERLAY_ACTION_##x

  /* update arranger action */
  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      switch (P_TOOL)
        {
        case TOOL_ERASER:
          SET_ACTION (ERASING);
          break;
        case TOOL_AUDITION:
          SET_ACTION (AUDITIONING);
          break;
        case TOOL_SELECT_NORMAL:
          if (is_fade_in_point)
            SET_ACTION (RESIZING_L_FADE);
          else if (is_fade_out_point)
            SET_ACTION (RESIZING_R_FADE);
          else if (is_resize_l && is_resize_loop)
            SET_ACTION (RESIZING_L_LOOP);
          else if (is_resize_l)
            SET_ACTION (RESIZING_L);
          else if (is_resize_r && is_resize_loop)
            SET_ACTION (RESIZING_R_LOOP);
          else if (is_resize_r)
            SET_ACTION (RESIZING_R);
          else if (show_cut_lines)
            SET_ACTION (CUTTING);
          else if (is_fade_in_outer)
            SET_ACTION (RESIZING_UP_FADE_IN);
          else if (is_fade_out_outer)
            SET_ACTION (RESIZING_UP_FADE_OUT);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_SELECT_STRETCH:
          if (is_resize_l)
            SET_ACTION (STRETCHING_L);
          else if (is_resize_r)
            SET_ACTION (STRETCHING_R);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_EDIT:
          if (is_resize_l)
            SET_ACTION (RESIZING_L);
          else if (is_resize_r)
            SET_ACTION (RESIZING_R);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_CUT:
          SET_ACTION (CUTTING);
          break;
        case TOOL_RAMP:
          /* TODO */
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      switch (P_TOOL)
        {
        case TOOL_ERASER:
          SET_ACTION (ERASING);
          break;
        case TOOL_AUDITION:
          SET_ACTION (AUDITIONING);
          break;
        case TOOL_SELECT_NORMAL:
        case TOOL_EDIT:
        case TOOL_SELECT_STRETCH:
          if ((is_resize_l) &&
              !PIANO_ROLL->drum_mode)
            SET_ACTION (RESIZING_L);
          else if (is_resize_r &&
                   !PIANO_ROLL->drum_mode)
            SET_ACTION (RESIZING_R);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_CUT:
          /* TODO */
          break;
        case TOOL_RAMP:
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      if (is_resize_up)
        SET_ACTION (RESIZING_UP);
      else
        SET_ACTION (STARTING_MOVING);
      break;
    case ARRANGER_OBJECT_TYPE_VELOCITY:
      if (is_resize_up)
        SET_ACTION (RESIZING_UP);
      else
        SET_ACTION (NONE);
      break;
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      SET_ACTION (STARTING_MOVING);
      break;
    case ARRANGER_OBJECT_TYPE_SCALE_OBJECT:
      SET_ACTION (STARTING_MOVING);
      break;
    case ARRANGER_OBJECT_TYPE_MARKER:
      SET_ACTION (STARTING_MOVING);
      break;
    default:
      g_return_val_if_reached (0);
    }

#undef SET_ACTION

  /* clone the arranger selections at this point */
  ArrangerSelections * orig_selections =
    arranger_widget_get_selections (self);
  self->sel_at_start =
    arranger_selections_clone (orig_selections);

  /* if the action is stretching, set the
   * "before_length" on each region */
  if (orig_selections->type ==
        ARRANGER_SELECTIONS_TYPE_TIMELINE &&
      ACTION_IS (STRETCHING_R))
    {
      TimelineSelections * sel =
        (TimelineSelections *) orig_selections;
      transport_prepare_audio_regions_for_stretch (
        TRANSPORT, sel);
    }

  return true;
}

static void
drag_begin (
  GtkGestureDrag *   gesture,
  gdouble            start_x,
  gdouble            start_y,
  ArrangerWidget *   self)
{
  self->start_x = start_x;
  arranger_widget_px_to_pos (
    self, start_x, &self->start_pos, 1);
  self->start_y = start_y;

  /* check if selections can create links */
  self->can_link =
    TYPE_IS (TIMELINE) &&
    TL_SELECTIONS->num_regions > 0 &&
    TL_SELECTIONS->num_scale_objects == 0 &&
    TL_SELECTIONS->num_markers == 0;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  /* get current pos */
  arranger_widget_px_to_pos (
    self, self->start_x,
    &self->curr_pos, 1);

  /* get difference with drag start pos */
  self->curr_ticks_diff_from_start =
    position_get_ticks_diff (
      &self->curr_pos, &self->start_pos, NULL);

  /* handle hit object */
  int objects_hit =
    on_drag_begin_handle_hit_object (
      self, start_x, start_y);
  g_message ("objects hit %d", objects_hit);

  if (objects_hit)
    {
      ArrangerSelections * sel =
        arranger_widget_get_selections (self);
      self->sel_at_start =
        arranger_selections_clone (sel);
    }
  /* if nothing hit */
  else
    {
      self->sel_at_start = NULL;

      /* single click */
      if (self->n_press == 1)
        {
          switch (P_TOOL)
            {
            case TOOL_SELECT_NORMAL:
            case TOOL_SELECT_STRETCH:
              /* selection */
              self->action =
                UI_OVERLAY_ACTION_STARTING_SELECTION;

              /* deselect all */
              arranger_widget_select_all (self, 0);

              /* if timeline, set either selecting
               * objects or selecting range */
              if (self->type == TYPE (TIMELINE))
                {
                  timeline_arranger_widget_set_select_type (
                    self, start_y);
                }
              break;
            case TOOL_EDIT:
              if (!self->ctrl_held)
                {
                  /* something is created */
                  create_item (
                    self, start_x, start_y);
                }
              else
                {
                  /* autofill */
                }
              break;
            case TOOL_ERASER:
              /* delete selection */
              self->action =
                UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
              break;
            case TOOL_RAMP:
              self->action =
                UI_OVERLAY_ACTION_STARTING_RAMP;
              break;
            default:
              break;
            }
        }
      /* double click */
      else if (self->n_press == 2)
        {
          switch (P_TOOL)
            {
            case TOOL_SELECT_NORMAL:
            case TOOL_SELECT_STRETCH:
            case TOOL_EDIT:
              create_item (
                self, start_x, start_y);
              break;
            case TOOL_ERASER:
              /* delete selection */
              self->action =
                UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
              break;
            default:
              break;
            }
        }
    }

  /* set the start pos of the earliest object and
   * the flag whether the earliest object exists */
  set_earliest_obj (self);

  arranger_widget_refresh_cursor (self);
}

/**
 * Selects objects for the given arranger in the
 * range from start_* to offset_*.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
static void
select_in_range (
  ArrangerWidget * self,
  double           offset_x,
  double           offset_y,
  int              delete)
{
  ArrangerSelections * prev_sel =
    arranger_selections_clone (
      arranger_widget_get_selections (self));

  if (delete)
    {
      int num_objs;
      ArrangerObject ** objs =
        arranger_selections_get_all_objects (
          self->sel_to_delete, &num_objs);
      for (int i = 0; i < num_objs; i++)
        {
          objs[i]->deleted_temporarily = false;
        }
      arranger_selections_clear (
        self->sel_to_delete);
      free (objs);
    }
  else
    {
      /* deselect all */
      arranger_widget_select_all (self, 0);
    }

  ArrangerObject * objs[800];
  int              num_objs = 0;
  GdkRectangle rect = {
    .x = (int) offset_x >= 0 ?
      (int) self->start_x :
      (int) (self->start_x + offset_x),
    .y = (int) offset_y >= 0 ?
      (int) self->start_y :
      (int) (self->start_y + offset_y),
    .width = abs ((int) offset_x),
    .height = abs ((int) offset_y),
  };
  switch (self->type)
    {
    case TYPE (CHORD):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_CHORD_OBJECT,
        &rect, objs, &num_objs);
      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];
          if (delete)
            {
              arranger_selections_add_object (
                self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND);
            }
        }
      break;
    case TYPE (AUTOMATION):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_AUTOMATION_POINT,
        &rect, objs, &num_objs);
      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];
          if (delete)
            {
              arranger_selections_add_object (
                self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND);
            }
        }
      break;
    case TYPE (TIMELINE):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_REGION,
        &rect, objs, &num_objs);
      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];
          if (delete)
            {
              arranger_selections_add_object (
                self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              /* select the enclosed region */
              arranger_object_select (
                obj, F_SELECT, F_APPEND);
            }
        }
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_SCALE_OBJECT,
        &rect, objs, &num_objs);
      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];
          if (delete)
            {
              arranger_selections_add_object (
                self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND);
            }
        }
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_MARKER,
        &rect, objs, &num_objs);
      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];
          if (delete)
            {
              arranger_selections_add_object (
                self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND);
            }
        }
      break;
    case TYPE (MIDI):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_MIDI_NOTE,
        &rect, objs, &num_objs);
      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];
          if (delete)
            {
              arranger_selections_add_object (
                self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND);
            }
        }
      midi_arranger_selections_unlisten_note_diff (
        (MidiArrangerSelections *) prev_sel,
        (MidiArrangerSelections *)
        arranger_widget_get_selections (self));
      break;
    case TYPE (MIDI_MODIFIER):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_VELOCITY,
        &rect, objs, &num_objs);
      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];
          Velocity * vel =
            (Velocity *) obj;
          MidiNote * mn =
            velocity_get_midi_note (vel);
          ArrangerObject * mn_obj =
            (ArrangerObject *) mn;

          if (delete)
            {
              arranger_selections_add_object (
                self->sel_to_delete, mn_obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                mn_obj, F_SELECT, F_APPEND);
            }
        }
      break;
    default:
      break;
    }

  if (prev_sel)
    arranger_selections_free (prev_sel);
}

static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  ArrangerWidget * self)
{
  /* state mask needs to be updated */
  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));
  if (state_mask & GDK_SHIFT_MASK)
    self->shift_held = 1;
  else
    self->shift_held = 0;
  if (state_mask & GDK_CONTROL_MASK)
    self->ctrl_held = 1;
  else
    self->ctrl_held = 0;

  /* get current pos */
  arranger_widget_px_to_pos (
    self, self->start_x + offset_x,
    &self->curr_pos, 1);

  /* get difference with drag start pos */
  self->curr_ticks_diff_from_start =
    position_get_ticks_diff (
      &self->curr_pos, &self->start_pos, NULL);

  if (self->earliest_obj_exists)
    {
      /* add diff to the earliest object's start pos
       * and snap it, then get the diff ticks */
      Position earliest_obj_new_pos;
      position_set_to_pos (
        &earliest_obj_new_pos,
        &self->earliest_obj_start_pos);
      position_add_ticks (
        &earliest_obj_new_pos,
        self->curr_ticks_diff_from_start);

      if (position_is_before (
            &earliest_obj_new_pos, &POSITION_START))
        {
          /* stop at 0.0.0.0 */
          position_set_to_pos (
            &earliest_obj_new_pos, &POSITION_START);
        }
      else if (!self->shift_held &&
          SNAP_GRID_ANY_SNAP (self->snap_grid))
        {
          position_snap (
            NULL, &earliest_obj_new_pos, NULL,
            NULL, self->snap_grid);
        }
      self->adj_ticks_diff =
        position_get_ticks_diff (
          &earliest_obj_new_pos,
          &self->earliest_obj_start_pos,
          NULL);
    }

  /* set action to selecting if starting selection.
   * this
   * is because drag_update never gets called if
   * it's just
   * a click, so we can check at drag_end and see if
   * anything was selected */
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      self->action =
        UI_OVERLAY_ACTION_SELECTING;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
      self->action =
        UI_OVERLAY_ACTION_DELETE_SELECTING;
      {
        ArrangerSelections * sel =
          arranger_widget_get_selections (self);
        arranger_selections_clear (sel);
        self->sel_to_delete =
          arranger_selections_clone (
            arranger_widget_get_selections (self));
      }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      if (self->alt_held &&
          self->can_link)
        self->action =
          UI_OVERLAY_ACTION_MOVING_LINK;
      else if (self->ctrl_held)
        self->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        self->action =
          UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_MOVING:
      if (self->alt_held &&
          self->can_link)
        self->action =
          UI_OVERLAY_ACTION_MOVING_LINK;
      else if (self->ctrl_held)
        self->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      break;
    case UI_OVERLAY_ACTION_MOVING_LINK:
      if (!self->alt_held)
        self->action =
          self->ctrl_held ?
          UI_OVERLAY_ACTION_MOVING_COPY :
          UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
      if (!self->ctrl_held)
        self->action =
          self->alt_held && self->can_link ?
          UI_OVERLAY_ACTION_MOVING_LINK :
          UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_STARTING_RAMP:
      self->action =
        UI_OVERLAY_ACTION_RAMPING;
      if (self->type == TYPE (MIDI_MODIFIER))
        {
          midi_modifier_arranger_widget_set_start_vel (
            self);
        }
      break;
    case UI_OVERLAY_ACTION_CUTTING:
      /* alt + move changes the action from
       * cutting to moving-link */
      if (self->alt_held && self->can_link)
        self->action =
          UI_OVERLAY_ACTION_MOVING_LINK;
      break;
    default:
      break;
    }

  /* update visibility */
  /*arranger_widget_update_visibility (self);*/

  switch (self->action)
    {
    /* if drawing a selection */
    case UI_OVERLAY_ACTION_SELECTING:
      /* find and select objects inside selection */
      select_in_range (
        self, offset_x, offset_y, F_NO_DELETE);
      EVENTS_PUSH (
        ET_SELECTING_IN_ARRANGER, self);
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      /* find and delete objects inside
       * selection */
      select_in_range (
        self, offset_x, offset_y, F_DELETE);
      EVENTS_PUSH (
        ET_SELECTING_IN_ARRANGER, self);
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_FADE:
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
      /* snap selections based on new pos */
      if (self->type == TYPE (TIMELINE))
        {
          int ret =
            timeline_arranger_widget_snap_regions_l (
              self,
              &self->curr_pos, 1);
          if (!ret)
            timeline_arranger_widget_snap_regions_l (
              self,
              &self->curr_pos, 0);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
    case UI_OVERLAY_ACTION_STRETCHING_L:
      /* snap selections based on new pos */
      if (self->type == TYPE (TIMELINE))
        {
          int ret =
            timeline_arranger_widget_snap_regions_l (
              self, &self->curr_pos, 1);
          if (!ret)
            timeline_arranger_widget_snap_regions_l (
              self, &self->curr_pos, 0);
        }
      else if (self->type == TYPE (MIDI))
        {
          int ret =
            midi_arranger_widget_snap_midi_notes_l (
              self, &self->curr_pos, 1);
          if (!ret)
            midi_arranger_widget_snap_midi_notes_l (
              self, &self->curr_pos, 0);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_FADE:
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      if (self->type == TYPE (TIMELINE))
        {
          if (self->resizing_range)
            timeline_arranger_widget_snap_range_r (
              self, &self->curr_pos);
          else
            {
              int ret =
                timeline_arranger_widget_snap_regions_r (
                  self, &self->curr_pos, 1);
              if (!ret)
                timeline_arranger_widget_snap_regions_r (
                  self, &self->curr_pos, 0);
            }
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_STRETCHING_R:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      if (self->type == TYPE (TIMELINE))
        {
          if (self->resizing_range)
            timeline_arranger_widget_snap_range_r (
              self, &self->curr_pos);
          else
            {
              int ret =
                timeline_arranger_widget_snap_regions_r (
                  self, &self->curr_pos, 1);
              if (!ret)
                timeline_arranger_widget_snap_regions_r (
                  self, &self->curr_pos, 0);
            }
        }
      else if (self->type == TYPE (MIDI))
        {
          int ret =
            midi_arranger_widget_snap_midi_notes_r (
              self, &self->curr_pos, 1);
          if (!ret)
            midi_arranger_widget_snap_midi_notes_r (
              self, &self->curr_pos, 0);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      if (self->type == TYPE (MIDI_MODIFIER))
        {
          midi_modifier_arranger_widget_resize_velocities (
            self, offset_y);
        }
      else if (self->type == TYPE (AUTOMATION))
        {
          automation_arranger_widget_resize_curves (
            self, offset_y);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN:
      timeline_arranger_widget_fade_up (
        self, offset_y, 1);
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT:
      timeline_arranger_widget_fade_up (
        self, offset_y, 0);
      break;
    case UI_OVERLAY_ACTION_MOVING:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      move_items_x (
        self,
        self->adj_ticks_diff -
          self->last_adj_ticks_diff);
      move_items_y (self, offset_y);
      break;
    case UI_OVERLAY_ACTION_AUTOFILLING:
      /* TODO */
      g_message ("autofilling");
      break;
    case UI_OVERLAY_ACTION_AUDITIONING:
      /* TODO */
      g_message ("auditioning");
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      /* find and select objects inside selection */
      if (self->type == TYPE (MIDI_MODIFIER))
        {
          midi_modifier_arranger_widget_ramp (
            self, offset_x, offset_y);
        }
      break;
    case UI_OVERLAY_ACTION_CUTTING:
      /* nothing to do, wait for drag end */
      break;
    default:
      /* TODO */
      break;
    }

  if (self->action != UI_OVERLAY_ACTION_NONE)
    auto_scroll (
      self, (int) (self->start_x + offset_x),
      (int) (self->start_y + offset_y));

  if (self->type == TYPE (MIDI))
    {
      midi_arranger_listen_notes (
        self, 1);
    }

  /* update last offsets */
  self->last_offset_x = offset_x;
  self->last_offset_y = offset_y;
  self->last_adj_ticks_diff = self->adj_ticks_diff;

  arranger_widget_redraw_whole (self);
  arranger_widget_refresh_cursor (self);
}

static void
on_drag_end_automation (
  ArrangerWidget * self)
{
  /*AutomationPoint * ap;*/
  for (int i = 0;
       i < AUTOMATION_SELECTIONS->
             num_automation_points; i++)
    {
      /*ap =*/
        /*AUTOMATION_SELECTIONS->automation_points[i];*/
      /*ArrangerObject * obj =*/
        /*(ArrangerObject *) ap;*/
      /*arranger_object_widget_update_tooltip (*/
        /*(ArrangerObjectWidget *) obj->widget, 0);*/
    }

  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_UP:
      {
        UndoableAction * ua =
          arranger_selections_action_new_edit (
            self->sel_at_start,
            (ArrangerSelections *)
            AUTOMATION_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE,
            F_ALREADY_EDITED);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      {
        /* if something was clicked with ctrl without
         * moving*/
        if (self->ctrl_held)
          {
            /*if (self->start_region &&*/
                /*self->start_region_was_selected)*/
              /*{*/
                /*[> deselect it <]*/
                /*[>ARRANGER_WIDGET_SELECT_REGION (<]*/
                  /*[>self, self->start_region,<]*/
                  /*[>F_NO_SELECT, F_APPEND);<]*/
              /*}*/
          }
        else if (self->n_press == 2)
          {
            /* double click on object */
            /*g_message ("DOUBLE CLICK");*/
          }
      }
    break;
    case UI_OVERLAY_ACTION_MOVING:
      {
        AutomationSelections * sel_at_start =
          (AutomationSelections *) self->sel_at_start;
        AutomationPoint * start_ap =
          sel_at_start->automation_points[0];
        ArrangerObject * start_obj =
          (ArrangerObject *) start_ap;
        AutomationPoint * ap =
          AUTOMATION_SELECTIONS->automation_points[0];
        ArrangerObject * obj =
          (ArrangerObject *) ap;
        double ticks_diff =
          obj->pos.total_ticks -
          start_obj->pos.total_ticks;
        double norm_value_diff =
          (double)
          (ap->normalized_val -
           start_ap->normalized_val);
        UndoableAction * ua =
          arranger_selections_action_new_move_automation (
            AUTOMATION_SELECTIONS,
            ticks_diff, norm_value_diff,
            F_ALREADY_MOVED);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
  /* if copy-moved */
    case UI_OVERLAY_ACTION_MOVING_COPY:
      {
        ArrangerObject * obj =
          (ArrangerObject *) self->start_object;
        double ticks_diff =
          obj->pos.total_ticks -
          obj->transient->pos.total_ticks;
        float value_diff =
          ((AutomationPoint *) obj)->normalized_val -
          ((AutomationPoint *) obj->transient)->
            normalized_val;
        UndoableAction * ua = NULL;
        ua =
          (UndoableAction *)
          arranger_selections_action_new_duplicate_automation (
            (ArrangerSelections *)
              AUTOMATION_SELECTIONS,
            ticks_diff, value_diff,
            F_ALREADY_MOVED);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_NONE:
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      {
        arranger_selections_clear (
          (ArrangerSelections *)
          AUTOMATION_SELECTIONS);
      }
      break;
    /* if something was created */
    case UI_OVERLAY_ACTION_CREATING_MOVING:
      {
        UndoableAction * ua =
          arranger_selections_action_new_create (
            (ArrangerSelections *)
            AUTOMATION_SELECTIONS);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      {
        UndoableAction * ua =
          arranger_selections_action_new_delete (
            self->sel_to_delete);
        undo_manager_perform (
          UNDO_MANAGER, ua);
        object_free_w_func_and_null (
          arranger_selections_free_full,
          self->sel_to_delete);
      }
      break;
    /* if didn't click on something */
    default:
      break;
    }

  self->start_object = NULL;
}

static void
on_drag_end_midi_modifier (
  ArrangerWidget * self)
{
  MidiNote * midi_note;
  /*Velocity * vel;*/
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        MA_SELECTIONS->midi_notes[i];
      /*vel = midi_note->vel;*/

      /*ArrangerObject * vel_obj =*/
        /*(ArrangerObject *) vel;*/
      /*if (Z_IS_ARRANGER_OBJECT_WIDGET (*/
            /*vel_obj->widget))*/
        /*{*/
          /*arranger_object_widget_update_tooltip (*/
            /*(ArrangerObjectWidget *)*/
            /*vel_obj->widget, 0);*/
        /*}*/

      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_CHANGED, midi_note);
    }

  /*arranger_widget_update_visibility (*/
    /*(ArrangerWidget *) self);*/

  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_UP:
      {
        g_return_if_fail (self->sel_at_start);
        UndoableAction * ua =
          arranger_selections_action_new_edit (
            self->sel_at_start,
            (ArrangerSelections *) MA_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE,
            true);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      {
        Position selection_start_pos,
                 selection_end_pos;
        ui_px_to_pos_editor (
          self->start_x,
          self->last_offset_x >= 0 ?
            &selection_start_pos :
            &selection_end_pos,
          F_PADDING);
        ui_px_to_pos_editor (
          self->start_x + self->last_offset_x,
          self->last_offset_x >= 0 ?
            &selection_end_pos :
            &selection_start_pos,
          F_PADDING);

        /* prepare the velocities in cloned
         * arranger selections from the
         * vels at start */
        midi_modifier_arranger_widget_select_vels_in_range (
          self, self->last_offset_x);
        self->sel_at_start =
          arranger_selections_clone (
            (ArrangerSelections *)
            MA_SELECTIONS);
        MidiArrangerSelections * sel_at_start =
          (MidiArrangerSelections *)
          self->sel_at_start;
        for (int i = 0;
             i < sel_at_start->num_midi_notes; i++)
          {
            MidiNote * mn =
              sel_at_start->midi_notes[i];
            Velocity * vel = mn->vel;
            vel->vel = vel->vel_at_start;
          }

        UndoableAction * ua =
          arranger_selections_action_new_edit (
            self->sel_at_start,
            (ArrangerSelections *) MA_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE,
            true);
        if (ua)
          undo_manager_perform (
            UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      {
        UndoableAction * ua =
          arranger_selections_action_new_delete (
            self->sel_to_delete);
        undo_manager_perform (
          UNDO_MANAGER, ua);
        object_free_w_func_and_null (
          arranger_selections_free_full,
          self->sel_to_delete);
      }
      break;
    default:
      break;
    }
}

static void
on_drag_end_midi (
  ArrangerWidget * self)
{
  midi_arranger_listen_notes (self, 0);

  MidiNote * midi_note;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        MA_SELECTIONS->midi_notes[i];
      /*ArrangerObject * mn_obj =*/
        /*(ArrangerObject *) midi_note;*/

      /*if (Z_IS_ARRANGER_OBJECT_WIDGET (*/
            /*mn_obj->widget))*/
        /*{*/
          /*arranger_object_widget_update_tooltip (*/
            /*Z_ARRANGER_OBJECT_WIDGET (*/
              /*mn_obj->widget), 0);*/
        /*}*/

      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_CREATED, midi_note);
    }

  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_L:
    {
      ArrangerObject * obj =
        (ArrangerObject *) self->start_object;
      double ticks_diff =
        obj->pos.total_ticks -
        obj->transient->pos.total_ticks;
      UndoableAction * ua =
        arranger_selections_action_new_resize (
          (ArrangerSelections *) MA_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_L,
          ticks_diff);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    {
      ArrangerObject * obj =
        (ArrangerObject *) self->start_object;
      double ticks_diff =
        obj->end_pos.total_ticks -
        obj->transient->end_pos.total_ticks;
      UndoableAction * ua =
        arranger_selections_action_new_resize (
          (ArrangerSelections *) MA_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_R,
          ticks_diff);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    {
      /* if something was clicked with ctrl without
       * moving*/
      if (self->ctrl_held)
        {
          if (self->start_object &&
              self->start_object_was_selected)
            {
              /* deselect it */
              arranger_object_select (
                self->start_object,
                F_NO_SELECT, F_APPEND);
            }
        }
    }
      break;
    case UI_OVERLAY_ACTION_MOVING:
    {
      ArrangerObject * obj =
        (ArrangerObject *) self->start_object;
      double ticks_diff =
        obj->pos.total_ticks -
        obj->transient->pos.total_ticks;
      int pitch_diff =
        ((MidiNote *) obj)->val -
        ((MidiNote *) obj->transient)->val;
      UndoableAction * ua =
        arranger_selections_action_new_move_midi (
          MA_SELECTIONS, ticks_diff, pitch_diff,
          F_ALREADY_MOVED);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
      break;
    /* if copy/link-moved */
    case UI_OVERLAY_ACTION_MOVING_COPY:
    {
      ArrangerObject * obj =
        (ArrangerObject *) self->start_object;
      double ticks_diff =
        obj->pos.total_ticks -
        obj->transient->pos.total_ticks;
      int pitch_diff =
        ((MidiNote *) obj)->val -
        ((MidiNote *) obj->transient)->val;
      UndoableAction * ua = NULL;
      ua =
        (UndoableAction *)
        arranger_selections_action_new_duplicate_midi (
          (ArrangerSelections *) MA_SELECTIONS,
          ticks_diff, pitch_diff,
          F_ALREADY_MOVED);

      if (ua)
        {
          undo_manager_perform (
            UNDO_MANAGER, ua);
        }
    }
      break;
    case UI_OVERLAY_ACTION_NONE:
      {
        arranger_selections_clear (
          (ArrangerSelections *) MA_SELECTIONS);
      }
      break;
    /* something was created */
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      {
        UndoableAction * ua =
          arranger_selections_action_new_create (
            (ArrangerSelections *) MA_SELECTIONS);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      {
        UndoableAction * ua =
          arranger_selections_action_new_delete (
            self->sel_to_delete);
        undo_manager_perform (
          UNDO_MANAGER, ua);
        object_free_w_func_and_null (
          arranger_selections_free_full,
          self->sel_to_delete);
      }
      break;
    /* if didn't click on something */
    default:
      {
      }
      break;
    }

  self->start_object = NULL;
  /*if (self->start_midi_note_clone)*/
    /*{*/
      /*midi_note_free (self->start_midi_note_clone);*/
      /*self->start_midi_note_clone = NULL;*/
    /*}*/

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED, MA_SELECTIONS);
}

static void
on_drag_end_chord (
  ArrangerWidget * self)
{
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      {
        /* if something was clicked with ctrl without
         * moving*/
        if (self->ctrl_held)
          {
            if (self->start_object &&
                self->start_object_was_selected)
              {
                /*[> deselect it <]*/
                arranger_object_select (
                  self->start_object,
                  F_NO_SELECT, F_APPEND);
              }
          }
      }
      break;
    case UI_OVERLAY_ACTION_MOVING:
      {
        ArrangerObject * obj =
          (ArrangerObject *) self->start_object;
        double ticks_diff =
          obj->pos.total_ticks -
          obj->transient->pos.total_ticks;
        UndoableAction * ua =
          (UndoableAction *)
          arranger_selections_action_new_move_chord (
            CHORD_SELECTIONS, ticks_diff,
            0, F_ALREADY_MOVED);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      {
        ArrangerObject * obj =
          (ArrangerObject *) self->start_object;
        double ticks_diff =
          obj->pos.total_ticks -
          obj->transient->pos.total_ticks;
        UndoableAction * ua =
          (UndoableAction *)
          arranger_selections_action_new_duplicate_chord (
            CHORD_SELECTIONS, ticks_diff,
            0, F_ALREADY_MOVED);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_NONE:
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      {
        arranger_selections_clear (
          (ArrangerSelections *)
          CHORD_SELECTIONS);
      }
      break;
    case UI_OVERLAY_ACTION_CREATING_MOVING:
      {
        UndoableAction * ua =
          arranger_selections_action_new_create (
            (ArrangerSelections *) CHORD_SELECTIONS);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      {
        UndoableAction * ua =
          arranger_selections_action_new_delete (
            self->sel_to_delete);
        undo_manager_perform (
          UNDO_MANAGER, ua);
        object_free_w_func_and_null (
          arranger_selections_free_full,
          self->sel_to_delete);
      }
      break;
    /* if didn't click on something */
    default:
      {
      }
      break;
    }
}

static void
on_drag_end_timeline (
  ArrangerWidget * self)
{
  /* clear tmp_lane from selected regions */
  /*for (int i = 0; i < TL_SELECTIONS->num_regions;*/
       /*i++)*/
    /*{*/
      /*ZRegion * region = TL_SELECTIONS->regions[i];*/
      /*region->tmp_lane = NULL;*/
    /*}*/

  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN:
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT:
      {
        UndoableAction * ua =
          arranger_selections_action_new_edit (
            self->sel_at_start,
            (ArrangerSelections *) TL_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_EDIT_FADES,
            true);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      if (!self->resizing_range)
        {
          ArrangerObject * obj =
            (ArrangerObject *) self->start_object;
          double ticks_diff =
            obj->pos.total_ticks -
            obj->transient->pos.total_ticks;
          UndoableAction * ua =
            arranger_selections_action_new_resize (
              (ArrangerSelections *) TL_SELECTIONS,
              ARRANGER_SELECTIONS_ACTION_RESIZE_L,
              ticks_diff);
          undo_manager_perform (
            UNDO_MANAGER, ua);
        }
      break;
    case UI_OVERLAY_ACTION_STRETCHING_L:
      {
        ArrangerObject * obj =
          (ArrangerObject *) self->start_object;
        double ticks_diff =
          obj->pos.total_ticks -
          obj->transient->pos.total_ticks;
        UndoableAction * ua =
          arranger_selections_action_new_resize (
            (ArrangerSelections *) TL_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_STRETCH_L,
            ticks_diff);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
        {
          ArrangerObject * obj =
            (ArrangerObject *) self->start_object;
          double ticks_diff =
            obj->pos.total_ticks -
            obj->transient->pos.total_ticks;
          UndoableAction * ua =
            arranger_selections_action_new_resize (
              (ArrangerSelections *) TL_SELECTIONS,
              ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP,
              ticks_diff);
          undo_manager_perform (
            UNDO_MANAGER, ua);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_FADE:
        {
          ArrangerObject * obj =
            (ArrangerObject *) self->start_object;
          double ticks_diff =
            obj->fade_in_pos.total_ticks -
            obj->transient->fade_in_pos.total_ticks;
          UndoableAction * ua =
            arranger_selections_action_new_resize (
              (ArrangerSelections *) TL_SELECTIONS,
              ARRANGER_SELECTIONS_ACTION_RESIZE_L_FADE,
              ticks_diff);
          undo_manager_perform (
            UNDO_MANAGER, ua);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      if (!self->resizing_range)
        {
          ArrangerObject * obj =
            (ArrangerObject *) self->start_object;
          double ticks_diff =
            obj->end_pos.total_ticks -
            obj->transient->end_pos.total_ticks;
          UndoableAction * ua =
            arranger_selections_action_new_resize (
              (ArrangerSelections *) TL_SELECTIONS,
              ARRANGER_SELECTIONS_ACTION_RESIZE_R,
              ticks_diff);
          undo_manager_perform (
            UNDO_MANAGER, ua);
        }
      break;
    case UI_OVERLAY_ACTION_STRETCHING_R:
      {
        ArrangerObject * obj =
          (ArrangerObject *) self->start_object;
        double ticks_diff =
          obj->end_pos.total_ticks -
          obj->transient->end_pos.total_ticks;
        /* stretch now */
        transport_stretch_audio_regions (
          TRANSPORT, TL_SELECTIONS, false, 0.0);
        UndoableAction * ua =
          arranger_selections_action_new_resize (
            (ArrangerSelections *) TL_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_STRETCH_R,
            ticks_diff);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      {
        ArrangerObject * obj =
          (ArrangerObject *) self->start_object;
        double ticks_diff =
          obj->end_pos.total_ticks -
          obj->transient->end_pos.total_ticks;
        UndoableAction * ua =
          arranger_selections_action_new_resize (
            (ArrangerSelections *) TL_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP,
            ticks_diff);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_FADE:
        {
          ArrangerObject * obj =
            (ArrangerObject *) self->start_object;
          double ticks_diff =
            obj->fade_out_pos.total_ticks -
            obj->transient->fade_out_pos.total_ticks;
          UndoableAction * ua =
            arranger_selections_action_new_resize (
              (ArrangerSelections *) TL_SELECTIONS,
              ARRANGER_SELECTIONS_ACTION_RESIZE_R_FADE,
              ticks_diff);
          undo_manager_perform (
            UNDO_MANAGER, ua);
        }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      /* if something was clicked with ctrl without
       * moving*/
      if (self->ctrl_held)
        {
          if (self->start_object &&
              self->start_object_was_selected)
            {
              /* deselect it */
              arranger_object_select (
                self->start_object,
                F_NO_SELECT, F_APPEND);
            }
        }
      else if (self->n_press == 2)
        {
          /* double click on object */
          /*g_message ("DOUBLE CLICK");*/
        }
      break;
    case UI_OVERLAY_ACTION_MOVING:
      {
        ArrangerObject * obj =
          (ArrangerObject *) self->start_object;
        double ticks_diff =
          obj->pos.total_ticks -
          obj->transient->pos.total_ticks;
        UndoableAction * ua =
          arranger_selections_action_new_move_timeline (
            TL_SELECTIONS, ticks_diff,
            self->visible_track_diff,
            self->lane_diff, F_ALREADY_MOVED);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      {
        ArrangerObject * obj =
          (ArrangerObject *) self->start_object;
        double ticks_diff =
          obj->pos.total_ticks -
          obj->transient->pos.total_ticks;
        UndoableAction * ua = NULL;
        if (ACTION_IS (MOVING_COPY))
          {
            ua =
              (UndoableAction *)
              arranger_selections_action_new_duplicate_timeline (
                TL_SELECTIONS, ticks_diff,
                self->visible_track_diff,
                self->lane_diff, F_ALREADY_MOVED);
          }
        else if (ACTION_IS (MOVING_LINK))
          {
            ua =
              (UndoableAction *)
              arranger_selections_action_new_link (
                self->sel_at_start,
                (ArrangerSelections *)
                  TL_SELECTIONS, ticks_diff,
                self->visible_track_diff,
                self->lane_diff, F_ALREADY_MOVED);
          }

        if (ua)
          {
            undo_manager_perform (
              UNDO_MANAGER, ua);
          }
      }
      break;
    case UI_OVERLAY_ACTION_NONE:
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      arranger_selections_clear (
        (ArrangerSelections *) TL_SELECTIONS);
      break;
    /* if something was created */
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      {
        UndoableAction * ua =
          arranger_selections_action_new_create (
            (ArrangerSelections *) TL_SELECTIONS);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      {
        UndoableAction * ua =
          arranger_selections_action_new_delete (
            self->sel_to_delete);
        undo_manager_perform (
          UNDO_MANAGER, ua);
        object_free_w_func_and_null (
          arranger_selections_free_full,
          self->sel_to_delete);
      }
      break;
    case UI_OVERLAY_ACTION_CUTTING:
      {
        /* get cut position */
        Position cut_pos;
        position_set_to_pos (
          &cut_pos, &self->curr_pos);

        if (SNAP_GRID_ANY_SNAP (
              self->snap_grid) &&
            !self->shift_held)
          position_snap_simple (
            &cut_pos, self->snap_grid);
        UndoableAction * ua =
          arranger_selections_action_new_split (
            (ArrangerSelections *) TL_SELECTIONS,
            &cut_pos);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    default:
      break;
    }

  self->resizing_range = 0;
  self->resizing_range_start = 0;
  self->visible_track_diff = 0;
  self->lane_diff = 0;
}

static void
drag_end (
  GtkGestureDrag *gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  ArrangerWidget * self)
{
  if (ACTION_IS (SELECTING) ||
      ACTION_IS (DELETE_SELECTING))
    {
      EVENTS_PUSH (
        ET_SELECTING_IN_ARRANGER, self);
    }

#undef ON_DRAG_END
  switch (self->type)
    {
    case TYPE (TIMELINE):
      on_drag_end_timeline (self);
      break;
    case TYPE (AUDIO):
      break;
    case TYPE (MIDI):
      on_drag_end_midi (self);
      break;
    case TYPE (MIDI_MODIFIER):
      on_drag_end_midi_modifier (self);
      break;
    case TYPE (CHORD):
      on_drag_end_chord (self);
      break;
    case TYPE (AUTOMATION):
      on_drag_end_automation (self);
      break;
    default:
      break;
    }

  /* if clicked on nothing */
  if (ACTION_IS (STARTING_SELECTION))
    {
      /* deselect all */
      g_message (
        "clicked on empty space, deselecting all");
      arranger_widget_select_all (self, 0);
      /*arranger_widget_update_visibility (self);*/
    }

  /* reset start coordinates and offsets */
  self->start_x = 0;
  self->start_y = 0;
  self->last_offset_x = 0;
  self->last_offset_y = 0;
  self->last_adj_ticks_diff = 0;
  self->start_object = NULL;

  self->shift_held = 0;
  self->ctrl_held = 0;

  if (self->sel_at_start)
    {
      arranger_selections_free_full (
        self->sel_at_start);
      self->sel_at_start = NULL;
    }

  /* reset action */
  self->action = UI_OVERLAY_ACTION_NONE;

  /* queue redraw to hide the selection */
  /*gtk_widget_queue_draw (GTK_WIDGET (self->bg));*/

  arranger_widget_redraw_whole (self);
  arranger_widget_refresh_cursor (self);
}

#if 0
/**
 * @param type The arranger object type, or -1 to
 *   search for all types.
 */
static ArrangerObject *
get_hit_timeline_object (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  double             x,
  double             y)
{
  AutomationTrack * at =
    timeline_arranger_widget_get_at_at_y (
      self, y);
  TrackLane * lane = NULL;
  if (!at)
    lane =
      timeline_arranger_widget_get_track_lane_at_y (
        self, y);
  Track * track = NULL;

  /* get the position of x */
  Position pos;
  arranger_widget_px_to_pos (
    self, x, &pos, 1);

  /* check for hit automation regions */
  if (at)
    {
      if (type != ARRANGER_OBJECT_TYPE_ALL &&
          type != ARRANGER_OBJECT_TYPE_REGION)
        return NULL;

      /* return if any region matches the
       * position */
      for (int i = 0; i < at->num_regions; i++)
        {
          ZRegion * r = at->regions[i];
          if (region_is_hit (r, pos.frames, 1))
            {
              return (ArrangerObject *) r;
            }
        }
    }
  /* check for hit regions in lane */
  else if (lane)
    {
      if (type >= ARRANGER_OBJECT_TYPE_ALL &&
          type != ARRANGER_OBJECT_TYPE_REGION)
        return NULL;

      /* return if any region matches the
       * position */
      for (int i = 0; i < lane->num_regions; i++)
        {
          ZRegion * r = lane->regions[i];
          if (region_is_hit (r, pos.frames, 1))
            {
              return (ArrangerObject *) r;
            }
        }
    }
  /* check for hit regions in main track */
  else
    {
      track =
        timeline_arranger_widget_get_track_at_y (
          self, y);

      if (track)
        {
          for (int i = 0; i < track->num_lanes; i++)
            {
              lane = track->lanes[i];
              for (int j = 0; j < lane->num_regions;
                   j++)
                {
                  ZRegion * r = lane->regions[j];
                  if (region_is_hit (
                        r, pos.frames, 1))
                    {
                      return (ArrangerObject *) r;
                    }
                }
            }
        }
    }

  return NULL;
}
#endif

/**
 * Returns the ArrangerObject of the given type
 * at (x,y).
 *
 * @param type The arranger object type, or -1 to
 *   search for all types.
 */
ArrangerObject *
arranger_widget_get_hit_arranger_object (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  double             x,
  double             y)
{
  ArrangerObject * objs[800];
  int              num_objs;
  arranger_widget_get_hit_objects_at_point (
    self, type, x, y, objs, &num_objs);
  if (num_objs > 0)
    {
      return objs[num_objs - 1];
    }
  else
    {
      return NULL;
    }

  /*switch (self->type)*/
    /*{*/
    /*case TYPE (TIMELINE):*/
      /*return*/
        /*get_hit_timeline_object (self, type, x, y);*/
      /*break;*/
    /*default:*/
      /*break;*/
    /*}*/
  /*return NULL;*/
}

/**
 * Wrapper of the UI functions based on the arranger
 * type.
 */
int
arranger_widget_pos_to_px (
  ArrangerWidget * self,
  Position * pos,
  int        use_padding)
{
  if (self->type == TYPE (TIMELINE))
    {
      return
        ui_pos_to_px_timeline (pos, use_padding);
    }
  else
    {
      return
        ui_pos_to_px_editor (pos, use_padding);
    }

  g_return_val_if_reached (-1);
}

/**
 * Returns the ArrangerSelections for this
 * ArrangerWidget.
 */
ArrangerSelections *
arranger_widget_get_selections (
  ArrangerWidget * self)
{
  switch (self->type)
    {
    case TYPE (TIMELINE):
      return
        (ArrangerSelections *) TL_SELECTIONS;
    case TYPE (MIDI):
    case TYPE (MIDI_MODIFIER):
      return
        (ArrangerSelections *) MA_SELECTIONS;
    case TYPE (AUTOMATION):
      return
        (ArrangerSelections *)
        AUTOMATION_SELECTIONS;
    case TYPE (CHORD):
      return
        (ArrangerSelections *) CHORD_SELECTIONS;
    case TYPE (AUDIO):
      /* FIXME */
      return
        (ArrangerSelections *) TL_SELECTIONS;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Sets transient object and actual object
 * visibility for every ArrangerObject in the
 * ArrangerWidget based on the current action.
 */
/*void*/
/*arranger_widget_update_visibility (*/
  /*ArrangerWidget * self)*/
/*{*/
  /*GList *children, *iter;*/
  /*ArrangerObjectWidget * aow;*/

/*#define UPDATE_VISIBILITY(x) \*/
  /*children = \*/
    /*gtk_container_get_children ( \*/
      /*GTK_CONTAINER (x)); \*/
  /*aow = NULL; \*/
  /*for (iter = children; \*/
       /*iter != NULL; \*/
       /*iter = g_list_next (iter)) \*/
    /*{ \*/
      /*if (!Z_IS_ARRANGER_OBJECT_WIDGET ( \*/
            /*iter->data)) \*/
        /*continue; \*/
 /*\*/
      /*aow = \*/
        /*Z_ARRANGER_OBJECT_WIDGET (iter->data); \*/
      /*ARRANGER_OBJECT_WIDGET_GET_PRIVATE (aow); \*/
      /*g_warn_if_fail (ao_prv->arranger_object); \*/
      /*arranger_object_set_widget_visibility_and_state ( \*/
        /*ao_prv->arranger_object, 1); \*/
    /*} \*/
  /*g_list_free (children);*/

  /*UPDATE_VISIBILITY (self);*/

  /* if midi arranger, do the same for midi modifier
   * arranger, and vice versa */
  /*if (Z_IS_MIDI_ARRANGER_WIDGET (self))*/
    /*{*/
      /*UPDATE_VISIBILITY (MW_MIDI_MODIFIER_ARRANGER);*/
    /*}*/
  /*else if (Z_IS_MIDI_MODIFIER_ARRANGER_WIDGET (self))*/
    /*{*/
      /*UPDATE_VISIBILITY (MW_MIDI_ARRANGER);*/
    /*}*/

/*#undef UPDATE_VISIBILITY*/
/*}*/

/**
 * Gets the corresponding scrolled window.
 */
GtkScrolledWindow *
arranger_widget_get_scrolled_window (
  ArrangerWidget * self)
{
  switch (self->type)
    {
    case TYPE (TIMELINE):
      if (self->is_pinned)
        {
          return
            MW_TIMELINE_PANEL->
              pinned_timeline_scroll;
        }
      else
        {
          return
            MW_TIMELINE_PANEL->timeline_scroll;
        }
    case TYPE (MIDI):
      return MW_MIDI_EDITOR_SPACE->arranger_scroll;
    case TYPE (MIDI_MODIFIER):
      return MW_MIDI_EDITOR_SPACE->
        modifier_arranger_scroll;
    case TYPE (AUDIO):
      return MW_AUDIO_EDITOR_SPACE->arranger_scroll;
    case TYPE (CHORD):
      return MW_CHORD_EDITOR_SPACE->arranger_scroll;
    case TYPE (AUTOMATION):
      return MW_AUTOMATION_EDITOR_SPACE->
        arranger_scroll;
    }

  return NULL;
}

/**
 * Get all objects currently present in the
 * arranger.
 *
 * @param objs Array to fill in.
 * @param size Array size to fill in.
 */
void
arranger_widget_get_all_objects (
  ArrangerWidget *  self,
  ArrangerObject ** objs,
  int *             size)
{
  GdkRectangle rect = {
    0, 0,
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self)),
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self)),
  };

  arranger_widget_get_hit_objects_in_rect (
    self, ARRANGER_OBJECT_TYPE_ALL, &rect,
    objs, size);
}

#if 0
static gboolean
arranger_tick_cb (
  GtkWidget *      widget,
  GdkFrameClock *  frame_clock,
  ArrangerWidget * self)
{
  gint64 frame_time =
    gdk_frame_clock_get_frame_time (frame_clock);

  if (gtk_widget_get_visible (widget) &&
      (frame_time - self->last_frame_time) >
        15000)
    {
      /* figure out the area to draw */
      GtkScrolledWindow * scroll =
        arranger_widget_get_scrolled_window (self);
      GtkAdjustment * xadj =
        gtk_scrolled_window_get_hadjustment (
          scroll);
      double x =
        gtk_adjustment_get_value (xadj);
      GtkAdjustment * yadj =
        gtk_scrolled_window_get_vadjustment (
          scroll);
      double y =
        gtk_adjustment_get_value (yadj);
      int height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (scroll));
      int width =
        gtk_widget_get_allocated_width (
          GTK_WIDGET (scroll));

      /* redraw visible area */
      gtk_widget_queue_draw_area (
        widget, (int) x, (int) y, width, height);

      self->last_frame_time = frame_time;
    }

  return G_SOURCE_CONTINUE;
}
#endif

/**
 * Returns the current visible rectangle.
 *
 * @param rect The rectangle to fill in.
 */
void
arranger_widget_get_visible_rect (
  ArrangerWidget * self,
  GdkRectangle *   rect)
{
  GtkScrolledWindow * scroll =
    arranger_widget_get_scrolled_window (self);
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
 * Queues a redraw of the whole visible arranger.
 */
void
arranger_widget_redraw_whole (
  ArrangerWidget * self)
{
  GdkRectangle rect;
  arranger_widget_get_visible_rect (self, &rect);

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
arranger_widget_redraw_playhead (
  ArrangerWidget * self)
{
  GdkRectangle rect;
  arranger_widget_get_visible_rect (self, &rect);

  int playhead_x = get_playhead_px (self);
  int min_x =
    MIN (self->last_playhead_px, playhead_x);
  min_x = MAX (min_x - 4, rect.x);
  int max_x =
    MAX (self->last_playhead_px, playhead_x);
  max_x = MIN (max_x + 4, rect.x + rect.width);

  /* skip if playhead is not in the visible
   * rectangle */
  int width = max_x - min_x;
  if (width < 0)
    {
      g_message (
        "playhead not currently visible in "
        "arranger, skipping redraw");
      return;
    }

  gtk_widget_queue_draw_area (
    GTK_WIDGET (self), min_x, rect.y,
    (max_x - min_x), rect.height);
}

/**
 * Only redraws the given rectangle.
 */
void
arranger_widget_redraw_rectangle (
  ArrangerWidget * self,
  GdkRectangle *   rect)
{
  gtk_widget_queue_draw_area (
    GTK_WIDGET (self), rect->x, rect->y,
    rect->width, rect->height);
}

static gboolean
on_scroll (
  GtkWidget *widget,
  GdkEventScroll  *event,
  ArrangerWidget * self)
{
  if (!(event->state & GDK_CONTROL_MASK))
    return FALSE;

  double x = event->x,
         /*y = event->y,*/
         adj_val,
         diff;
  Position cursor_pos;
   GtkScrolledWindow * scroll =
    arranger_widget_get_scrolled_window (self);
  GtkAdjustment * adj;
  int new_x;
  RulerWidget * ruler =
    self->type == TYPE (TIMELINE) ?
    MW_RULER : EDITOR_RULER;

  /* get current adjustment so we can get the
   * difference from the cursor */
  adj = gtk_scrolled_window_get_hadjustment (
    scroll);
  adj_val = gtk_adjustment_get_value (adj);

  /* get positions of cursor */
  arranger_widget_px_to_pos (
    self, x, &cursor_pos, 1);

  /* get px diff so we can calculate the new
   * adjustment later */
  diff = x - adj_val;

  /* scroll down, zoom out */
  if (event->delta_y > 0)
    {
      ruler_widget_set_zoom_level (
        ruler,
        ruler->zoom_level / 1.3);
    }
  else /* scroll up, zoom in */
    {
      ruler_widget_set_zoom_level (
        ruler,
        ruler->zoom_level * 1.3);
    }

  new_x = arranger_widget_pos_to_px (
    self, &cursor_pos, 1);

  /* refresh relevant widgets */
  if (self->type == TYPE (TIMELINE))
    timeline_minimap_widget_refresh (
      MW_TIMELINE_MINIMAP);

  /* get updated adjustment and set its value
   at the same offset as before */
  adj =
    gtk_scrolled_window_get_hadjustment (scroll);
  gtk_adjustment_set_value (adj, new_x - diff);

  return TRUE;
}

/**
 * Motion callback.
 */
static gboolean
on_motion (
  GtkWidget *      widget,
  GdkEventMotion * event,
  ArrangerWidget * self)
{
  self->hover_x = event->x;
  self->hover_y = event->y;

  GdkModifierType state;
  int has_state =
    gtk_get_current_event_state (&state);
  if (has_state)
    {
      self->alt_held =
        state & GDK_MOD1_MASK;
      self->ctrl_held =
        state & GDK_CONTROL_MASK;
      self->shift_held =
        state & GDK_SHIFT_MASK;
    }

  /* highlight hovered object */
  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      self, ARRANGER_OBJECT_TYPE_ALL,
      self->hover_x, self->hover_y);
  if (self->hovered_object != obj)
    {
      g_warn_if_fail (
        !self->hovered_object ||
        IS_ARRANGER_OBJECT (self->hovered_object));
      ArrangerObject * prev_obj =
        self->hovered_object;
      self->hovered_object = obj;

      /* redraw previous hovered object to
       * unhover it */
      if (prev_obj)
        arranger_object_queue_redraw (prev_obj);

      /* redraw new hovered object */
      if (obj)
        arranger_object_queue_redraw (obj);
    }

  arranger_widget_refresh_cursor (self);

  switch (self->type)
    {
    case TYPE (TIMELINE):
      timeline_arranger_widget_set_cut_lines_visible (
        self);
      break;
    case TYPE (CHORD):
      if (event->type == GDK_LEAVE_NOTIFY)
        self->hovered_chord_index = -1;
      else
        self->hovered_chord_index =
          chord_arranger_widget_get_chord_at_y (
            event->y);
      break;
    case TYPE (MIDI):
      if (event->type == GDK_LEAVE_NOTIFY)
        {
          midi_arranger_widget_set_hovered_note (
            self, -1);
        }
      else
        {
          midi_arranger_widget_set_hovered_note (
            self,
            piano_roll_keys_widget_get_key_from_y (
              MW_PIANO_ROLL_KEYS, event->y));
        }
      break;
    default:
      break;
    }

  return FALSE;
}

static gboolean
on_focus (
  GtkWidget * widget,
  gpointer    user_data)
{
  MAIN_WINDOW->last_focused = widget;

  g_message ("FOCUSED");

  return FALSE;
}

static gboolean
on_focus_out (GtkWidget *widget,
               GdkEvent  *event,
               ArrangerWidget * self)
{
  g_message ("arranger focus out");

  self->alt_held = 0;
  self->ctrl_held = 0;
  self->shift_held = 0;

  return FALSE;
}

static gboolean
on_grab_broken (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
  /*GdkEventGrabBroken * ev =*/
    /*(GdkEventGrabBroken *) event;*/
  g_message ("arranger grab broken");
  return FALSE;
}

void
arranger_widget_setup (
  ArrangerWidget *   self,
  ArrangerWidgetType type,
  SnapGrid *         snap_grid)
{
  g_return_if_fail (
    self &&
    type >= ARRANGER_WIDGET_TYPE_TIMELINE &&
    snap_grid);
  self->type = type;
  self->snap_grid = snap_grid;

  /* connect signals */
  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (self->drag), "cancel",
    G_CALLBACK (drag_cancel), self);
  g_signal_connect (
    G_OBJECT (self->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT (self), "scroll-event",
    G_CALLBACK (on_scroll), self);
  g_signal_connect (
    G_OBJECT (self), "key-press-event",
    G_CALLBACK (arranger_widget_on_key_action),
    self);
  g_signal_connect (
    G_OBJECT (self), "key-release-event",
    G_CALLBACK (arranger_widget_on_key_release),
    self);
  g_signal_connect (
    G_OBJECT(self), "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self), "leave-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (self), "focus-out-event",
    G_CALLBACK (on_focus_out), self);
  g_signal_connect (
    G_OBJECT (self), "grab-focus",
    G_CALLBACK (on_focus), self);
  g_signal_connect (
    G_OBJECT (self), "grab-broken-event",
    G_CALLBACK (on_grab_broken), self);
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (arranger_draw_cb), self);

  /*gtk_widget_add_tick_callback (*/
    /*GTK_WIDGET (self),*/
    /*(GtkTickCallback) arranger_tick_cb,*/
    /*self, NULL);*/

  gtk_widget_set_focus_on_click (
    GTK_WIDGET (self), 1);
}

/**
 * Wrapper for ui_px_to_pos depending on the arranger
 * type.
 */
void
arranger_widget_px_to_pos (
  ArrangerWidget * self,
  double           px,
  Position *       pos,
  int              has_padding)
{
  if (self->type == TYPE (TIMELINE))
    {
      ui_px_to_pos_timeline (
        px, pos, has_padding);
    }
  else
    {
      ui_px_to_pos_editor (
        px, pos, has_padding);
    }
}

static ArrangerCursor
get_audio_arranger_cursor (
  ArrangerWidget * self,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action =
    self->action;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (P_TOOL == TOOL_SELECT_NORMAL)
        {
          RegionWidget * rw = NULL;

          int is_hit =
            rw != NULL;
          int is_resize_l = 0;
          int is_resize_r = 0;

          if (is_hit && is_resize_l)
            {
              return ARRANGER_CURSOR_RESIZING_L;
            }
          else if (is_hit && is_resize_r)
            {
              return ARRANGER_CURSOR_RESIZING_R;
            }
          else if (is_hit)
            {
              return ARRANGER_CURSOR_GRAB;
            }
          else
            {
              /* set cursor to normal */
              return ARRANGER_CURSOR_SELECT;
            }
        }
      else if (P_TOOL == TOOL_SELECT_STRETCH)
        {
          /* set cursor to normal */
          return ARRANGER_CURSOR_SELECT;
        }
      else if (P_TOOL == TOOL_EDIT)
        ac = ARRANGER_CURSOR_EDIT;
      else if (P_TOOL == TOOL_ERASER)
        ac = ARRANGER_CURSOR_ERASER;
      else if (P_TOOL == TOOL_RAMP)
        ac = ARRANGER_CURSOR_RAMP;
      else if (P_TOOL == TOOL_AUDITION)
        ac = ARRANGER_CURSOR_AUDITION;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

static ArrangerCursor
get_midi_modifier_arranger_cursor (
  ArrangerWidget * self,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action =
    self->action;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (tool == TOOL_SELECT_NORMAL ||
          tool == TOOL_SELECT_STRETCH ||
          tool == TOOL_EDIT)
        {
          ArrangerObject * vel_obj =
            arranger_widget_get_hit_arranger_object (
              (ArrangerWidget *) self,
              ARRANGER_OBJECT_TYPE_VELOCITY,
              self->hover_x, self->hover_y);
          int is_hit = vel_obj != NULL;

          if (is_hit)
            {
              int is_resize =
                arranger_object_is_resize_up (
                  vel_obj,
                  (int) self->hover_x -
                    vel_obj->full_rect.x,
                  (int) self->hover_y -
                    vel_obj->full_rect.y);
              if (is_resize)
                {
                  return
                    ARRANGER_CURSOR_RESIZING_UP;
                }
            }

          /* set cursor to whatever it is */
          if (tool == TOOL_EDIT)
            return ARRANGER_CURSOR_EDIT;
          else
            return ARRANGER_CURSOR_SELECT;
        }
      else if (P_TOOL == TOOL_EDIT)
        ac = ARRANGER_CURSOR_EDIT;
      else if (P_TOOL == TOOL_ERASER)
        ac = ARRANGER_CURSOR_ERASER;
      else if (P_TOOL == TOOL_RAMP)
        ac = ARRANGER_CURSOR_RAMP;
      else if (P_TOOL == TOOL_AUDITION)
        ac = ARRANGER_CURSOR_AUDITION;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      ac = ARRANGER_CURSOR_RESIZING_UP;
      break;
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
    case UI_OVERLAY_ACTION_SELECTING:
      ac = ARRANGER_CURSOR_SELECT;
      /* TODO depends on tool */
      break;
    case UI_OVERLAY_ACTION_STARTING_RAMP:
    case UI_OVERLAY_ACTION_RAMPING:
      ac = ARRANGER_CURSOR_RAMP;
      break;
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;

}

static ArrangerCursor
get_chord_arranger_cursor (
  ArrangerWidget * self,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action =
    self->action;

  int is_hit =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_CHORD_OBJECT,
      self->hover_x, self->hover_y) != NULL;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        {
          if (is_hit)
            {
              return ARRANGER_CURSOR_GRAB;
            }
          else
            {
              /* set cursor to normal */
              return ARRANGER_CURSOR_SELECT;
            }
        }
          break;
        case TOOL_SELECT_STRETCH:
          break;
        case TOOL_EDIT:
          ac = ARRANGER_CURSOR_EDIT;
          break;
        case TOOL_CUT:
          ac = ARRANGER_CURSOR_CUT;
          break;
        case TOOL_ERASER:
          ac = ARRANGER_CURSOR_ERASER;
          break;
        case TOOL_RAMP:
          ac = ARRANGER_CURSOR_RAMP;
          break;
        case TOOL_AUDITION:
          ac = ARRANGER_CURSOR_AUDITION;
          break;
        }
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

static ArrangerCursor
get_automation_arranger_cursor (
  ArrangerWidget * self,
  Tool             tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action =
    self->action;

  int is_hit =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_AUTOMATION_POINT,
      self->hover_x, self->hover_y) != NULL;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
          {
            if (is_hit)
              {
                return ARRANGER_CURSOR_GRAB;
              }
            else
              {
                /* set cursor to normal */
                return ARRANGER_CURSOR_SELECT;
              }
          }
          break;
        case TOOL_SELECT_STRETCH:
          break;
        case TOOL_EDIT:
          ac = ARRANGER_CURSOR_EDIT;
          break;
        case TOOL_CUT:
          ac = ARRANGER_CURSOR_CUT;
          break;
        case TOOL_ERASER:
          ac = ARRANGER_CURSOR_ERASER;
          break;
        case TOOL_RAMP:
          ac = ARRANGER_CURSOR_RAMP;
          break;
        case TOOL_AUDITION:
          ac = ARRANGER_CURSOR_AUDITION;
          break;
        }
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

static ArrangerCursor
get_timeline_cursor (
  ArrangerWidget * self,
  Tool             tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action =
    self->action;

  ArrangerObject * r_obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_REGION,
      self->hover_x, self->hover_y);
  ArrangerObject * s_obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_SCALE_OBJECT,
      self->hover_x, self->hover_y);
  ArrangerObject * m_obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_MARKER,
      self->hover_x, self->hover_y);

  int is_hit = r_obj || s_obj || m_obj;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        case TOOL_SELECT_STRETCH:
        {
          if (is_hit)
            {
              if (r_obj)
                {
                  if (self->alt_held)
                    return ARRANGER_CURSOR_CUT;
                  int is_fade_in_point =
                    arranger_object_is_fade_in (
                      r_obj,
                      (int) self->hover_x -
                        r_obj->full_rect.x,
                      (int) self->hover_y -
                        r_obj->full_rect.y, 1, 0);
                  int is_fade_out_point =
                    arranger_object_is_fade_out (
                      r_obj,
                      (int) self->hover_x -
                        r_obj->full_rect.x,
                      (int) self->hover_y -
                        r_obj->full_rect.y, 1, 0);
                  int is_fade_in_outer_region =
                    arranger_object_is_fade_in (
                      r_obj,
                      (int) self->hover_x -
                        r_obj->full_rect.x,
                      (int) self->hover_y -
                        r_obj->full_rect.y, 0, 1);
                  int is_fade_out_outer_region =
                    arranger_object_is_fade_out (
                      r_obj,
                      (int) self->hover_x -
                        r_obj->full_rect.x,
                      (int) self->hover_y -
                        r_obj->full_rect.y, 0, 1);
                  int is_resize_l =
                    arranger_object_is_resize_l (
                      r_obj,
                      (int) self->hover_x -
                        r_obj->full_rect.x);
                  int is_resize_r =
                    arranger_object_is_resize_r (
                      r_obj,
                      (int) self->hover_x -
                        r_obj->full_rect.x);
                  int is_resize_loop =
                    arranger_object_is_resize_loop (
                      r_obj,
                      (int) self->hover_y -
                        r_obj->full_rect.y);
                  if (is_fade_in_point)
                    return
                      ARRANGER_CURSOR_FADE_IN;
                  else if (is_fade_out_point)
                    return
                      ARRANGER_CURSOR_FADE_OUT;
                  else if (is_resize_l &&
                           is_resize_loop)
                    {
                      return
                        ARRANGER_CURSOR_RESIZING_L_LOOP;
                    }
                  else if (is_resize_l)
                    {
                      if (P_TOOL ==
                           TOOL_SELECT_NORMAL)
                        return
                          ARRANGER_CURSOR_RESIZING_L;
                      else if (P_TOOL ==
                                 TOOL_SELECT_STRETCH)
                        {
                          return
                            ARRANGER_CURSOR_STRETCHING_L;
                        }
                    }
                  else if (is_resize_r &&
                           is_resize_loop)
                    return
                      ARRANGER_CURSOR_RESIZING_R_LOOP;
                  else if (is_resize_r)
                    {
                      if (P_TOOL ==
                           TOOL_SELECT_NORMAL)
                        return
                          ARRANGER_CURSOR_RESIZING_R;
                      else if (P_TOOL ==
                                 TOOL_SELECT_STRETCH)
                        return
                          ARRANGER_CURSOR_STRETCHING_R;
                    }
                  else if (is_fade_in_outer_region)
                    return
                      ARRANGER_CURSOR_FADE_IN;
                  else if (is_fade_out_outer_region)
                    return
                      ARRANGER_CURSOR_FADE_OUT;
                }
              return ARRANGER_CURSOR_GRAB;
            }
          else
            {
              Track * track =
                timeline_arranger_widget_get_track_at_y (
                self, self->hover_y);

              if (track)
                {
                  if (track_widget_is_cursor_in_top_half (
                        track->widget,
                        self->hover_y))
                    {
                      /* set cursor to normal */
                      return ARRANGER_CURSOR_SELECT;
                    }
                  else
                    {
                      /* set cursor to range selection */
                      return ARRANGER_CURSOR_RANGE;
                    }
                }
              else
                {
                  /* set cursor to normal */
                  return ARRANGER_CURSOR_SELECT;
                }
            }
        }
          break;
        case TOOL_EDIT:
          ac = ARRANGER_CURSOR_EDIT;
          break;
        case TOOL_CUT:
          ac = ARRANGER_CURSOR_CUT;
          break;
        case TOOL_ERASER:
          ac = ARRANGER_CURSOR_ERASER;
          break;
        case TOOL_RAMP:
          ac = ARRANGER_CURSOR_RAMP;
          break;
        case TOOL_AUDITION:
          ac = ARRANGER_CURSOR_AUDITION;
          break;
        }
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STRETCHING_L:
      ac = ARRANGER_CURSOR_STRETCHING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      if (self->resizing_range)
        ac = ARRANGER_CURSOR_RANGE;
      else
        ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
      ac = ARRANGER_CURSOR_RESIZING_L_LOOP;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_FADE:
      ac = ARRANGER_CURSOR_FADE_IN;
      break;
    case UI_OVERLAY_ACTION_STRETCHING_R:
      ac = ARRANGER_CURSOR_STRETCHING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      if (self->resizing_range)
        ac = ARRANGER_CURSOR_RANGE;
      else
        ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      ac = ARRANGER_CURSOR_RESIZING_R_LOOP;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_FADE:
      ac = ARRANGER_CURSOR_FADE_OUT;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN:
      ac = ARRANGER_CURSOR_FADE_IN;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT:
      ac = ARRANGER_CURSOR_FADE_OUT;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

static ArrangerCursor
get_midi_arranger_cursor (
  ArrangerWidget * self,
  Tool             tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action =
    self->action;

  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_MIDI_NOTE,
      self->hover_x, self->hover_y);
  int is_hit = obj != NULL;


  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (tool == TOOL_SELECT_NORMAL ||
          tool == TOOL_SELECT_STRETCH ||
          tool == TOOL_EDIT)
        {
          int is_resize_l = 0, is_resize_r = 0;

          if (is_hit)
            {
              is_resize_l =
                arranger_object_is_resize_l (
                  obj,
                  (int) self->hover_x -
                    obj->full_rect.x);
              is_resize_r =
                arranger_object_is_resize_r (
                  obj,
                  (int) self->hover_x -
                    obj->full_rect.x);
            }

          if (is_hit && is_resize_l &&
              !PIANO_ROLL->drum_mode)
            {
              return ARRANGER_CURSOR_RESIZING_L;
            }
          else if (is_hit && is_resize_r &&
                   !PIANO_ROLL->drum_mode)
            {
              return ARRANGER_CURSOR_RESIZING_R;
            }
          else if (is_hit)
            {
              return ARRANGER_CURSOR_GRAB;
            }
          else
            {
              /* set cursor to whatever it is */
              if (tool == TOOL_EDIT)
                return ARRANGER_CURSOR_EDIT;
              else
                return ARRANGER_CURSOR_SELECT;
            }
        }
      else if (P_TOOL == TOOL_EDIT)
        ac = ARRANGER_CURSOR_EDIT;
      else if (P_TOOL == TOOL_ERASER)
        ac = ARRANGER_CURSOR_ERASER;
      else if (P_TOOL == TOOL_RAMP)
        ac = ARRANGER_CURSOR_RAMP;
      else if (P_TOOL == TOOL_AUDITION)
        ac = ARRANGER_CURSOR_AUDITION;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
    case UI_OVERLAY_ACTION_SELECTING:
      ac = ARRANGER_CURSOR_SELECT;
      /* TODO depends on tool */
      break;
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

/**
 * Figures out which cursor should be used based
 * on the current state and then sets it.
 */
void
arranger_widget_refresh_cursor (
  ArrangerWidget * self)
{
  if (!gtk_widget_get_realized (
        GTK_WIDGET (self)))
    return;

  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

  switch (self->type)
    {
    case TYPE (TIMELINE):
      ac =
        get_timeline_cursor (self, P_TOOL);
      break;
    case TYPE (AUDIO):
      ac = get_audio_arranger_cursor (self, P_TOOL);
      break;
    case TYPE (CHORD):
      ac = get_chord_arranger_cursor (self, P_TOOL);
      break;
    case TYPE (MIDI):
      ac = get_midi_arranger_cursor (self, P_TOOL);
      break;
    case TYPE (MIDI_MODIFIER):
      ac =
        get_midi_modifier_arranger_cursor (
          self, P_TOOL);
      break;
    case TYPE (AUTOMATION):
      ac =
        get_automation_arranger_cursor (
          self, P_TOOL);
      break;
    default:
      break;
    }

  arranger_widget_set_cursor (self, ac);
}

/**
 * Toggles the mute status of the selection, based
 * on the mute status of the selected object.
 *
 * This creates an undoable action and executes it.
 */
void
arranger_widget_toggle_selections_muted (
  ArrangerWidget * self,
  ArrangerObject * clicked_object)
{
  g_return_if_fail (
    arranger_object_can_mute (clicked_object));

  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (MAIN_WINDOW),
      "mute-selection");
  GVariant * var =
    g_variant_new_string ("timeline");
  g_action_activate (action, var);
  g_free (var);
}

/**
 * Scroll until the given object is visible.
 *
 * @param horizontal 1 for horizontal, 2 for
 *   vertical.
 * @param up Whether scrolling up or down.
 * @param padding Padding pixels.
 */
void
arranger_widget_scroll_until_obj (
  ArrangerWidget * self,
  ArrangerObject * obj,
  int              horizontal,
  int              up,
  int              left,
  double           padding)
{
  GtkScrolledWindow *scroll =
    arranger_widget_get_scrolled_window (self);
  int scroll_width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (scroll));
  int scroll_height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (scroll));
  GtkAdjustment *hadj =
    gtk_scrolled_window_get_hadjustment (
      GTK_SCROLLED_WINDOW (scroll));
  GtkAdjustment *vadj =
    gtk_scrolled_window_get_vadjustment (
      GTK_SCROLLED_WINDOW (scroll));
  double adj_x =
    gtk_adjustment_get_value (hadj);
  double adj_y =
    gtk_adjustment_get_value (vadj);

  if (horizontal)
    {
      double start_px =
        (double)
        arranger_widget_pos_to_px (
          self, &obj->pos, 1);
      double end_px =
        (double)
        arranger_widget_pos_to_px (
          self, &obj->end_pos, 1);

      /* adjust px for objects with non-global
       * positions */
      if (!arranger_object_type_has_global_pos (
            obj->type))
        {
          ArrangerObject * r_obj =
            (ArrangerObject *)
            clip_editor_get_region (CLIP_EDITOR);
          g_return_if_fail (r_obj);
          double tmp_px =
            (double)
            arranger_widget_pos_to_px (
              self, &r_obj->pos, 1);
          start_px += tmp_px;
          end_px += tmp_px;
        }

      if (start_px <= adj_x ||
          end_px >= adj_x + (double) scroll_width)
        {
          if (left)
            {
              gtk_adjustment_set_value (
                hadj, start_px - padding);
            }
          else
            {
              double tmp =
                (end_px + padding) -
                (double) scroll_width;
              gtk_adjustment_set_value (hadj, tmp);
            }
        }
    }
  else
    {
      arranger_object_set_full_rectangle (
        obj, self);
      double start_px = obj->full_rect.y;
      double end_px =
        obj->full_rect.y + obj->full_rect.height;
      if (start_px <= adj_y ||
          end_px >= adj_y + (double) scroll_height)
        {
          if (up)
            {
              gtk_adjustment_set_value (
                vadj, start_px - padding);
            }
          else
            {
              double tmp =
                (end_px + padding) -
                (double) scroll_height;
              gtk_adjustment_set_value (vadj, tmp);
            }
        }
    }
}

/**
 * Returns the earliest possible position allowed
 * in this arranger (eg, 1.1.0.0 for timeline).
 */
void
arranger_widget_get_min_possible_position (
  ArrangerWidget * self,
  Position *       pos)
{
  switch (self->type)
    {
    case TYPE (TIMELINE):
      position_set_to_bar (pos, 1);
      break;
    case TYPE (MIDI):
    case TYPE (MIDI_MODIFIER):
    case TYPE (CHORD):
    case TYPE (AUTOMATION):
    case TYPE (AUDIO):
      {
        ZRegion * region =
          clip_editor_get_region (CLIP_EDITOR);
        g_return_if_fail (region);
        position_set_to_pos (
          pos, &((ArrangerObject *) region)->pos);
        position_change_sign (pos);
      }
      break;
    }
}

static void
arranger_widget_class_init (
  ArrangerWidgetClass * _klass)
{
}

static void
arranger_widget_init (
  ArrangerWidget *self)
{
  /* make widget able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_ALL_EVENTS_MASK);

  /* make widget able to focus */
  gtk_widget_set_can_focus (
    GTK_WIDGET (self), 1);
  gtk_widget_set_focus_on_click (
    GTK_WIDGET (self), 1);

  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->drag),
    GTK_PHASE_CAPTURE);
  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->multipress),
    GTK_PHASE_CAPTURE);
  self->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (
      self->right_mouse_mp),
      GDK_BUTTON_SECONDARY);
}
