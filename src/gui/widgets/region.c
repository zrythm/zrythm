/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "config.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/ui.h"

#include <glib/gi18n-lib.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  RegionWidget,
  region_widget,
  ARRANGER_OBJECT_WIDGET_TYPE)

#define NAME_FONT "Sans SemiBold 9"
#define NAME_PADDING_R 5
#define NAME_BOX_HEIGHT 19
#define NAME_BOX_CURVINESS 4.0


/** Background color for the name. */
static GdkRGBA name_bg_color;

/**
 * Draws the loop points (dashes).
 */
void
region_widget_draw_loop_points (
  RegionWidget * self,
  GtkWidget *    widget,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  REGION_WIDGET_GET_PRIVATE (self);
  Region * r = rw_prv->region;

  int height =
    gtk_widget_get_allocated_height (widget);

  double dashes[] = { 5 };
  cairo_set_dash (
    cr, dashes, 1, 0);
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba (
    cr, 0, 0, 0, 1.0);

  ArrangerObject * r_obj =
    (ArrangerObject *) r;
  Position tmp;
  long loop_start_ticks =
    r_obj->loop_start_pos.total_ticks;
  long loop_end_ticks =
    r_obj->loop_end_pos.total_ticks;
  g_warn_if_fail (
    loop_end_ticks > loop_start_ticks);
  long loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      r_obj);
  long clip_start_ticks =
    r_obj->clip_start_pos.total_ticks;

  position_from_ticks (
    &tmp, loop_start_ticks - clip_start_ticks);
  int px =
    ui_pos_to_px_timeline (&tmp, 0);
  if (px != 0 &&
      px >= rect->x && px < rect->x + rect->width)
    {
      cairo_set_source_rgba (
        cr, 0, 1, 0, 1.0);
      cairo_move_to (cr, px, 0);
      cairo_line_to (
        cr, px, height);
      cairo_stroke (cr);
    }

  int num_loops =
    arranger_object_get_num_loops (r_obj, 1);
  for (int i = 0; i < num_loops; i++)
    {
      position_from_ticks (
        &tmp, loop_end_ticks + loop_ticks * i);

      /* adjust for clip_start */
      position_add_ticks (
        &tmp, - clip_start_ticks);

      px = ui_pos_to_px_timeline (&tmp, 0);

      if (px >= rect->x && px < rect->width)
        {
          cairo_set_source_rgba (
            cr, 0, 0, 0, 1.0);
          cairo_move_to (
            cr, px, 0);
          cairo_line_to (
            cr, px, height);
          cairo_stroke (
            cr);
        }
    }

  cairo_set_dash (
    cr, NULL, 0, 0);
}

/**
 * Draws the background rectangle.
 */
void
region_widget_draw_background (
  RegionWidget * self,
  GtkWidget *    widget,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  REGION_WIDGET_GET_PRIVATE (self);
  Region * r = rw_prv->region;

  /*int width =*/
    /*gtk_widget_get_allocated_width (widget);*/
  /*int height =*/
    /*gtk_widget_get_allocated_height (widget);*/

  Track * track =
    arranger_object_get_track (
      (ArrangerObject *) r);

  /* set color */
  GdkRGBA color;
  if (track)
    color = track->color;
  else
    {
      color.red = 1;
      color.green = 0;
      color.blue = 0;
      color.alpha = 1;
    }
  ui_get_arranger_object_color (
    &color,
    gtk_widget_get_state_flags (
      GTK_WIDGET (self)) &
      GTK_STATE_FLAG_PRELIGHT,
    region_is_selected (r),
    region_is_transient (r));
  gdk_cairo_set_source_rgba (
    cr, &color);

  /* draw arc-rectangle */
  z_cairo_rounded_rectangle (
    cr, 0, 0, rect->width, rect->height,
    1.0, 4.0);
  cairo_fill (cr);
}

/**
 * Draws the name of the Region.
 *
 * To be called during a cairo draw callback.
 */
void
region_widget_draw_name (
  RegionWidget * self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  REGION_WIDGET_GET_PRIVATE (self);

  /* no need to draw if the start of the region is
   * not visible */
  if (rect->x > 800)
    return;

  Region * region = rw_prv->region;
  g_return_if_fail (
    region &&
    region->name);

  char str[200];
  strcpy (str, region->name);
  if (DEBUGGING)
    {
      if (region_is_transient (region))
        {
          strcat (str, " [t]");
        }
      if (region_is_lane (region))
        {
          strcat (str, " [l]");
        }
    }

  /* draw dark bg behind text */
  PangoLayout * layout = rw_prv->layout;
  pango_layout_set_text (
    layout, str, -1);
  PangoRectangle pangorect;
  /* get extents */
  pango_layout_get_pixel_extents (
    layout, NULL, &pangorect);
  gdk_cairo_set_source_rgba (
    cr, &name_bg_color);
  double radius = NAME_BOX_CURVINESS / 1.0;
  double degrees = G_PI / 180.0;
  cairo_new_sub_path (cr);
  cairo_move_to (
    cr, pangorect.width + NAME_PADDING_R, 0);
  cairo_arc (
    cr, (pangorect.width + NAME_PADDING_R) - radius,
    NAME_BOX_HEIGHT - radius, radius,
    0 * degrees, 90 * degrees);
  cairo_line_to (cr, 0, NAME_BOX_HEIGHT);
  cairo_line_to (cr, 0, 0);
  cairo_close_path (cr);
  cairo_fill (cr);

  /* draw text */
  cairo_set_source_rgba (
    cr, 1, 1, 1, 1);
  cairo_translate (cr, 2, 2);
  pango_cairo_show_layout (cr, layout);
}

/**
 * Sets up the RegionWidget.
 */
void
region_widget_setup (
  RegionWidget * self,
  Region *       region)
{
  REGION_WIDGET_GET_PRIVATE (self);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) region);

  rw_prv->region = region;
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
  /* FIXME temporary */
  REGION_WIDGET_GET_PRIVATE (self);
  if (rw_prv->region->type == REGION_TYPE_AUDIO)
    return 0;

  if (x < UI_RESIZE_CURSOR_SPACE)
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

  if (x > allocation.width - UI_RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * loop.
 */
int
region_widget_is_resize_loop (
  RegionWidget * self,
  int             y)
{
  REGION_WIDGET_GET_PRIVATE (self);

  Region * r = rw_prv->region;
  ArrangerObject * r_obj =
    (ArrangerObject *) r;

  if (r->type == REGION_TYPE_AUDIO)
    return 1;

  if ((position_to_ticks (&r_obj->end_pos) -
       position_to_ticks (&r_obj->pos)) >
      position_to_ticks (&r_obj->loop_end_pos))
    {
      return 1;
    }

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  if (y > height / 2)
    {
      return 1;
    }
  return 0;
}

RegionWidgetPrivate *
region_widget_get_private (RegionWidget * self)
{
  return region_widget_get_instance_private (self);
}

static void
recreate_pango_layouts (
  RegionWidget * self,
  GdkRectangle * allocation)
{
  REGION_WIDGET_GET_PRIVATE (self);

  if (PANGO_IS_LAYOUT (rw_prv->layout))
    g_object_unref (rw_prv->layout);

  GtkWidget * widget = (GtkWidget *) self;

  PangoFontDescription *desc;
  rw_prv->layout =
    gtk_widget_create_pango_layout (
      widget, NULL);
  desc =
    pango_font_description_from_string (
      NAME_FONT);
  pango_layout_set_font_description (
    rw_prv->layout, desc);
  pango_font_description_free (desc);
  pango_layout_set_ellipsize (
    rw_prv->layout, PANGO_ELLIPSIZE_END);
  if (allocation)
    {
      pango_layout_set_width (
        rw_prv->layout,
        pango_units_from_double (
          allocation->width - NAME_PADDING_R));
    }
}

static void
on_size_allocate (
  GtkWidget *    widget,
  GdkRectangle * allocation,
  RegionWidget * self)
{
  recreate_pango_layouts (self, allocation);
  arranger_object_widget_force_redraw (
    Z_ARRANGER_OBJECT_WIDGET (self));
}

static void
on_screen_changed (
  GtkWidget *    widget,
  GdkScreen *    previous_screen,
  RegionWidget * self)
{
  recreate_pango_layouts (self, NULL);
}

/**
 * Destroys the widget completely.
 */
void
region_widget_delete (RegionWidget *self)
{
  gtk_widget_set_sensitive (
    GTK_WIDGET (self), 0);

  if (gtk_widget_is_ancestor (
        GTK_WIDGET (self),
        GTK_WIDGET (MW_TIMELINE)))
    gtk_container_remove (
      GTK_CONTAINER (MW_TIMELINE),
      GTK_WIDGET (self));

  g_object_unref (self);
}

static void
finalize (
  RegionWidget * self)
{
  REGION_WIDGET_GET_PRIVATE (self);

  if (PANGO_IS_LAYOUT (rw_prv->layout))
    g_object_unref (rw_prv->layout);

  G_OBJECT_CLASS (
    region_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
region_widget_class_init (
  RegionWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}

static void
region_widget_init (
  RegionWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), 1);
  g_object_ref (self);

  /* this should really be a singleton but ok for
   * now */
  gdk_rgba_parse (&name_bg_color, "#323232");
  name_bg_color.alpha = 0.8;

  g_signal_connect (
    G_OBJECT (self), "screen-changed",
    G_CALLBACK (on_screen_changed),  self);
  g_signal_connect (
    G_OBJECT (self), "size-allocate",
    G_CALLBACK (on_size_allocate),  self);
}
