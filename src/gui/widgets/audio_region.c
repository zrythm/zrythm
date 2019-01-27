/*
 * gui/widgets/region.c- Region
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

#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "utils/ui.h"

G_DEFINE_TYPE (AudioRegionWidget,
               audio_region_widget,
               REGION_WIDGET_TYPE)

static gboolean
draw_cb (AudioRegionWidget * self, cairo_t *cr, gpointer data)
{
  REGION_WIDGET_GET_PRIVATE (data);
  guint width, height;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  GdkRGBA * color = &rw_prv->region->track->color;
  /*if (rw_prv->hover)*/
    /*{*/
      /*cairo_set_source_rgba (cr,*/
                             /*color->red,*/
                             /*color->green,*/
                             /*color->blue,*/
                             /*0.8);*/
    /*}*/
  /*else if (rw_prv->selected)*/
    /*{*/
      /*cairo_set_source_rgba (cr,*/
                             /*color->red + 0.2,*/
                             /*color->green + 0.2,*/
                             /*color->blue + 0.2,*/
                             /*0.8);*/
    /*}*/
  /*else*/
    /*{*/
      cairo_set_source_rgba (cr,
                             color->red - 0.2,
                             color->green - 0.2,
                             color->blue - 0.2,
                             0.7);
    /*}*/

  /* TODO draw audio notes */

 return FALSE;
}

/**
 * Sets the appropriate cursor.
 */
static void
on_motion (GtkWidget * widget, GdkEventMotion *event)
{
  AudioRegionWidget * self = Z_AUDIO_REGION_WIDGET (widget);
  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);
  ARRANGER_WIDGET_GET_PRIVATE (MW_TIMELINE);
  REGION_WIDGET_GET_PRIVATE (self);

  if (event->type == GDK_MOTION_NOTIFY)
    {
      if (event->x < RESIZE_CURSOR_SPACE)
        {
          rw_prv->cursor_state = UI_CURSOR_STATE_RESIZE_L;
          if (ar_prv->action != ARRANGER_ACTION_MOVING)
            ui_set_cursor (widget, "w-resize");
        }

      else if (event->x >
                 allocation.width - RESIZE_CURSOR_SPACE)
        {
          rw_prv->cursor_state =
            UI_CURSOR_STATE_RESIZE_R;
          if (ar_prv->action != ARRANGER_ACTION_MOVING)
            ui_set_cursor (widget, "e-resize");
        }
      else
        {
          rw_prv->cursor_state = UI_CURSOR_STATE_DEFAULT;
          if (ar_prv->action != ARRANGER_ACTION_MOVING &&
              ar_prv->action != ARRANGER_ACTION_STARTING_MOVING &&
              ar_prv->action != ARRANGER_ACTION_RESIZING_L &&
              ar_prv->action != ARRANGER_ACTION_RESIZING_R)
            {
              ui_set_cursor (widget, "default");
            }
        }
    }
  /* if leaving */
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (ar_prv->action != ARRANGER_ACTION_MOVING &&
          ar_prv->action != ARRANGER_ACTION_RESIZING_L &&
          ar_prv->action != ARRANGER_ACTION_RESIZING_R)
        {
          ui_set_cursor (widget, "default");
        }
    }
}

AudioRegionWidget *
audio_region_widget_new (AudioRegion * audio_region)
{
  g_message ("Creating audio region widget...");
  AudioRegionWidget * self = g_object_new (AUDIO_REGION_WIDGET_TYPE, NULL);

  region_widget_setup (Z_REGION_WIDGET (self),
                       (Region *) audio_region);

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
audio_region_widget_class_init (AudioRegionWidgetClass * klass)
{
}

static void
audio_region_widget_init (AudioRegionWidget * self)
{
}

