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

#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "utils/ui.h"

G_DEFINE_TYPE (MidiRegionWidget,
               midi_region_widget,
               REGION_WIDGET_TYPE)

/**
 * Space on the edges to show resize cursors
 */
#define RESIZE_CURSOR_SPACE 9

static gboolean
draw_cb (MidiRegionWidget * self, cairo_t *cr, gpointer data)
{
  RegionWidgetPrivate * rw =
    midi_region_widget_get_instance_private (self);
  guint width, height;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  GdkRGBA * color = &((InstrumentTrack *)rw->region->track)->channel->color;
  if (rw->hover)
    {
      cairo_set_source_rgba (cr,
                             color->red,
                             color->green,
                             color->blue,
                             0.8);
    }
  else if (rw->selected)
    {
      cairo_set_source_rgba (cr,
                             color->red + 0.2,
                             color->green + 0.2,
                             color->blue + 0.2,
                             0.8);
    }
  else
    {
      cairo_set_source_rgba (cr,
                             color->red - 0.2,
                             color->green - 0.2,
                             color->blue - 0.2,
                             0.7);
    }

  /* TODO draw midi notes */

 return FALSE;
}

/**
 * Sets the appropriate cursor.
 */
static void
on_motion (GtkWidget * widget, GdkEventMotion *event)
{
  MidiRegionWidget * self = MIDI_REGION_WIDGET (widget);
  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);
  ARRANGER_WIDGET_GET_PRIVATE (MW_TIMELINE);

  if (event->type == GDK_MOTION_NOTIFY)
    {
      if (event->x < RESIZE_CURSOR_SPACE)
        {
          self->cursor_state = MIDI_REGION_CURSOR_RESIZE_L;
          if (prv->action != ARRANGER_ACTION_MOVING)
            ui_set_cursor (widget, "w-resize");
        }

      else if (event->x > allocation.width - RESIZE_CURSOR_SPACE)
        {
          self->cursor_state = MIDI_REGION_CURSOR_RESIZE_R;
          if (prv->action != ARRANGER_ACTION_MOVING)
            ui_set_cursor (widget, "e-resize");
        }
      else
        {
          self->cursor_state = MIDI_REGION_CURSOR_DEFAULT;
          if (prv->action != ARRANGER_ACTION_MOVING &&
              prv->action != ARRANGER_ACTION_STARTING_MOVING &&
              prv->action != ARRANGER_ACTION_RESIZING_L &&
              prv->action != ARRANGER_ACTION_RESIZING_R)
            {
              ui_set_cursor (widget, "default");
            }
        }
    }
  /* if leaving */
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (prv->action != ARRANGER_ACTION_MOVING &&
          prv->action != ARRANGER_ACTION_RESIZING_L &&
          prv->action != ARRANGER_ACTION_RESIZING_R)
        {
          ui_set_cursor (widget, "default");
        }
    }
}

MidiRegionWidget *
midi_region_widget_new (MidiRegion * midi_region)
{
  g_message ("Creating midi region widget...");
  MidiRegionWidget * self = g_object_new (MIDI_REGION_WIDGET_TYPE, NULL);

  region_widget_setup (REGION_WIDGET (self),
                       (Region *) midi_region);

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
midi_region_widget_class_init (MidiRegionWidgetClass * klass)
{
}

static void
midi_region_widget_init (MidiRegionWidget * self)
{
}
