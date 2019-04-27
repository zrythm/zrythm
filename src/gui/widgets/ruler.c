/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

/**
 * \file
 *
 * Ruler widget.
 */

#include <math.h>

#include "actions/actions.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_clip_editor.h"
#include "gui/widgets/audio_ruler.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_marker.h"
#include "gui/widgets/ruler_range.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (RulerWidget,
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

#define GET_PRIVATE RULER_WIDGET_GET_PRIVATE (self)

/**
 * Gets each specific ruler.
 *
 * Useful boilerplate
 */
#define GET_RULER_ALIASES(self) \
  TimelineRulerWidget * timeline_ruler = NULL; \
  MidiRulerWidget *     midi_ruler = NULL; \
  AudioRulerWidget *    audio_ruler = NULL; \
  if (Z_IS_TIMELINE_RULER_WIDGET (self)) \
    { \
      timeline_ruler = \
        Z_TIMELINE_RULER_WIDGET (self); \
    } \
  else if (Z_IS_MIDI_RULER_WIDGET (self)) \
    { \
      midi_ruler = Z_MIDI_RULER_WIDGET (self); \
    } \
  else if (Z_IS_AUDIO_RULER_WIDGET (self)) \
    { \
      audio_ruler = \
        Z_AUDIO_RULER_WIDGET (self); \
    }

/**
 * Gets called to set the position/size of each overlayed widget.
 */
static gboolean
get_child_position (GtkOverlay   *overlay,
                    GtkWidget    *widget,
                    GdkRectangle *allocation,
                    gpointer      user_data)
{
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
      else if (midi_ruler)
        midi_ruler_widget_set_ruler_marker_position (
          Z_MIDI_RULER_WIDGET (self),
          Z_RULER_MARKER_WIDGET (widget),
          allocation);
      else if (audio_ruler)
        audio_ruler_widget_set_ruler_marker_position (
          Z_AUDIO_RULER_WIDGET (self),
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

static gboolean
ruler_draw_cb (GtkWidget * widget,
         cairo_t *cr,
         RulerWidget * self)
{
  /* engine is run only set after everything is set up
   * so this is a good way to decide if we should draw
   * or not */
  if (!g_atomic_int_get (&AUDIO_ENGINE->run))
    return FALSE;

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);

  GET_PRIVATE;

  if (rw_prv->px_per_bar < 2.0)
    return FALSE;

  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  guint height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr,
                         rect.x, rect.y,
                         rect.width, rect.height);

  /* draw lines */
  int i = 0;
  double curr_px;
  while ((curr_px =
          rw_prv->px_per_bar * i++ +
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
      cairo_select_font_face(cr, FONT,
          CAIRO_FONT_SLANT_NORMAL,
          CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, FONT_SIZE);
      gchar * label =
        g_strdup_printf ("%d", i);
      static cairo_text_extents_t extents;
      cairo_text_extents(cr, label, &extents);
      cairo_move_to (cr,
                     (curr_px ) - extents.width / 2,
                     (height / 2) + Y_SPACING);
      cairo_show_text(cr, label);
    }
  i = 0;
  while ((curr_px =
          rw_prv->px_per_beat * i++ +
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
  }

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
      if (midi_ruler)
        {
        }
      if (audio_ruler)
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
        position_compare (&PROJECT->range_1,
                          &PROJECT->range_2) <= 0;
    }

  guint height =
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
              timeline_ruler->song_start)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_SONG_START;
            }
          else if (hit_marker ==
                   timeline_ruler->song_end)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_SONG_END;
            }
          else if (hit_marker ==
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
      else if (midi_ruler)
        {
          if (hit_marker == midi_ruler->loop_start)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_LOOP_START;
            }
          else if (hit_marker ==
                   midi_ruler->loop_end)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_LOOP_END;
            }
          else if (hit_marker ==
                   midi_ruler->clip_start)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_CLIP_START;
            }
        }
      else if (audio_ruler)
        {
          if (hit_marker == audio_ruler->loop_start)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_LOOP_START;
            }
          else if (hit_marker ==
                   audio_ruler->loop_end)
            {
              rw_prv->action =
                UI_OVERLAY_ACTION_STARTING_MOVING;
              rw_prv->target =
                RW_TARGET_LOOP_END;
            }
          else if (hit_marker ==
                   audio_ruler->clip_start)
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
          gesture, start_x, start_y, timeline_ruler,
          height);
    }
  rw_prv->last_offset_x = 0;
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
            RulerWidget * self)
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

  if (rw_prv->action ==
      UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      rw_prv->action = UI_OVERLAY_ACTION_MOVING;
    }

  if (timeline_ruler)
    timeline_ruler_on_drag_update (
      gesture, offset_x, offset_y, timeline_ruler);
  /* handle x */
  /* if moving the selection */
  else if ((midi_ruler || audio_ruler) &&
           rw_prv->action ==
             UI_OVERLAY_ACTION_MOVING)
    {
      Position tmp;
      Region * r = CLIP_EDITOR->region;

      /* convert px to position */
      if (midi_ruler)
        ui_px_to_pos_piano_roll (
          rw_prv->start_x + offset_x,
          &tmp, 1);
      else if (audio_ruler)
        ui_px_to_pos_audio_clip_editor (
          rw_prv->start_x + offset_x,
          &tmp, 1);

      /* snap if not shift held */
      if (!rw_prv->shift_held)
        position_snap_simple (
          &tmp, SNAP_GRID_MIDI);

      if (rw_prv->target == RW_TARGET_LOOP_START)
        {
          /* if position is acceptable */
          if (position_compare (
                &tmp,
                &r->loop_end_pos) < 0 &&
              position_compare (
                &tmp,
                &r->clip_start_pos) >= 0)
            {
              /* set it */
              position_set_to_pos (
                &r->loop_start_pos,
                &tmp);
              transport_update_position_frames ();
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (rw_prv->target == RW_TARGET_LOOP_END)
        {
          /* if position is acceptable */
          if (position_compare (
                &tmp,
                &r->loop_start_pos) > 0 &&
              position_compare (
                &tmp,
                &r->clip_start_pos) > 0)
            {
              /* set it */
              position_set_to_pos (
                &r->loop_end_pos,
                &tmp);
              transport_update_position_frames ();
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (rw_prv->target ==
                 RW_TARGET_CLIP_START)
        {
          /* if position is acceptable */
          if (position_compare (
                &tmp,
                &r->loop_start_pos) <= 0)
            {
              /* set it */
              position_set_to_pos (
                &r->clip_start_pos,
                &tmp);
              transport_update_position_frames ();
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
    } /* endif MOVING */

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

  rw_prv->start_x = 0;
  rw_prv->shift_held = 0;

  rw_prv->action = UI_OVERLAY_ACTION_NONE;

  if (self == (RulerWidget *) MW_RULER)
    timeline_ruler_on_drag_end ();
}

static void
on_motion (GtkDrawingArea * da,
           GdkEventMotion *event,
           RulerWidget *    self)
{
  /* change statusbar */
  if (event->type == GDK_MOTION_NOTIFY)
    {
      if (self == (RulerWidget *) MW_RULER)
        bot_bar_change_status (
          "Timeline Ruler - Click and drag on a "
          "marker to change its position - Click "
          "and drag on the bot half to change the "
          "playhead position - Click and drag on "
          "the top half to create a range "
          "selection (Use Shift to bypass "
          "snapping)");
      else if (self == (RulerWidget *) MIDI_RULER)
        bot_bar_change_status (
          "Clip Editor Ruler - Click and drag on "
          "a marker to change its position (Use "
          "Shift to bypass snapping)");
      else if (self == (RulerWidget *) AUDIO_RULER)
        bot_bar_change_status (
          "Clip Editor Ruler - Click and drag on "
          "a marker to change its position (Use "
          "Shift to bypass snapping)");
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    bot_bar_change_status ("");


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
    timeline_ruler_widget_refresh ();
  else if (midi_ruler)
    midi_ruler_widget_refresh ();
  else if (audio_ruler)
    audio_ruler_widget_refresh ();
}

/**
 * FIXME move to somewhere else, maybe project.
 * Sets zoom level and disables/enables buttons
 * accordingly.
 *
 * Returns if the zoom level was set or not.
 */
int
ruler_widget_set_zoom_level (RulerWidget * self,
                             float         zoom_level)
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
      action_enable_window_action ("zoom-out");
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
  g_message ("initing ruler...");
  GET_PRIVATE;

  rw_prv->zoom_level = DEFAULT_ZOOM_LEVEL;

  rw_prv->bg =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_visible (GTK_WIDGET (rw_prv->bg),
                          1);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (rw_prv->bg));
  gtk_widget_add_events (GTK_WIDGET (rw_prv->bg),
                         GDK_ALL_EVENTS_MASK);

  rw_prv->playhead =
    ruler_marker_widget_new (
      self, RULER_MARKER_TYPE_PLAYHEAD);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (rw_prv->playhead));

  /* make it able to notify */
  gtk_widget_add_events (GTK_WIDGET (rw_prv->bg),
                         GDK_ALL_EVENTS_MASK);

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
    G_OBJECT (rw_prv->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
}

static void
ruler_widget_class_init (RulerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "ruler");
}
