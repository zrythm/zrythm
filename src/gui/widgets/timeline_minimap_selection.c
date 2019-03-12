/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_minimap_selection.h"
#include "utils/cairo.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineMinimapSelectionWidget,
               timeline_minimap_selection_widget,
               GTK_TYPE_BOX)

#define PADDING 2

static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  z_cairo_draw_selection (
    cr,
    0,
    PADDING,
    gtk_widget_get_allocated_width (widget),
    gtk_widget_get_allocated_height (widget) - PADDING * 2);

  return FALSE;
}

static gboolean
on_motion (GtkWidget * widget,
           GdkEventMotion *event,
           gpointer user_data)
{
  TimelineMinimapSelectionWidget * self =
    Z_TIMELINE_MINIMAP_SELECTION_WIDGET (user_data);
  guint width =
    gtk_widget_get_allocated_width (widget);

  if (event->type == GDK_ENTER_NOTIFY)
    bot_bar_change_status (
      "Minimap Selector - Click and drag to move - "
      "Click and drag edges to resize");
  if (event->type == GDK_MOTION_NOTIFY)
    {
      gtk_widget_set_state_flags (GTK_WIDGET (self),
                                  GTK_STATE_FLAG_PRELIGHT,
                                  0);
      if (event->x < RESIZE_CURSOR_SPACE)
        {
          self->cursor = UI_CURSOR_STATE_RESIZE_L;
          if (self->parent->action != TIMELINE_MINIMAP_ACTION_MOVING)
            ui_set_cursor (widget, "w-resize");
        }
      else if (event->x > width -
                 RESIZE_CURSOR_SPACE)
        {
          self->cursor = UI_CURSOR_STATE_RESIZE_R;
          if (self->parent->action != TIMELINE_MINIMAP_ACTION_MOVING)
            ui_set_cursor (widget, "e-resize");
        }
      else
        {
          self->cursor = UI_CURSOR_STATE_DEFAULT;
          if (self->parent->action !=
                TIMELINE_MINIMAP_ACTION_MOVING &&
              self->parent->action !=
                TIMELINE_MINIMAP_ACTION_STARTING_MOVING &&
              self->parent->action !=
                TIMELINE_MINIMAP_ACTION_RESIZING_L &&
              self->parent->action !=
                TIMELINE_MINIMAP_ACTION_RESIZING_R)
            {
              ui_set_cursor (widget, "default");
            }
        }
    }
  /* if leaving */
  if (event->type == GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }

  return FALSE;
}

TimelineMinimapSelectionWidget *
timeline_minimap_selection_widget_new (
  TimelineMinimapWidget * parent)
{
  TimelineMinimapSelectionWidget * self =
    g_object_new (TIMELINE_MINIMAP_SELECTION_WIDGET_TYPE,
                  NULL);

  self->parent = parent;

  return self;
}

static void
timeline_minimap_selection_widget_class_init (
  TimelineMinimapSelectionWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "timeline-minimap-selection");
}

static void
timeline_minimap_selection_widget_init (
  TimelineMinimapSelectionWidget * self)
{
  self->drawing_area =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_hexpand (GTK_WIDGET (self->drawing_area),
                         1);
  gtk_widget_set_visible (GTK_WIDGET (self->drawing_area),
                          1);

  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->drawing_area));
  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  gtk_widget_add_events (GTK_WIDGET (self->drawing_area),
                         GDK_ALL_EVENTS_MASK);

  g_signal_connect (G_OBJECT (self->drawing_area),
                    "draw",
                    G_CALLBACK (draw_cb),
                    self);
  /*g_signal_connect (G_OBJECT (self->drawing_area),*/
                    /*"enter-notify-event",*/
                    /*G_CALLBACK (on_motion),*/
                    /*self);*/
  /*g_signal_connect (G_OBJECT(self->drawing_area),*/
                    /*"leave-notify-event",*/
                    /*G_CALLBACK (on_motion),*/
                    /*self);*/
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "motion-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "enter-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "leave-notify-event",
    G_CALLBACK (on_motion), self);
}
