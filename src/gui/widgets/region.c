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
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "utils/ui.h"

G_DEFINE_TYPE_WITH_PRIVATE (RegionWidget,
                            region_widget,
                            GTK_TYPE_DRAWING_AREA)

static void
draw_text (cairo_t *cr, char * name)
{
#define FONT "Sans Bold 9"

  PangoLayout *layout;
  PangoFontDescription *desc;
  /*int i;*/

  cairo_translate (cr, 2, 2);

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  pango_layout_set_text (layout, name, -1);
  desc = pango_font_description_from_string (FONT);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  cairo_set_source_rgb (cr, 0, 0, 0);

  /* Inform Pango to re-layout the text with the new transformation */
  /*pango_cairo_update_layout (cr, layout);*/

  /*pango_layout_get_size (layout, &width, &height);*/
  /*cairo_move_to (cr, - ((double)width / PANGO_SCALE) / 2, - RADIUS);*/
  pango_cairo_show_layout (cr, layout);


  /* free the layout object */
  g_object_unref (layout);
}

static gboolean
draw_cb (RegionWidget * self, cairo_t *cr, gpointer data)
{
  guint width, height;
  GtkStyleContext *context;

  RegionWidgetPrivate * prv =
    region_widget_get_instance_private (self);

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  GdkRGBA * color = &((ChannelTrack *) prv->region->track)->channel->color;
  if (prv->hover)
    {
      cairo_set_source_rgba (cr,
                             color->red + 0.2,
                             color->green + 0.2,
                             color->blue + 0.2,
                             0.8);
    }
  else if (prv->selected)
    {
      cairo_set_source_rgba (cr,
                             color->red + 0.4,
                             color->green + 0.4,
                             color->blue + 0.4,
                             0.8);
    }
  else
    {
      cairo_set_source_rgba (cr, color->red, color->green, color->blue, 0.7);
    }
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_stroke_preserve(cr);
  cairo_fill(cr);

  draw_text (cr, prv->region->name);

 return FALSE;
}

static void
on_motion (GtkWidget * widget, GdkEventMotion *event)
{
  RegionWidget * self = REGION_WIDGET (widget);
  RegionWidgetPrivate * prv =
    region_widget_get_instance_private (self);

  if (event->type == GDK_MOTION_NOTIFY)
    {
      prv->hover = 1;
    }
  /* if leaving */
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      prv->hover = 0;
    }
  g_idle_add ((GSourceFunc) gtk_widget_queue_draw, GTK_WIDGET (self));
}

void
region_widget_setup (RegionWidget * self,
                     Region *       region)
{
  REGION_WIDGET_GET_PRIVATE (self);

  rw_prv->region = region;

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
}

void
region_widget_select (RegionWidget * self,
                      int            select)
{
  RegionWidgetPrivate * prv =
    region_widget_get_instance_private (self);
  prv->selected = select;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

RegionWidgetPrivate *
region_widget_get_private (RegionWidget * self)
{
  return region_widget_get_instance_private (self);
}

static void
region_widget_class_init (RegionWidgetClass * klass)
{
}

static void
region_widget_init (RegionWidget * self)
{
}

