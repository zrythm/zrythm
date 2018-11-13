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
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "utils/ui.h"

G_DEFINE_TYPE (RegionWidget, region_widget, GTK_TYPE_DRAWING_AREA)

/**
 * Space on the edges to show resize cursors
 */
#define RESIZE_CURSOR_SPACE 9

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

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  GdkRGBA * color = &self->region->track->channel->color;
  if (self->hover)
    {
      cairo_set_source_rgba (cr,
                             color->red + 0.2,
                             color->green + 0.2,
                             color->blue + 0.2,
                             0.8);
    }
  else if (self->selected)
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

  draw_text (cr, self->region->name);

 return FALSE;
}

static void
on_motion (GtkWidget * widget, GdkEventMotion *event)
{
  RegionWidget * self = REGION_WIDGET (widget);
  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);

  if (event->type == GDK_MOTION_NOTIFY)
    {
      /* set hover state */
      self->hover = 1;

      /* set cursor state */
      if (event->x < RESIZE_CURSOR_SPACE)
        {
          self->cursor_state = RWS_CURSOR_RESIZE_L;
          if (MW_TIMELINE->action != ARRANGER_ACTION_MOVING)
            ui_set_cursor (widget, "w-resize");
        }

      else if (event->x > allocation.width - RESIZE_CURSOR_SPACE)
        {
          self->cursor_state = RWS_CURSOR_RESIZE_R;
          if (MW_TIMELINE->action != ARRANGER_ACTION_MOVING)
            ui_set_cursor (widget, "e-resize");
        }
      else
        {
          self->cursor_state = RWS_CURSOR_DEFAULT;
          if (MW_TIMELINE->action != ARRANGER_ACTION_MOVING &&
              MW_TIMELINE->action != ARRANGER_ACTION_STARTING_MOVING &&
              MW_TIMELINE->action != ARRANGER_ACTION_RESIZING_L &&
              MW_TIMELINE->action != ARRANGER_ACTION_RESIZING_R)
            {
              ui_set_cursor (widget, "default");
            }
        }
    }
  /* if leaving */
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      self->hover = 0;
      if (MAIN_WINDOW->timeline->action != ARRANGER_ACTION_MOVING &&
          MAIN_WINDOW->timeline->action != ARRANGER_ACTION_RESIZING_L &&
          MAIN_WINDOW->timeline->action != ARRANGER_ACTION_RESIZING_R)
        {
          ui_set_cursor (widget, "default");
        }
    }
  g_idle_add ((GSourceFunc) gtk_widget_queue_draw, GTK_WIDGET (self));
}

RegionWidget *
region_widget_new (Region * region)
{
  g_message ("Creating region widget...");
  RegionWidget * self = g_object_new (REGION_WIDGET_TYPE, NULL);

  self->region = region;

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

void
region_widget_select (RegionWidget * self,
                      int            select)
{
  self->selected = select;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
region_widget_class_init (RegionWidgetClass * klass)
{
}

static void
region_widget_init (RegionWidget * self)
{
}

