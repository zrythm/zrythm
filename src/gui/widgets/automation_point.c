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

#include "audio/automation_track.h"
#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/ui.h"

G_DEFINE_TYPE (AutomationPointWidget,
               automation_point_widget,
               GTK_TYPE_DRAWING_AREA)

static gboolean
draw_cb (AutomationPointWidget * self,
         cairo_t *cr, gpointer data)
{
  guint width, height;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  Track * track =
    self->automation_point->at->track;
  GdkRGBA * color = &track->color;
  cairo_set_source_rgba (
    cr, color->red, color->green, color->blue, 0.7);
  cairo_arc (cr,
             width / 2,
             height / 2,
             width / 2 - AP_WIDGET_PADDING,
             0,
             2 * G_PI);
  cairo_stroke_preserve(cr);
  cairo_fill(cr);

 return FALSE;
}

static void
on_motion (GtkWidget * widget,
           GdkEventMotion *event)
{
  AutomationPointWidget * self =
    Z_AUTOMATION_POINT_WIDGET (widget);

  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);

      bot_bar_change_status (
        "Automation Point - Click and drag to move "
        "it around (Use Shift to bypass snapping)");
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      ui_set_cursor_from_name (widget, "default");
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);

      bot_bar_change_status ("");
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

DEFINE_ARRANGER_OBJECT_WIDGET_SELECT (
  AUTOMATION_POINT, AutomationPoint,
  automation_point, timeline_selections,
  TL_SELECTIONS);

void
automation_point_widget_update_tooltip (
  AutomationPointWidget * self,
  int              show)
{
  AutomationPoint * ap =
    self->automation_point;

  /* set tooltip text */
  char * tooltip =
    g_strdup_printf (
      "%s %f",
      ap->at->automatable->label,
      ap->fvalue);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);

  /* set tooltip window */
  if (show)
    {
      tooltip =
        g_strdup_printf (
          "%f",
          ap->fvalue);
      gtk_label_set_text (self->tooltip_label,
                          tooltip);
      gtk_window_present (self->tooltip_win);

      g_free (tooltip);
    }
  else
    gtk_widget_hide (
      GTK_WIDGET (self->tooltip_win));
}

AutomationPointWidget *
automation_point_widget_new (
  AutomationPoint * ap)
{
  AutomationPointWidget * self =
    g_object_new (
      AUTOMATION_POINT_WIDGET_TYPE,
      "visible", 1,
      NULL);
  g_message ("Creating automation_point widget... %p",
             self);

  self->automation_point = ap;

  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "motion-notify-event",
    G_CALLBACK (on_motion),  self);

  return self;
}

static void
automation_point_widget_class_init (
  AutomationPointWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "automation-point");
}

static void
automation_point_widget_init (
  AutomationPointWidget * self)
{
  /* set tooltip window */
  self->tooltip_win =
    GTK_WINDOW (gtk_window_new (GTK_WINDOW_POPUP));
  gtk_window_set_type_hint (
    self->tooltip_win,
    GDK_WINDOW_TYPE_HINT_TOOLTIP);
  self->tooltip_label =
    GTK_LABEL (gtk_label_new ("label"));
  gtk_widget_set_visible (
    GTK_WIDGET (self->tooltip_label), 1);
  gtk_container_add (
    GTK_CONTAINER (self->tooltip_win),
    GTK_WIDGET (self->tooltip_label));
  gtk_window_set_position (
    self->tooltip_win, GTK_WIN_POS_MOUSE);

  g_object_ref (self);
}
