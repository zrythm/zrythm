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

static gboolean
region_draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  RegionWidget * self)
{
  REGION_WIDGET_GET_PRIVATE (self);
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  if (ao_prv->redraw)
    {
      Region * r = rw_prv->region;

      GtkStyleContext *context =
        gtk_widget_get_style_context (widget);
      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);

      ao_prv->cached_surface =
        cairo_surface_create_similar (
          cairo_get_target (cr),
          CAIRO_CONTENT_COLOR_ALPHA,
          width, height);
      ao_prv->cached_cr =
        cairo_create (ao_prv->cached_surface);

      gtk_render_background (
        context, ao_prv->cached_cr,
        0, 0, width, height);

      Track * track = NULL;
      if (TRACKLIST &&
          TRACKLIST->num_tracks >
            rw_prv->region->track_pos)
        track =
          TRACKLIST->tracks[rw_prv->region->track_pos];

      /* set color */
      GdkRGBA color;
      if (track)
        color = track->color;
      else if (TESTING)
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
        ao_prv->cached_cr, &color);

      z_cairo_rounded_rectangle (
        ao_prv->cached_cr, 0, 0,
        width, height, 1.0, 4.0);
      cairo_fill (ao_prv->cached_cr);
      /*cairo_set_source_rgba (rw_prv->cached_cr,*/
                             /*color->red,*/
                             /*color->green,*/
                             /*color->blue,*/
                             /*1.0);*/
      /*cairo_rectangle(rw_prv->cached_cr, 0, 0, width, height);*/
      /*cairo_set_line_width (rw_prv->cached_cr, 3.5);*/
      /*cairo_stroke (rw_prv->cached_cr);*/

      /* draw loop points */
      double dashes[] = { 5 };
      cairo_set_dash (
        ao_prv->cached_cr, dashes, 1, 0);
      cairo_set_line_width (ao_prv->cached_cr, 1);
      cairo_set_source_rgba (
        ao_prv->cached_cr, 0, 0, 0, 1.0);

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
      if (px != 0)
        {
          cairo_set_source_rgba (
            ao_prv->cached_cr, 0, 1, 0, 1.0);
          cairo_move_to (ao_prv->cached_cr, px, 0);
          cairo_line_to (
            ao_prv->cached_cr, px, height);
          cairo_stroke (ao_prv->cached_cr);
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

          if (px <= (int) width - 1)
            {
              cairo_set_source_rgba (
                ao_prv->cached_cr, 0, 0, 0, 1.0);
              cairo_move_to (
                ao_prv->cached_cr, px, 0);
              cairo_line_to (
                ao_prv->cached_cr, px, height);
              cairo_stroke (
                ao_prv->cached_cr);
            }
        }

      ao_prv->redraw = 0;
    }

  cairo_set_source_surface (
    cr, ao_prv->cached_surface, 0, 0);
  cairo_paint (cr);

 return FALSE;
}

/**
 * Draws the cut line if in cut mode.
 */
void
region_widget_draw_cut_line (
  RegionWidget * self,
  cairo_t *      cr)
{
  REGION_WIDGET_GET_PRIVATE (self);
  Region * r = rw_prv->region;
  ArrangerObject * r_obj =
    (ArrangerObject *) r;

  if (rw_prv->show_cut)
    {
      int start_pos_px;
      double px;
      int height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));

      /* get absolute position */
      start_pos_px =
        ui_pos_to_px_timeline (
          &r_obj->pos, F_PADDING);
      Position pos;
      ui_px_to_pos_timeline (
        start_pos_px + rw_prv->hover_x,
        &pos, F_PADDING);

      /* snap */
      Track * track =
        arranger_object_get_track (r_obj);
      ArrangerWidgetPrivate * ar_prv =
        arranger_widget_get_private (
          track->pinned ?
            Z_ARRANGER_WIDGET (MW_PINNED_TIMELINE) :
            Z_ARRANGER_WIDGET (MW_TIMELINE));
      if (!ar_prv->shift_held)
        position_snap_simple (
          &pos, SNAP_GRID_TIMELINE);

      /* convert back to px */
      px =
        ui_pos_to_px_timeline (
          &pos, F_PADDING);

      /* convert to local px */
      px -= start_pos_px;

      double dashes[] = { 5 };
      cairo_set_dash (cr, dashes, 1, 0);
      cairo_set_source_rgba (
        cr, 1, 1, 1, 1.0);
      cairo_move_to (cr, px, 0);
      cairo_line_to (cr, px, height);
      cairo_stroke (cr);
    }
}

/**
 * Draws the name of the Region.
 *
 * To be called during a cairo draw callback.
 */
void
region_widget_draw_name (
  RegionWidget * self,
  cairo_t *      cr)
{
  REGION_WIDGET_GET_PRIVATE (self);

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

static int
on_motion (GtkWidget *      widget,
           GdkEventMotion * event,
           RegionWidget *   self)
{
  if (!MAIN_WINDOW || !MW_TIMELINE)
    return FALSE;

  ARRANGER_WIDGET_GET_PRIVATE (MW_TIMELINE);
  REGION_WIDGET_GET_PRIVATE (self);

  int alt_pressed =
    event->state & GDK_MOD1_MASK;

  if (event->type == GDK_MOTION_NOTIFY)
    {
      rw_prv->show_cut =
        region_widget_should_show_cut_lines (
          alt_pressed);
      rw_prv->resize_l =
        region_widget_is_resize_l (
          self, (int) event->x);
      rw_prv->resize_r =
        region_widget_is_resize_r (
          self, (int) event->x);
      rw_prv->resize_loop =
        region_widget_is_resize_loop (
          self, (int) event->y);
      rw_prv->hover_x = (int) event->x;
    }

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
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
      rw_prv->show_cut = 0;
    }
  rw_prv->redraw = 1;

  return FALSE;
}

/**
 * Returns if region widgets should show cut lines.
 *
 * @param alt_pressed Whether alt is currently
 *   pressed.
 */
int
region_widget_should_show_cut_lines (
  int alt_pressed)
{
  switch (P_TOOL)
    {
    case TOOL_SELECT_NORMAL:
    case TOOL_SELECT_STRETCH:
      if (alt_pressed)
        return 1;
      else
        return 0;
      break;
    case TOOL_CUT:
      return 1;
      break;
    default:
      return 0;
      break;
    }
  g_return_val_if_reached (-1);
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

  rw_prv->region = region;

  rw_prv->drawing_area =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (rw_prv->drawing_area));
  gtk_widget_set_visible (
    GTK_WIDGET (rw_prv->drawing_area), 1);
  gtk_widget_set_hexpand (
    GTK_WIDGET (rw_prv->drawing_area), 1);

  /* GDK_ALL_EVENTS_MASK is needed, otherwise the
   * grab gets broken */
  gtk_widget_add_events (
    GTK_WIDGET (rw_prv->drawing_area),
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
  REGION_WIDGET_GET_PRIVATE (self);
  rw_prv->redraw = 1;
  gtk_widget_queue_draw (
    GTK_WIDGET (self));
}

static void
on_screen_changed (
  GtkWidget *    widget,
  GdkScreen *    previous_screen,
  RegionWidget * self)
{
  recreate_pango_layouts (self, NULL);
}

void
region_widget_force_redraw (
  RegionWidget * self)
{
  REGION_WIDGET_GET_PRIVATE (self);
  rw_prv->redraw = 1;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Destroys the widget completely.
 */
void
region_widget_delete (RegionWidget *self)
{
  REGION_WIDGET_GET_PRIVATE (self);

  gtk_widget_set_sensitive (
    GTK_WIDGET (self), 0);
  gtk_container_remove (
    GTK_CONTAINER (self),
    GTK_WIDGET (rw_prv->drawing_area));

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

  REGION_WIDGET_GET_PRIVATE (self);
  rw_prv->redraw = 1;
}
