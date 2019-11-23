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

/** Background color for the name. */
/*static GdkRGBA name_bg_color;*/

void
region_recreate_pango_layouts (
  Region * self)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (!PANGO_IS_LAYOUT (self->layout))
    {
      PangoFontDescription *desc;
      self->layout =
        gtk_widget_create_pango_layout (
          GTK_WIDGET (
            arranger_object_get_arranger (obj)),
          NULL);
      desc =
        pango_font_description_from_string (
          REGION_NAME_FONT);
      pango_layout_set_font_description (
        self->layout, desc);
      pango_font_description_free (desc);
      pango_layout_set_ellipsize (
        self->layout, PANGO_ELLIPSIZE_END);
    }
  pango_layout_set_width (
    self->layout,
    pango_units_from_double (
      obj->draw_rect.width - REGION_NAME_PADDING_R));
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_background (
  Region *       self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  ArrangerObject * obj =
    (ArrangerObject *) self;

  Track * track =
    arranger_object_get_track (obj);

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
    obj->hover,
    region_is_selected (self),
    /* FIXME */
    0);
  gdk_cairo_set_source_rgba (
    cr, &color);

  /* draw arc-rectangle */
  g_message ("drawing");
  z_cairo_rounded_rectangle (
    cr, 0, 0, obj->draw_rect.width,
    obj->draw_rect.height,
    1.0, 4.0);
  cairo_fill (cr);
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_loop_points (
  Region *       self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  double dashes[] = { 5 };
  cairo_set_dash (
    cr, dashes, 1, 0);
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba (
    cr, 0, 0, 0, 1.0);

  ArrangerObject * obj =
    (ArrangerObject *) self;
  Position tmp;
  long loop_start_ticks =
    obj->loop_start_pos.total_ticks;
  long loop_end_ticks =
    obj->loop_end_pos.total_ticks;
  g_warn_if_fail (
    loop_end_ticks > loop_start_ticks);
  long loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      obj);
  long clip_start_ticks =
    obj->clip_start_pos.total_ticks;

  position_from_ticks (
    &tmp, loop_start_ticks - clip_start_ticks);
  int px =
    ui_pos_to_px_timeline (&tmp, 0);
  if (px != 0 &&
      /* if loop px is visible */
      px + obj->full_rect.x >=
        rect->x &&
      px + obj->full_rect.x <=
        rect->x + rect->width)
    {
      cairo_set_source_rgba (
        cr, 0, 1, 0, 1.0);
      cairo_move_to (cr, px, 0);
      cairo_line_to (
        cr, px,
        obj->draw_rect.height);
      cairo_stroke (cr);
    }

  int num_loops =
    arranger_object_get_num_loops (obj, 1);
  for (int i = 0; i < num_loops; i++)
    {
      position_from_ticks (
        &tmp, loop_end_ticks + loop_ticks * i);

      /* adjust for clip_start */
      position_add_ticks (
        &tmp, - clip_start_ticks);

      /* note: this is relative to the region */
      px = ui_pos_to_px_timeline (&tmp, 0);

      g_message ("draw rect x %d rect x %d px %d", obj->draw_rect.x, rect->x, px);
      if (
        px + obj->full_rect.x >=
          rect->x &&
        px + obj->full_rect.x <=
          rect->x + rect->width)
        {
          cairo_set_source_rgba (
            cr, 0, 0, 0, 1.0);
          cairo_move_to (cr, px, 0);
          cairo_line_to (
            cr, px, obj->draw_rect.height);
          cairo_stroke (cr);
        }
    }

  cairo_set_dash (
    cr, NULL, 0, 0);
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_midi_region (
  Region *       self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  ArrangerObject * obj =
    (ArrangerObject *) self;

  cairo_set_source_rgba (
    cr, 1, 1, 1, 1);
  int num_loops =
    arranger_object_get_num_loops (obj, 1);
  long ticks_in_region =
    arranger_object_get_length_in_ticks (obj);
  float x_start, y_start, x_end;

  int width = obj->full_rect.width;
  int height = obj->full_rect.height;

  int min_val = 127, max_val = 0;
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * mn = self->midi_notes[i];

      if (mn->val < min_val)
        min_val = mn->val;
      if (mn->val > max_val)
        max_val = mn->val;
    }
  float y_interval =
    MAX (
      (float) (max_val - min_val) + 1.f, 7.f);
  float y_note_size = 1.f / y_interval;

  /* draw midi notes */
  long loop_end_ticks =
    obj->loop_end_pos.total_ticks;
  long loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      obj);
  long clip_start_ticks =
    obj->clip_start_pos.total_ticks;

  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * mn = self->midi_notes[i];
      ArrangerObject * mn_obj =
        (ArrangerObject *) mn;

      /* get ratio (0.0 - 1.0) on x where midi note
       * starts & ends */
      long mn_start_ticks =
        position_to_ticks (&mn_obj->pos);
      long mn_end_ticks =
        position_to_ticks (&mn_obj->end_pos);
      long tmp_start_ticks, tmp_end_ticks;

      /* if before loop end */
      if (position_is_before (
            &mn_obj->pos,
            &obj->loop_end_pos))
        {
          for (int j = 0; j < num_loops; j++)
            {
              /* if note started before loop start
               * only draw it once */
              if (position_is_before (
                    &mn_obj->pos,
                    &obj->loop_start_pos) &&
                  j != 0)
                break;

              /* calculate draw endpoints */
              tmp_start_ticks =
                mn_start_ticks + loop_ticks * j;
              /* if should be clipped */
              if (position_is_after_or_equal (
                    &mn_obj->end_pos,
                    &obj->loop_end_pos))
                tmp_end_ticks =
                  loop_end_ticks + loop_ticks * j;
              else
                tmp_end_ticks =
                  mn_end_ticks + loop_ticks * j;

              /* adjust for clip start */
              tmp_start_ticks -= clip_start_ticks;
              tmp_end_ticks -= clip_start_ticks;

              /* get ratios (0.0 - 1.0) of
               * where midi note is */
              x_start =
                (float) tmp_start_ticks /
                (float) ticks_in_region;
              x_end =
                (float) tmp_end_ticks /
                (float) ticks_in_region;
              y_start =
                ((float) max_val - (float) mn->val) /
                y_interval;

              /* get actual values using the
               * ratios */
              x_start *= (float) width;
              x_end *= (float) width;
              y_start *= (float) height;

              /* skip if any part of the note is
               * not visible in the rect */
              if ((x_start >= obj->draw_rect.x &&
                   x_start < obj->draw_rect.x + obj->draw_rect.width) ||
                  (x_end >= obj->draw_rect.x &&
                   x_end < obj->draw_rect.x + obj->draw_rect.width) ||
                  (x_start < obj->draw_rect.x &&
                   x_end > obj->draw_rect.x))
                {
                  /* draw */
                  cairo_rectangle (
                    cr,
                    MAX (
                      x_start -
                        (float) obj->draw_rect.x, 0.f),
                    MAX (
                      y_start -
                        (float) obj->draw_rect.y, 0.f),
                    MIN (
                      x_end -
                        MAX (
                          x_start,
                          (float) obj->draw_rect.x),
                      (float) obj->draw_rect.width),
                    MIN (
                      y_note_size *
                        (float) height -
                        (float) obj->draw_rect.y,
                      (float) obj->draw_rect.height));
                  cairo_fill (cr);
                }
            }
        }
    }
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_name (
  Region *       self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  /* no need to draw if the start of the region is
   * not visible */
  ArrangerObject * obj =
    (ArrangerObject *) self;
  if (obj->draw_rect.x - obj->full_rect.x >
      800)
    return;

  g_return_if_fail (
    self && self->name);

  char str[200];
  strcpy (str, self->name);

  /* draw dark bg behind text */
  region_recreate_pango_layouts (self);
  PangoLayout * layout = self->layout;
  pango_layout_set_text (
    layout, str, -1);
  PangoRectangle pangorect;
  /* get extents */
  pango_layout_get_pixel_extents (
    layout, NULL, &pangorect);
  GdkRGBA name_bg_color;
  gdk_rgba_parse (&name_bg_color, "#323232");
  name_bg_color.alpha = 0.8;
  gdk_cairo_set_source_rgba (
    cr, &name_bg_color);
  double radius = REGION_NAME_BOX_CURVINESS / 1.0;
  double degrees = G_PI / 180.0;
  cairo_new_sub_path (cr);
  cairo_move_to (
    cr, pangorect.width + REGION_NAME_PADDING_R, 0);
  cairo_arc (
    cr, (pangorect.width + REGION_NAME_PADDING_R) - radius,
    REGION_NAME_BOX_HEIGHT - radius, radius,
    0 * degrees, 90 * degrees);
  cairo_line_to (cr, 0, REGION_NAME_BOX_HEIGHT);
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
 * Draws the Region in the given cairo context in
 * absolute coordinates.
 *
 * @param rect Arranger rectangle.
 */
void
region_draw (
  Region *       self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  draw_background (self, cr, rect);
  draw_loop_points (self, cr, rect);

  switch (self->type)
    {
    case REGION_TYPE_MIDI:
      {
        draw_midi_region (self, cr, rect);
      }
      break;
    default:
      break;
    }
  draw_name (self, cr, rect);
}

#if 0

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
    0);
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
#endif
