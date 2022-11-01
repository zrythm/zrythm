// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
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
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_minimap_bg.h"
#include "gui/widgets/timeline_minimap_selection.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  TimelineMinimapWidget,
  timeline_minimap_widget,
  GTK_TYPE_WIDGET)

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
  int    px_in_ruler = (int) (MW_RULER->total_px * ratio);
  ui_px_to_pos_timeline (px_in_ruler, pos, 1);
}

static void
move_selection_x (TimelineMinimapWidget * self, double offset_x)
{
  double width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));

  double new_wx = self->selection_start_pos + offset_x;

  double ratio = new_wx / width;
  double ruler_px = MW_RULER->total_px * ratio;

  editor_settings_set_scroll_start_x (
    &PRJ_TIMELINE->editor_settings, (int) ruler_px,
    F_VALIDATE);
}

static void
resize_selection_l (
  TimelineMinimapWidget * self,
  double                  offset_x)
{
  double width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));

  double new_l = self->selection_start_pos + offset_x;
  double old_selection_width =
    self->selection_end_pos - self->selection_start_pos;
  double new_selection_width = self->selection_end_pos - new_l;

  /** update zoom level */
  double ratio = new_selection_width / old_selection_width;
  int    zoom_level_set = ruler_widget_set_zoom_level (
       Z_RULER_WIDGET (MW_RULER), self->start_zoom_level / ratio);
  /*zoom_level_set =*/
  /*ruler_widget_set_zoom_level (*/
  /*Z_RULER_WIDGET (EDITOR_RULER),*/
  /*self->start_zoom_level / ratio);*/

  if (zoom_level_set)
    {
      /* set alignment */
      ratio = new_l / width;
      double ruler_px = MW_RULER->total_px * ratio;
      editor_settings_set_scroll_start_x (
        &PRJ_TIMELINE->editor_settings, (int) ruler_px,
        F_VALIDATE);

      EVENTS_PUSH (ET_RULER_VIEWPORT_CHANGED, MW_RULER);
    }
}

static void
resize_selection_r (
  TimelineMinimapWidget * self,
  double                  offset_x)
{
  double width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));

  double new_r = self->selection_end_pos + offset_x;
  double old_selection_width =
    self->selection_end_pos - self->selection_start_pos;
  double new_selection_width =
    new_r - self->selection_start_pos;

  /** update zoom level */
  double ratio = new_selection_width / old_selection_width;
  int    zoom_level_set = ruler_widget_set_zoom_level (
       Z_RULER_WIDGET (MW_RULER), self->start_zoom_level / ratio);
  /*zoom_level_set =*/
  /*ruler_widget_set_zoom_level (*/
  /*Z_RULER_WIDGET (EDITOR_RULER),*/
  /*self->start_zoom_level / ratio);*/

  if (zoom_level_set)
    {
      /* set alignment */
      ratio = self->selection_start_pos / width;
      double ruler_px = MW_RULER->total_px * ratio;
      editor_settings_set_scroll_start_x (
        &PRJ_TIMELINE->editor_settings, (int) ruler_px,
        F_VALIDATE);

      EVENTS_PUSH (ET_RULER_VIEWPORT_CHANGED, MW_RULER);
    }
}

static void
move_y (TimelineMinimapWidget * self, int offset_y)
{
}

/**
 * Gets called to set the position/size of each
 * overlayed widget.
 */
static gboolean
get_child_position (
  GtkOverlay *   overlay,
  GtkWidget *    widget,
  GdkRectangle * allocation,
  gpointer       user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  if (Z_IS_TIMELINE_MINIMAP_SELECTION_WIDGET (widget))
    {
      if (
        MAIN_WINDOW && MW_CENTER_DOCK && MW_TIMELINE_PANEL
        && MW_TIMELINE_PANEL->ruler)
        {
          int width = gtk_widget_get_allocated_width (
            GTK_WIDGET (self));
          int height = gtk_widget_get_allocated_height (
            GTK_WIDGET (self));

          /* get pixels at start of visible ruler */
          double px_start =
            (double)
              PRJ_TIMELINE->editor_settings.scroll_start_x;
          double px_width = gtk_widget_get_allocated_width (
            GTK_WIDGET (MW_TIMELINE_PANEL->ruler));

          double start_ratio =
            px_start / (double) MW_RULER->total_px;
          double width_ratio =
            px_width / (double) MW_RULER->total_px;

          allocation->x = (int) ((double) width * start_ratio);
          allocation->y = 0;
          allocation->width =
            (int) ((double) width * width_ratio);
          allocation->height = height;

          return true;
        }
    }
  return false;
}

static void
show_context_menu (TimelineMinimapWidget * self)
{
  /* TODO */
}

static void
on_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
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
on_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  self->n_press = n_press;
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  gpointer         user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  self->start_x = start_x;
  self->start_y = start_y;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  self->start_zoom_level =
    ruler_widget_get_zoom_level (MW_RULER);

  int is_hit = ui_is_child_hit (
    GTK_WIDGET (self), GTK_WIDGET (self->selection), 1, 1,
    start_x, start_y, 0, 0);
  if (is_hit)
    {
      /* update arranger action */
      if (self->selection->cursor == UI_CURSOR_STATE_RESIZE_L)
        self->action = TIMELINE_MINIMAP_ACTION_RESIZING_L;
      else if (self->selection->cursor == UI_CURSOR_STATE_RESIZE_R)
        self->action = TIMELINE_MINIMAP_ACTION_RESIZING_R;
      else
        {
          self->action =
            TIMELINE_MINIMAP_ACTION_STARTING_MOVING;
        }

      double wx, wy;
      gtk_widget_translate_coordinates (
        GTK_WIDGET (self->selection), GTK_WIDGET (self), 0, 0,
        &wx, &wy);
      self->selection_start_pos = wx;
      self->selection_end_pos =
        wx
        + gtk_widget_get_allocated_width (
          GTK_WIDGET (self->selection));

      /* motion handler was causing drag update
       * to not get called */
      /*g_signal_handlers_disconnect_by_func (*/
      /*G_OBJECT (self->selection->drawing_area),*/
      /*timeline_minimap_selection_widget_on_motion,*/
      /*self->selection);*/
    }
  else /* nothing hit */
    {
      self->action = TIMELINE_MINIMAP_ACTION_NONE;

      if (self->n_press == 1)
        {
          self->action =
            TIMELINE_MINIMAP_ACTION_STARTING_MOVING;

          double selection_width =
            self->selection_end_pos
            - self->selection_start_pos;
          self->selection_start_pos =
            start_x - selection_width / 2.0;
          self->selection_end_pos =
            self->selection_start_pos
            + gtk_widget_get_allocated_width (
              GTK_WIDGET (self->selection));
        }
      else if (self->n_press == 2)
        {
          /* TODO or here for double click */
        }
    }

  if (self->action == TIMELINE_MINIMAP_ACTION_STARTING_MOVING)
    {
      ui_set_cursor_from_name (
        GTK_WIDGET (self->selection), "grabbing");
    }

  /* update inspector */
  /*update_inspector (self);*/
}

static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  gpointer         user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  if (self->action == TIMELINE_MINIMAP_ACTION_STARTING_MOVING)
    {
      self->action = TIMELINE_MINIMAP_ACTION_MOVING;
    }

  /* handle x */
  if (self->action == TIMELINE_MINIMAP_ACTION_RESIZING_L)
    {
      resize_selection_l (self, offset_x);
    }
  else if (self->action == TIMELINE_MINIMAP_ACTION_RESIZING_R)
    {
      resize_selection_r (self, offset_x);
    }

  /* if moving the selection */
  else if (self->action == TIMELINE_MINIMAP_ACTION_MOVING)
    {
      move_selection_x (self, offset_x);

      /* handle y */
      move_y (self, (int) offset_y);
    }

  /*gtk_widget_queue_allocate (GTK_WIDGET (self));*/
  self->last_offset_x = offset_x;
  self->last_offset_y = offset_y;

  /* update inspector */
  /*update_inspector (self);*/

  gtk_widget_queue_allocate (GTK_WIDGET (self->overlay));
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  gpointer         user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (user_data);

  self->start_x = 0;
  self->start_y = 0;
  self->last_offset_x = 0;
  self->last_offset_y = 0;

  self->action = TIMELINE_MINIMAP_ACTION_NONE;
}

/**
 * Causes reallocation.
 */
void
timeline_minimap_widget_refresh (TimelineMinimapWidget * self)
{
  /*gtk_widget_queue_allocate (GTK_WIDGET (self));*/
}

static gboolean
timeline_minimap_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  TimelineMinimapWidget * self =
    Z_TIMELINE_MINIMAP_WIDGET (widget);

  gtk_widget_queue_allocate (GTK_WIDGET (self->overlay));

  return G_SOURCE_CONTINUE;
}

static void
dispose (TimelineMinimapWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->overlay));

  G_OBJECT_CLASS (timeline_minimap_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
timeline_minimap_widget_class_init (
  TimelineMinimapWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "timeline-minimap");
  gtk_widget_class_set_layout_manager_type (
    klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
timeline_minimap_widget_init (TimelineMinimapWidget * self)
{
  self->overlay = GTK_OVERLAY (gtk_overlay_new ());
  gtk_widget_set_parent (
    GTK_WIDGET (self->overlay), GTK_WIDGET (self));
  gtk_widget_set_name (
    GTK_WIDGET (self->overlay), "timeline-minimap-overlay");

  self->bg = timeline_minimap_bg_widget_new ();
  gtk_overlay_set_child (self->overlay, GTK_WIDGET (self->bg));
  self->selection =
    timeline_minimap_selection_widget_new (self);
  gtk_overlay_add_overlay (
    self->overlay, GTK_WIDGET (self->selection));

  GtkGestureDrag * drag =
    GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT (drag), "drag-begin", G_CALLBACK (drag_begin),
    self);
  g_signal_connect (
    G_OBJECT (drag), "drag-update", G_CALLBACK (drag_update),
    self);
  g_signal_connect (
    G_OBJECT (drag), "drag-end", G_CALLBACK (drag_end), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->overlay), GTK_EVENT_CONTROLLER (drag));

  GtkGestureClick * click =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (click), "pressed", G_CALLBACK (on_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->overlay), GTK_EVENT_CONTROLLER (click));

  GtkGestureClick * right_click =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_click), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller (
    GTK_WIDGET (self->overlay),
    GTK_EVENT_CONTROLLER (right_click));
  g_signal_connect (
    G_OBJECT (right_click), "pressed",
    G_CALLBACK (on_right_click), self);

  g_signal_connect (
    G_OBJECT (self->overlay), "get-child-position",
    G_CALLBACK (get_child_position), self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), timeline_minimap_tick_cb, self, NULL);
}
