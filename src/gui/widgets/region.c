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

#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "config.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/ui.h"

#include <glib/gi18n-lib.h>

G_DEFINE_TYPE_WITH_PRIVATE (RegionWidget,
                            region_widget,
                            GTK_TYPE_BOX)

static void
draw_text (cairo_t *cr, char * name)
{
#define FONT "Sans Bold 9"

  PangoLayout *layout;
  PangoFontDescription *desc;

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
  /*cairo_move_to (cr, - ((double)width / PANGO_SCALA) / 2, - RADIUS);*/
  pango_cairo_show_layout (cr, layout);

  /* free the layout object */
  g_object_unref (layout);
}

static gboolean
region_draw_cb (RegionWidget * self,
                cairo_t *cr,
                gpointer data)
{
  guint width, height;
  GtkStyleContext *context;

  REGION_WIDGET_GET_PRIVATE (data);
  Region * r = rw_prv->region;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (
    context, cr, 0, 0, width, height);

  GdkRGBA * color = &rw_prv->region->track->color;
  cairo_set_source_rgba (cr,
                         color->red - 0.06,
                         color->green - 0.06,
                         color->blue - 0.06,
                         0.7);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);
  cairo_set_source_rgba (cr,
                         color->red,
                         color->green,
                         color->blue,
                         1.0);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_set_line_width (cr, 3.5);
  cairo_stroke (cr);

  /* draw loop points */
  double dashes[] = { 5 };
  cairo_set_dash (cr, dashes, 1, 0);
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba (cr,
                         0,
                         0,
                         0,
                         1.0);

  Position tmp;
  long loop_start_ticks =
    position_to_ticks (&r->loop_start_pos);
  long loop_end_ticks =
    position_to_ticks (&r->loop_end_pos);
  long loop_ticks =
    region_get_loop_length_in_ticks (r);
  long clip_start_ticks =
    position_to_ticks (&r->clip_start_pos);

  position_from_ticks (
    &tmp, loop_start_ticks - clip_start_ticks);
  int px =
    ui_pos_to_px_timeline (&tmp, 0);
  if (px != 0)
    {
      cairo_set_source_rgba (
        cr, 0, 1, 0, 1.0);
      cairo_move_to (cr, px, 0);
      cairo_line_to (cr, px, height);
      cairo_stroke (cr);
    }

  int num_loops = region_get_num_loops (r, 1);
  for (int i = 0; i < num_loops; i++)
    {
      position_from_ticks (
        &tmp, loop_end_ticks + loop_ticks * i);

      /* adjust for clip_start */
      position_add_ticks (
        &tmp, - clip_start_ticks);

      px = ui_pos_to_px_timeline (&tmp, 0);

      if (px <= width - 1)
        {
          cairo_set_source_rgba (
            cr, 0, 0, 0, 1.0);
          cairo_move_to (cr, px, 0);
          cairo_line_to (cr, px, height);
          cairo_stroke (cr);
        }
    }

  draw_text (cr, rw_prv->region->name);

 return FALSE;
}

static int
on_motion (GtkWidget *      widget,
           GdkEventMotion * event,
           RegionWidget *   self)
{
  ARRANGER_WIDGET_GET_PRIVATE (MW_TIMELINE);
  REGION_WIDGET_GET_PRIVATE (self);

  if (event->type == GDK_MOTION_NOTIFY)
    {
      rw_prv->resize_l =
        region_widget_is_resize_l (
          self, event->x);
      rw_prv->resize_r =
        region_widget_is_resize_r (
          self, event->x);
    }

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
      bot_bar_change_status (
        _("Region - Click and drag to move around ("
        "hold Shift to disable snapping) - "
        "Double click to bring up the clip editor"));
    }
  /* if leaving */
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (ar_prv->action !=
            UI_OVERLAY_ACTION_MOVING &&
          ar_prv->action !=
            UI_OVERLAY_ACTION_RESIZING_L &&
          ar_prv->action !=
            UI_OVERLAY_ACTION_RESIZING_R)
        gtk_widget_unset_state_flags (
          GTK_WIDGET (self),
          GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }
  return FALSE;
}

void
region_widget_setup (RegionWidget * self,
                     Region *       region)
{
  REGION_WIDGET_GET_PRIVATE (self);

  rw_prv->region = region;

  rw_prv->drawing_area =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (rw_prv->drawing_area));
  gtk_widget_set_visible (GTK_WIDGET (rw_prv->drawing_area),
                          1);
  gtk_widget_set_hexpand (GTK_WIDGET (rw_prv->drawing_area),
                          1);

  gtk_widget_add_events (GTK_WIDGET (rw_prv->drawing_area),
                         GDK_ALL_EVENTS_MASK);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (rw_prv->drawing_area), "draw",
    G_CALLBACK (region_draw_cb), self);
  g_signal_connect (
    G_OBJECT (rw_prv->drawing_area),
    "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(rw_prv->drawing_area),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(rw_prv->drawing_area),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
}

/**
 * Returns if the current position is for resizing
 * L.
 */
int
region_widget_is_resize_l (
  RegionWidget * self,
  int             x)
{
  if (x < RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * L.
 */
int
region_widget_is_resize_r (
  RegionWidget * self,
  int             x)
{
  GtkAllocation allocation;
  gtk_widget_get_allocation (
    GTK_WIDGET (self),
    &allocation);

  if (x > allocation.width - RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

void
region_widget_select (
  RegionWidget * self,
  int            select,
  int            with_transients)
{
  RegionWidgetPrivate * prv =
    region_widget_get_instance_private (self);
  /*prv->region->selected = select;*/
  if (select)
    {
      /*gtk_widget_set_state_flags (*/
        /*GTK_WIDGET (self),*/
        /*GTK_STATE_FLAG_SELECTED,*/
        /*0);*/
      timeline_selections_add_region (
        TL_SELECTIONS,
        prv->region,
        with_transients);
    }
  else
    {
      /*gtk_widget_unset_state_flags (*/
        /*GTK_WIDGET (self),*/
        /*GTK_STATE_FLAG_SELECTED);*/
      timeline_selections_remove_region (
        TL_SELECTIONS,
        prv->region);
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

RegionWidgetPrivate *
region_widget_get_private (RegionWidget * self)
{
  return region_widget_get_instance_private (self);
}

static void
region_widget_class_init (RegionWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "region");
}

static void
region_widget_init (RegionWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), 1);
}
