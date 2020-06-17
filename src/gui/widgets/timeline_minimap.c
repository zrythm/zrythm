/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_bg.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_minimap_bg.h"
#include "gui/widgets/timeline_minimap_selection.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineMinimapWidget,
               timeline_minimap_widget,
               GTK_TYPE_OVERLAY)

/**
 * Taken from arranger.c
 */
void
timeline_minimap_widget_px_to_pos (
  TimelineMinimapWidget * self,
  Position *              pos,
  int                     px)
{
  double width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));
  double ratio = (double) px / width;
  int px_in_ruler =
    (int) (MW_RULER->total_px * ratio);
  ui_px_to_pos_timeline (px_in_ruler, pos, 1);
}

static void
move_selection_x (
  TimelineMinimapWidget * self,
  double                  offset_x)
{
  double width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));

  double new_wx =
    self->selection_start_pos + offset_x;

  double ratio = new_wx / width;
  double ruler_px = MW_RULER->total_px * ratio;
  GtkAdjustment * adj =
    gtk_scrollable_get_hadjustment (
      GTK_SCROLLABLE (
        MW_TIMELINE_PANEL->ruler_viewport));
  gtk_adjustment_set_value (adj, ruler_px);
}

static void
resize_selection_l (
  TimelineMinimapWidget * self,
  double                  offset_x)
{
  double width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));

  double new_l =
    self->selection_start_pos + offset_x;
  double old_selection_width =
    self->selection_end_pos -
      self->selection_start_pos;
  double new_selection_width =
    self->selection_end_pos - new_l;

  /** update zoom level */
  double ratio =
    new_selection_width / old_selection_width;
  int zoom_level_set =
    ruler_widget_set_zoom_level (
      Z_RULER_WIDGET (MW_RULER),
      self->start_zoom_level / ratio);
  /*zoom_level_set =*/
    /*ruler_widget_set_zoom_level (*/
      /*Z_RULER_WIDGET (EDITOR_RULER),*/
      /*self->start_zoom_level / ratio);*/

  if (zoom_level_set)
    {
      /* set alignment */
      ratio =
        new_l / width;
      double ruler_px = MW_RULER->total_px * ratio;
      GtkAdjustment * adj =
        gtk_scrollable_get_hadjustment (
          GTK_SCROLLABLE (
            MW_TIMELINE_PANEL->ruler_viewport));
      gtk_adjustment_set_value (adj, ruler_px);

      EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
                   NULL);
    }
}

static void
resize_selection_r (
  TimelineMinimapWidget * self,
  double                  offset_x)
{
  double width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));

  double new_r =
    self->selection_end_pos + offset_x;
  double old_selection_width =
    self->selection_end_pos -
      self->selection_start_pos;
  double new_selection_width =
    new_r - self->selection_start_pos;

  /** update zoom level */
  double ratio =
    new_selection_width / old_selection_width;
  int zoom_level_set =
    ruler_widget_set_zoom_level (
      Z_RULER_WIDGET (MW_RULER),
      self->start_zoom_level / ratio);
  /*zoom_level_set =*/
    /*ruler_widget_set_zoom_level (*/
      /*Z_RULER_WIDGET (EDITOR_RULER),*/
      /*self->start_zoom_level / ratio);*/

  if (zoom_level_set)
    {
      /* set alignment */
      ratio =
        self->selection_start_pos / width;
      double ruler_px = MW_RULER->total_px * ratio;
      GtkAdjustment * adj =
        gtk_scrollable_get_hadjustment (
          GTK_SCROLLABLE (
            MW_TIMELINE_PANEL->ruler_viewport));
      gtk_adjustment_set_value (adj,
                                ruler_px);

      EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
                   NULL);
    }

}

static void
move_y (
  TimelineMinimapWidget * self,
  int                     offset_y)
{

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
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (overlay);

  if (Z_IS_TIMELINE_MINIMAP_SELECTION_WIDGET (widget))
    {
      if (MAIN_WINDOW &&
          MW_CENTER_DOCK &&
          MW_TIMELINE_PANEL &&
          MW_TIMELINE_PANEL->ruler_viewport)
        {
          int width =
            gtk_widget_get_allocated_width (
              GTK_WIDGET (self));
          int height =
            gtk_widget_get_allocated_height (
              GTK_WIDGET (self));

          /* get pixels at start of visible ruler */
          GtkAdjustment * adj =
            gtk_scrollable_get_hadjustment (
              GTK_SCROLLABLE (
                MW_TIMELINE_PANEL->ruler_viewport));
          double px_start =
            gtk_adjustment_get_value (adj);
          double px_width =
            gtk_widget_get_allocated_width (
              GTK_WIDGET (
                MW_TIMELINE_PANEL->ruler_viewport));

          double start_ratio =
            px_start / (double) MW_RULER->total_px;
          double width_ratio =
            px_width / (double) MW_RULER->total_px;

          allocation->x =
            (int) ((double) width * start_ratio);
          allocation->y = 0;
          allocation->width =
            (int) ((double) width * width_ratio);
          allocation->height = height;
          return TRUE;
        }
    }
  return FALSE;
}

static void
show_context_menu (TimelineMinimapWidget * self)
{
  /* TODO */
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

/**
 * On button press.
 *
 * This merely sets the number of clicks. It is always called
 * before drag_begin, so the logic is done in drag_begin.
 */
static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  self->n_press = n_press;
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  self->start_x = start_x;
  self->start_y = start_y;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (
      GTK_GESTURE_SINGLE (gesture));
  const GdkEvent * event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture),
      sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);

  int is_hit =
    ui_is_child_hit (GTK_WIDGET (self),
                     GTK_WIDGET (self->selection),
                     1, 1,
                     start_x,
                     start_y, 0, 0);
  if (is_hit)
    {
      /* update arranger action */
      if (self->selection->cursor ==
            UI_CURSOR_STATE_RESIZE_L)
        self->action = TIMELINE_MINIMAP_ACTION_RESIZING_L;
      else if (self->selection->cursor ==
                 UI_CURSOR_STATE_RESIZE_R)
        self->action = TIMELINE_MINIMAP_ACTION_RESIZING_R;
      else
        {
          self->action =
            TIMELINE_MINIMAP_ACTION_STARTING_MOVING;
          ui_set_cursor_from_name (GTK_WIDGET (self->selection),
                         "grabbing");
        }

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (self->selection),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);
      self->selection_start_pos = wx;
      self->selection_end_pos =
        wx +
          gtk_widget_get_allocated_width (
            GTK_WIDGET (self->selection));

      self->start_zoom_level =
        MW_RULER->zoom_level;

      /* motion handler was causing drag update
       * to not get called */
      /*g_signal_handlers_disconnect_by_func (*/
        /*G_OBJECT (self->selection->drawing_area),*/
        /*timeline_minimap_selection_widget_on_motion,*/
        /*self->selection);*/
    }
  else /* nothing hit */
    {
      if (self->n_press == 1)
        {
          /* TODO move the selection to be centered around
           * this point */
        }
      else if (self->n_press == 2)
        {
          /* TODO or here for double click */
        }
      self->action = TIMELINE_MINIMAP_ACTION_NONE;
    }

  /* update inspector */
  /*update_inspector (self);*/
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  if (self->action ==
           TIMELINE_MINIMAP_ACTION_STARTING_MOVING)
    {
      self->action = TIMELINE_MINIMAP_ACTION_MOVING;
    }

  /* handle x */
  if (self->action == TIMELINE_MINIMAP_ACTION_RESIZING_L)
    {
      resize_selection_l (self,
                          offset_x);
    }
  else if (self->action == TIMELINE_MINIMAP_ACTION_RESIZING_R)
    {
      resize_selection_r (self,
                          offset_x);
    }

  /* if moving the selection */
  else if (self->action == TIMELINE_MINIMAP_ACTION_MOVING)
    {
      move_selection_x (
        self,
        offset_x);

      /* handle y */
      move_y (
        self,
        (int) offset_y);
    } /* endif MOVING */

  gtk_widget_queue_allocate(GTK_WIDGET (self));
  self->last_offset_x = offset_x;
  self->last_offset_y = offset_y;

  /* update inspector */
  /*update_inspector (self);*/
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  self->start_x = 0;
  self->start_y = 0;
  self->last_offset_x = 0;
  self->last_offset_y = 0;

  self->action = TIMELINE_MINIMAP_ACTION_NONE;
  gtk_widget_queue_draw (GTK_WIDGET (self->bg));
}

/**
 * Causes reallocation.
 */
void
timeline_minimap_widget_refresh (
  TimelineMinimapWidget * self)
{
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
timeline_minimap_widget_class_init (
  TimelineMinimapWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "timeline-minimap");
}

static void
timeline_minimap_widget_init (
  TimelineMinimapWidget * self)
{
  self->bg = timeline_minimap_bg_widget_new ();
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->bg));
  self->selection =
    timeline_minimap_selection_widget_new (self);
  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                           GTK_WIDGET (self->selection));

  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  self->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp),
                        GDK_BUTTON_SECONDARY);

  g_signal_connect (
    G_OBJECT (self), "get-child-position",
    G_CALLBACK (get_child_position), NULL);
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
    G_OBJECT (self->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
}
