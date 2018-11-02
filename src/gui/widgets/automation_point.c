/*
 * gui/widgets/automation_point.c- AutomationPoint
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
#include "audio/channel.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/ruler.h"
#include "utils/ui.h"

G_DEFINE_TYPE (AutomationPointWidget, automation_point_widget, GTK_TYPE_DRAWING_AREA)

static gboolean
draw_cb (AutomationPointWidget * self, cairo_t *cr, gpointer data)
{
  guint width, height;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  GdkRGBA * color = &self->ap->at->track->channel->color;
  if (self->ap->type == AUTOMATION_POINT_CURVE)
    {
      cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, 0.7);
    }
  else
    {
      if (self->state != APW_STATE_NONE)
        {
          cairo_set_source_rgba (cr,
                                 color->red + 0.1,
                                 color->green + 0.1,
                                 color->blue + 0.1,
                                 0.7);
        }
      else
        {
          cairo_set_source_rgba (cr, color->red, color->green, color->blue, 0.7);
        }
    }
  /* TODO circle */
  cairo_rectangle(cr,
                  AP_WIDGET_PADDING,
                  AP_WIDGET_PADDING,
                  width - AP_WIDGET_PADDING * 2,
                  height - AP_WIDGET_PADDING * 2);
  cairo_stroke_preserve(cr);
  cairo_fill(cr);

 return FALSE;
}

static void
on_motion (GtkWidget * widget, GdkEventMotion *event)
{
  AutomationPointWidget * self = AUTOMATION_POINT_WIDGET (widget);

  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);

  if (event->type == GDK_MOTION_NOTIFY)
    {
      if (self->state != APW_STATE_SELECTED)
        self->state = APW_STATE_HOVER;
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (self->state != APW_STATE_SELECTED)
        self->state = APW_STATE_NONE;
    }
  g_idle_add ((GSourceFunc) gtk_widget_queue_draw, GTK_WIDGET (self));
}

AutomationPointWidget *
automation_point_widget_new (AutomationPoint * automation_point)
{
  g_message ("Creating automation_point widget...");
  AutomationPointWidget * self = g_object_new (AUTOMATION_POINT_WIDGET_TYPE, NULL);

  self->ap = automation_point;

  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  /* connect signals */
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), self);
  g_signal_connect (G_OBJECT (self), "enter-notify-event",
                    G_CALLBACK (on_motion),  self);
  g_signal_connect (G_OBJECT(self), "leave-notify-event",
                    G_CALLBACK (on_motion),  self);
  g_signal_connect (G_OBJECT(self), "motion-notify-event",
                    G_CALLBACK (on_motion),  self);

  return self;
}

static void
automation_point_widget_class_init (AutomationPointWidgetClass * klass)
{
}

static void
automation_point_widget_init (AutomationPointWidget * self)
{
}

/**
 * Sets hover state and queues draw.
 */
void
automation_point_widget_set_state_and_queue_draw (AutomationPointWidget *    self,
                                                  AutomationPointWidgetState state)
{
  if (self->state != state)
    {
      self->state = state;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }

}

