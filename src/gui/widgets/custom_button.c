/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdbool.h>
#include <stdlib.h>

#include "gui/widgets/custom_button.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

static void
init (CustomButtonWidget * self)
{
#if 0
  gdk_rgba_parse (
    &self->def_color, UI_COLOR_BUTTON_NORMAL);
  gdk_rgba_parse (
    &self->hovered_color, UI_COLOR_BUTTON_HOVER);
#endif
  self->def_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.1);
  self->hovered_color =
    Z_GDK_RGBA_INIT (1, 1, 1, 0.2);
  g_return_if_fail (UI_COLORS);
  self->toggled_color = UI_COLORS->bright_orange;
  self->held_color = UI_COLORS->bright_orange;
  self->aspect = 1.0;
  self->corner_radius = 2.0;
}

/**
 * Updates the drawing caches.
 */
/*static void*/
/*update_caches (*/
/*CustomButtonWidget * self)*/
/*{*/
/*}*/

/**
 * Creates a new track widget from the given track.
 */
CustomButtonWidget *
custom_button_widget_new (
  const char * icon_name,
  int          size)
{
  CustomButtonWidget * self =
    object_new (CustomButtonWidget);

  strcpy (self->icon_name, icon_name);
  self->size = size;
  init (self);

  const int texture_size = self->size - 2;
  self->icon_texture =
    z_gdk_texture_new_from_icon_name (
      self->icon_name, texture_size, texture_size, 1);
  if (!self->icon_texture)
    {
      g_critical (
        "Failed to get icon surface for %s",
        self->icon_name);
      free (self);
      return NULL;
    }

  return self;
}

static void
get_color_for_state (
  CustomButtonWidget *    self,
  CustomButtonWidgetState state,
  GdkRGBA *               c)
{
  (void) c;
  switch (state)
    {
    case CUSTOM_BUTTON_WIDGET_STATE_NORMAL:
      *c = self->def_color;
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_HOVERED:
      *c = self->hovered_color;
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_ACTIVE:
      *c = self->held_color;
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_TOGGLED:
      *c = self->toggled_color;
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_SEMI_TOGGLED:
      *c = self->def_color;
      break;
    default:
      g_warn_if_reached ();
    }
}

static void
draw_bg (
  CustomButtonWidget *    self,
  GtkSnapshot *           snapshot,
  double                  x,
  double                  y,
  double                  width,
  int                     draw_frame,
  CustomButtonWidgetState state)
{
  GskRoundedRect rounded_rect;
  graphene_rect_t graphene_rect = GRAPHENE_RECT_INIT (
    (float) x, (float) y, (float) width, self->size);
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &graphene_rect,
    (float) self->corner_radius);
  gtk_snapshot_push_rounded_clip (
    snapshot, &rounded_rect);

  if (draw_frame)
    {
      const float border_width = 1.f;
      GdkRGBA     border_color =
        Z_GDK_RGBA_INIT (1, 1, 1, 0.3);
      float border_widths[] = {
        border_width, border_width, border_width,
        border_width
      };
      GdkRGBA border_colors[] = {
        border_color, border_color, border_color,
        border_color
      };
      gtk_snapshot_append_border (
        snapshot, &rounded_rect, border_widths,
        border_colors);
    }

  /* draw bg with fade from last state */
  GdkRGBA c;
  get_color_for_state (self, state, &c);
  if (self->last_state != state)
    {
      self->transition_frames =
        CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES;
    }

  /* draw transition if transition frames exist */
  if (self->transition_frames)
    {
      GdkRGBA mid_c;
      ui_get_mid_color (
        &mid_c, &c, &self->last_color,
        1.f
          - self->transition_frames
              / CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES);
      c = mid_c;
      self->transition_frames--;
    }
  self->last_color = c;

  gtk_snapshot_append_color (
    snapshot, &c, &graphene_rect);
  if (state == CUSTOM_BUTTON_WIDGET_STATE_SEMI_TOGGLED)
    {
      get_color_for_state (
        self, CUSTOM_BUTTON_WIDGET_STATE_TOGGLED,
        &c);
      const float border_width = 1.f;
      float       border_widths[] = {
        border_width, border_width, border_width,
        border_width
      };
      GdkRGBA border_colors[] = { c, c, c, c };
      gtk_snapshot_append_border (
        snapshot, &rounded_rect, border_widths,
        border_colors);
    }

  /* pop rounded clip */
  gtk_snapshot_pop (snapshot);
}

static void
draw_icon_with_shadow (
  CustomButtonWidget *    self,
  GtkSnapshot *           snapshot,
  double                  x,
  double                  y,
  CustomButtonWidgetState state)
{
  /* TODO */
#if 0
  /* show icon with shadow */
  cairo_set_source_rgba (
    cr, 0, 0, 0, 0.4);
  cairo_mask_surface (
    cr, self->icon_surface, x + 1, y + 1);
  cairo_fill (cr);
#endif

  /* add main icon */
  gtk_snapshot_append_texture (
    snapshot, self->icon_texture,
    &GRAPHENE_RECT_INIT (
      (float) (x + 1), (float) (y + 1),
      (float) self->size - 2,
      (float) self->size - 2));
}

void
custom_button_widget_draw (
  CustomButtonWidget *    self,
  GtkSnapshot *           snapshot,
  double                  x,
  double                  y,
  CustomButtonWidgetState state)
{
  draw_bg (
    self, snapshot, x, y, self->size, false, state);

  draw_icon_with_shadow (
    self, snapshot, x, y, state);

  self->last_state = state;
}

/**
 * @param width Max width for the button to use.
 */
void
custom_button_widget_draw_with_text (
  CustomButtonWidget *    self,
  GtkSnapshot *           snapshot,
  double                  x,
  double                  y,
  double                  width,
  CustomButtonWidgetState state)
{
  draw_bg (self, snapshot, x, y, width, 0, state);

  draw_icon_with_shadow (
    self, snapshot, x, y, state);

  /* draw text */
  gtk_snapshot_save (snapshot);
  float text_x = (float) (x + self->size + 2);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (
      text_x,
      (float) ((y + self->size / 2) - self->text_height / 2)));
  PangoLayout * layout = self->layout;
  pango_layout_set_text (layout, self->text, -1);
  pango_layout_set_width (
    layout,
    pango_units_from_double (x + width - text_x));
  gtk_snapshot_append_layout (
    snapshot, layout, &Z_GDK_RGBA_INIT (1, 1, 1, 1));
  gtk_snapshot_restore (snapshot);

  self->width = (int) width;
  self->last_state = state;
}

/**
 * Sets the text and layout to draw the text width.
 *
 * @param font_descr Font description to set the
 *   pango layout font to.
 */
void
custom_button_widget_set_text (
  CustomButtonWidget * self,
  PangoLayout *        layout,
  char *               text,
  const char *         font_descr)
{
  g_return_if_fail (text && layout);

  self->text = g_strdup (text);
  self->layout = pango_layout_copy (layout);
  PangoFontDescription * desc =
    pango_font_description_from_string (font_descr);
  pango_layout_set_font_description (
    self->layout, desc);
  pango_font_description_free (desc);
  pango_layout_get_pixel_size (
    self->layout, NULL, &self->text_height);
}

void
custom_button_widget_free (CustomButtonWidget * self)
{
  object_free_w_func_and_null (g_free, self->text);
  object_free_w_func_and_null (
    g_object_unref, self->layout);
  object_free_w_func_and_null (
    g_object_unref, self->icon_texture);

  object_zero_and_free (self);
}
