/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  RulerWidget,
  ruler_widget,
  GTK_TYPE_OVERLAY)

#define Y_SPACING 5
#define FONT "Monospace"
#define FONT_SIZE 14

     /* FIXME delete these, see ruler_marker.h */
#define START_MARKER_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_WIDTH 8
#define Q_HEIGHT 12
#define Q_WIDTH 7

/**
 * Minimum number of pixels between beat lines.
 */
#define PX_TO_HIDE_BEATS 40.0

#define GET_PRIVATE RULER_WIDGET_GET_PRIVATE (self)

/**
 * Gets each specific ruler.
 *
 * Useful boilerplate
 */
#define GET_RULER_ALIASES(self) \
  TimelineRulerWidget * timeline_ruler = NULL; \
  EditorRulerWidget *     editor_ruler = NULL; \
  if (Z_IS_TIMELINE_RULER_WIDGET (self)) \
    { \
      timeline_ruler = \
        Z_TIMELINE_RULER_WIDGET (self); \
    } \
  else if (Z_IS_EDITOR_RULER_WIDGET (self)) \
    { \
      editor_ruler = Z_EDITOR_RULER_WIDGET (self); \
    }

#define ACTION_IS(x) \
  (rw_prv->action == UI_OVERLAY_ACTION_##x)

/**
 * Gets called to set the position/size of each overlayed widget.
 */
static gboolean
get_child_position (GtkOverlay   *overlay,
                    GtkWidget    *widget,
                    GdkRectangle *allocation,
                    gpointer      user_data)
{
  if (TRANSPORT->lticks_per_bar < 1)
    return FALSE;

  RulerWidget * self =
    Z_RULER_WIDGET (overlay);
  GET_RULER_ALIASES (self);

  if (Z_IS_RULER_RANGE_WIDGET (widget))
    {
      timeline_ruler_widget_set_ruler_range_position (
        Z_TIMELINE_RULER_WIDGET (self),
        Z_RULER_RANGE_WIDGET (widget),
        allocation);
    }
  else if (Z_IS_RULER_MARKER_WIDGET (widget))
    {
      if (timeline_ruler)
        timeline_ruler_widget_set_ruler_marker_position (
          Z_TIMELINE_RULER_WIDGET (self),
          Z_RULER_MARKER_WIDGET (widget),
          allocation);
      else if (editor_ruler)
        editor_ruler_widget_set_ruler_marker_position (
          Z_EDITOR_RULER_WIDGET (self),
          Z_RULER_MARKER_WIDGET (widget),
          allocation);
    }

  return TRUE;
}

RulerWidgetPrivate *
ruler_widget_get_private (RulerWidget * self)
{
  return ruler_widget_get_instance_private (self);
}

int
ruler_widget_get_beat_interval (
  RulerWidget * self)
{
  GET_PRIVATE;

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
      (PX_TO_HIDE_BEATS / rw_prv->px_per_beat),
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
  GET_PRIVATE;

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
      (PX_TO_HIDE_BEATS /
      rw_prv->px_per_sixteenth), 1);

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

static gboolean
ruler_draw_cb (
  GtkWidget *   widget,
  cairo_t *     cr,
  RulerWidget * self)
{
  /* engine is run only set after everything is set up
   * so this is a good way to decide if we should draw
   * or not */
  if (!g_atomic_int_get (&AUDIO_ENGINE->run))
    return FALSE;

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (
    cr, &rect);

  GET_PRIVATE;

  if (rw_prv->px_per_bar < 2.0)
    return FALSE;

  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self));

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  gtk_render_background (
    context, cr,
    rect.x, rect.y,
    rect.width, rect.height);

  /* if timeline, draw loop background */
  /* FIXME use rect */
  GET_RULER_ALIASES (self);
  double start_px = 0, end_px = 0;
  if (timeline_ruler)
    {
      start_px =
        ui_pos_to_px_timeline (
          &TRANSPORT->loop_start_pos, 1);
      end_px =
        ui_pos_to_px_timeline (
          &TRANSPORT->loop_end_pos, 1);
    }
  else if (editor_ruler)
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
      cr, 0, 0.9, 0.7, 0.25);
  else
    cairo_set_source_rgba (
      cr, 0.5, 0.5, 0.5, 0.25);
  cairo_set_line_width (cr, 2);
  if (start_px > rect.x)
    {
      cairo_move_to (
        cr, start_px + 1, rect.y);
      cairo_line_to (
        cr, start_px + 1, rect.height);
      cairo_stroke (cr);
    }
  if (end_px < rect.x + rect.width)
    {
      cairo_move_to (
        cr, end_px - 1, rect.y);
      cairo_line_to (
        cr, end_px - 1, rect.height);
      cairo_stroke (cr);
    }
  cairo_pattern_t * pat;
  pat =
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
  cairo_rectangle (
    cr, MAX (rect.x, start_px), rect.y,
    /* FIXME use rect properly, this exceeds */
    end_px - MAX (rect.x, start_px),
    rect.height);
  cairo_set_source (cr, pat);
  cairo_fill (cr);

  PangoLayout * layout =
    pango_cairo_create_layout (cr);
  PangoFontDescription *desc =
    pango_font_description_from_string (
      "Monospace 11");
  pango_layout_set_font_description (layout, desc);
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
    MAX (
      (int)
      (PX_TO_HIDE_BEATS /
         rw_prv->px_per_bar), 1);

  /* draw bars */
  i = - bar_interval;
  while ((curr_px =
          rw_prv->px_per_bar * (i += bar_interval) +
          SPACE_BEFORE_START) <
         rect.x + rect.width)
    {
      if (curr_px < rect.x)
        continue;

      cairo_set_source_rgb (cr, 1, 1, 1);
      cairo_set_line_width (cr, 1);
      cairo_move_to (cr, curr_px, 0);
      cairo_line_to (cr, curr_px, height / 3);
      cairo_stroke (cr);
      cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
      sprintf (text, "%d", i + 1);
      pango_layout_set_markup (layout, text, -1);
      pango_layout_get_pixel_size (
        layout, &textw, &texth);
      cairo_move_to (
        cr, curr_px - textw / 2, height / 3 + 2);
      pango_cairo_update_layout (cr, layout);
      pango_cairo_show_layout (cr, layout);
    }
  /* draw beats */
  desc =
    pango_font_description_from_string (
      "Monospace 6");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);
  i = 0;
  if (beat_interval > 0)
    {
      while ((curr_px =
              rw_prv->px_per_beat *
                (i += beat_interval) +
              SPACE_BEFORE_START) <
             rect.x + rect.width)
        {
          if (curr_px < rect.x)
            continue;

          cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
          cairo_set_line_width (cr, 0.5);
          cairo_move_to (cr, curr_px, 0);
          cairo_line_to (cr, curr_px, height / 4);
          cairo_stroke (cr);
          if ((rw_prv->px_per_beat >
                 PX_TO_HIDE_BEATS * 2) &&
              i % beats_per_bar != 0)
            {
              cairo_set_source_rgb (
                cr, 0.5, 0.5, 0.5);
              sprintf (
                text, "%d.%d",
                i / beats_per_bar + 1,
                i % beats_per_bar + 1);
              pango_layout_set_markup (
                layout, text, -1);
              pango_layout_get_pixel_size (
                layout, &textw, &texth);
              cairo_move_to (
                cr, curr_px - textw / 2,
                height / 4 + 2);
              pango_cairo_update_layout (cr, layout);
              pango_cairo_show_layout (cr, layout);
            }
        }
    }
  /* draw sixteenths */
  i = 0;
  if (sixteenth_interval > 0)
    {
      while ((curr_px =
              rw_prv->px_per_sixteenth *
                (i += sixteenth_interval) +
              SPACE_BEFORE_START) <
             rect.x + rect.width)
        {
          if (curr_px < rect.x)
            continue;

          cairo_set_source_rgb (cr, 0.6, 0.6, 0.6);
          cairo_set_line_width (cr, 0.3);
          cairo_move_to (cr, curr_px, 0);
          cairo_line_to (cr, curr_px, height / 6);
          cairo_stroke (cr);
          if ((rw_prv->px_per_sixteenth >
                 PX_TO_HIDE_BEATS * 2) &&
              i % sixteenths_per_beat != 0)
            {
              cairo_set_source_rgb (
                cr, 0.5, 0.5, 0.5);
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
                cr, curr_px - textw / 2,
                height / 4 + 2);
              pango_cairo_update_layout (cr, layout);
              pango_cairo_show_layout (cr, layout);
            }
        }
    }
  g_object_unref (layout);

 return FALSE;
}

static gboolean
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  RulerWidget *         self)
{
  RULER_WIDGET_GET_PRIVATE (self);
  GET_RULER_ALIASES (self);

  GdkModifierType state_mask;
  ui_get_modifier_type_from_gesture (
    GTK_GESTURE_SINGLE (gesture),
    &state_mask);
  if (state_mask & GDK_SHIFT_MASK)
    rw_prv->shift_held = 1;

  if (n_press == 2)
    {
      if (timeline_ruler)
        {
          Position pos;
          ui_px_to_pos_timeline (
            x, &pos, 1);
          if (!rw_prv->shift_held)
            position_snap_simple (
              &pos, SNAP_GRID_TIMELINE);
          position_set_to_pos (&TRANSPORT->cue_pos,
                               &pos);
        }
      if (editor_ruler)
        {
        }
    }

  return FALSE;
}

static void
drag_begin (GtkGestureDrag *      gesture,
            gdouble               start_x,
            gdouble               start_y,
            RulerWidget *         self)
{
  RULER_WIDGET_GET_PRIVATE (self);
  GET_RULER_ALIASES (self);

  rw_prv->start_x = start_x;

  if (timeline_ruler)
    {
      timeline_ruler->range1_first =
        position_is_before_or_equal (
          &PROJECT->range_1, &PROJECT->range_2);
    }

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  RulerMarkerWidget * hit_marker =
    Z_RULER_MARKER_WIDGET (
      ui_get_hit_child (
        GTK_CONTAINER (self),
        start_x,
        start_y,
        RULER_MARKER_WIDGET_TYPE));

  /* if one of the markers hit */
  if (hit_marker)
    {
      if (timeline_ruler)
        {
          if (hit_marker ==
                timeline_ruler->loop_start)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_LOOP_START;
            }
          else if (hit_marker ==
                   timeline_ruler->loop_end)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_LOOP_END;
            }

        }
      else if (editor_ruler)
        {
          if (hit_marker == editor_ruler->loop_start)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_LOOP_START;
            }
          else if (hit_marker ==
                   editor_ruler->loop_end)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_LOOP_END;
            }
          else if (hit_marker ==
                   editor_ruler->clip_start)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_CLIP_START;
            }
        }
    }
  else
    {
      if (timeline_ruler)
        timeline_ruler_on_drag_begin_no_marker_hit (
          timeline_ruler, start_x, start_y,
          height);
      else if (editor_ruler)
        editor_ruler_on_drag_begin_no_marker_hit (
          editor_ruler, start_x, start_y);
    }
  rw_prv->last_offset_x = 0;
}

static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  RulerWidget *    self)
{
  RULER_WIDGET_GET_PRIVATE (self);
  GET_RULER_ALIASES (self);

  GdkModifierType state_mask;
  ui_get_modifier_type_from_gesture (
    GTK_GESTURE_SINGLE (gesture),
    &state_mask);

  if (state_mask & GDK_SHIFT_MASK)
    rw_prv->shift_held = 1;
  else
    rw_prv->shift_held = 0;

  if (ACTION_IS (STARTING_MOVING))
    {
      rw_prv->action = UI_OVERLAY_ACTION_MOVING;
    }

  if (timeline_ruler)
    {
      timeline_ruler_on_drag_update (
        timeline_ruler, offset_x, offset_y);
    }
  else if (editor_ruler)
    {
      editor_ruler_on_drag_update (
        editor_ruler, offset_x, offset_y);
    }

  rw_prv->last_offset_x = offset_x;

  /* TODO update inspector */
}

static void
drag_end (GtkGestureDrag *gesture,
          gdouble         offset_x,
          gdouble         offset_y,
          RulerWidget *   self)
{
  RULER_WIDGET_GET_PRIVATE (self);
  GET_RULER_ALIASES (self);

  rw_prv->start_x = 0;
  rw_prv->shift_held = 0;

  rw_prv->action = UI_OVERLAY_ACTION_NONE;

  if (timeline_ruler)
    timeline_ruler_on_drag_end (timeline_ruler);
  else if (editor_ruler)
    editor_ruler_on_drag_end (editor_ruler);
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
on_motion (GtkDrawingArea * da,
           GdkEventMotion *event,
           RulerWidget *    self)
{
  if (self != (RulerWidget *) MW_RULER)
    return;

  int height = gtk_widget_get_allocated_height (
    GTK_WIDGET (da));
  if (event->type == GDK_MOTION_NOTIFY)
    {
      /* if lower 3/4ths */
      if (event->y > (height * 1) / 4)
        {
          /* set cursor to normal */
          ui_set_cursor_from_name (GTK_WIDGET (self),
                         "default");
        }
      else /* upper 1/4th */
        {
          /* set cursor to range selection */
          ui_set_cursor_from_name (GTK_WIDGET (self),
                         "text");
        }
    }
}

void
ruler_widget_refresh (RulerWidget * self)
{
  GET_RULER_ALIASES (self);

  if (timeline_ruler)
    timeline_ruler_widget_refresh (
      timeline_ruler);
  else if (editor_ruler)
    editor_ruler_widget_refresh (
      editor_ruler);
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
      RULER_WIDGET_GET_PRIVATE (self);
      rw_prv->zoom_level = zoom_level;
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
  GET_PRIVATE;

  rw_prv->zoom_level = DEFAULT_ZOOM_LEVEL;

  rw_prv->bg =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (rw_prv->bg), 1);
  gtk_container_add (
    GTK_CONTAINER (self), GTK_WIDGET (rw_prv->bg));

  rw_prv->playhead =
    ruler_marker_widget_new (
      self, RULER_MARKER_TYPE_PLAYHEAD);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (rw_prv->playhead));

  /* make it able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (rw_prv->bg), GDK_ALL_EVENTS_MASK);

  rw_prv->drag = GTK_GESTURE_DRAG (
    gtk_gesture_drag_new (GTK_WIDGET (self)));
  rw_prv->multipress = GTK_GESTURE_MULTI_PRESS (
    gtk_gesture_multi_press_new (
      GTK_WIDGET (self)));

  g_signal_connect (
    G_OBJECT (rw_prv->bg), "draw",
    G_CALLBACK (ruler_draw_cb), self);
  g_signal_connect (
    G_OBJECT (rw_prv->bg), "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (rw_prv->bg), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self), "get-child-position",
    G_CALLBACK (get_child_position), NULL);
  g_signal_connect (
    G_OBJECT(rw_prv->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(rw_prv->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(rw_prv->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (self), "grab-broken-event",
    G_CALLBACK (on_grab_broken), self);
  g_signal_connect (
    G_OBJECT (rw_prv->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
}

static void
ruler_widget_class_init (RulerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "ruler");
}
