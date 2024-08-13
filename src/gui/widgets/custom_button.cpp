// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "gui/widgets/custom_button.h"
#include "utils/gtk.h"
#include "utils/logger.h"
#include "utils/ui.h"
#include "zrythm_app.h"

void
CustomButtonWidget::init ()
{
  def_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.1);
  hovered_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.2);
  z_return_if_fail (UI_COLORS);
  toggled_color = UI_COLORS->bright_orange;
  held_color = UI_COLORS->bright_orange;
  aspect = 1.0;
  corner_radius = 2.0;
}

CustomButtonWidget::CustomButtonWidget (const std::string &icon_name, int size)
    : icon_name (icon_name), size (size)
{
  init ();

  const int texture_size = size - 2;
  icon_texture = z_gdk_texture_new_from_icon_name (
    icon_name.c_str (), texture_size, texture_size, 1);
  if (!icon_texture)
    {
      z_error ("Failed to get icon surface for {}", icon_name);
      return;
    }
}

GdkRGBA
CustomButtonWidget::get_color_for_state (State state) const
{
  switch (state)
    {
    case State::NORMAL:
      return def_color.to_gdk_rgba ();
    case State::HOVERED:
      return hovered_color.to_gdk_rgba ();
    case State::ACTIVE:
      return held_color.to_gdk_rgba ();
    case State::TOGGLED:
      return toggled_color.to_gdk_rgba ();
    case State::SEMI_TOGGLED:
      return def_color.to_gdk_rgba ();
    default:
      z_return_val_if_reached (GdkRGBA{});
    }
}

void
CustomButtonWidget::draw_bg (
  GtkSnapshot * snapshot,
  double        x,
  double        y,
  double        width,
  int           draw_frame,
  State         state)
{
  GskRoundedRect  rounded_rect;
  graphene_rect_t graphene_rect =
    Z_GRAPHENE_RECT_INIT ((float) x, (float) y, (float) width, (float) size);
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &graphene_rect, (float) corner_radius);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);

  if (draw_frame)
    {
      const float border_width = 1.f;
      GdkRGBA     border_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.3);
      float       border_widths[] = {
        border_width, border_width, border_width, border_width
      };
      GdkRGBA border_colors[] = {
        border_color, border_color, border_color, border_color
      };
      gtk_snapshot_append_border (
        snapshot, &rounded_rect, border_widths, border_colors);
    }

  /* draw bg with fade from last state */
  GdkRGBA c = get_color_for_state (state);
  if (last_state != state)
    {
      transition_frames = CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES;
    }

  /* draw transition if transition frames exist */
  if (transition_frames)
    {
      auto mid_c = Color::get_mid_color (
        c, last_color,
        1.f
          - (float) transition_frames
              / CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES);
      c = mid_c.to_gdk_rgba ();
      transition_frames--;
    }
  last_color = c;

  gtk_snapshot_append_color (snapshot, &c, &graphene_rect);
  if (state == State::SEMI_TOGGLED)
    {
      c = get_color_for_state (State::TOGGLED);
      const float border_width = 1.f;
      float       border_widths[] = {
        border_width, border_width, border_width, border_width
      };
      GdkRGBA border_colors[] = { c, c, c, c };
      gtk_snapshot_append_border (
        snapshot, &rounded_rect, border_widths, border_colors);
    }

  /* pop rounded clip */
  gtk_snapshot_pop (snapshot);
}

void
CustomButtonWidget::
  draw_icon_with_shadow (GtkSnapshot * snapshot, double x, double y, State state)
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
  {
    graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
      (float) (x + 1), (float) (y + 1), (float) size - 2, (float) size - 2);
    gtk_snapshot_append_texture (snapshot, icon_texture, &tmp_r);
  }
}

void
CustomButtonWidget::draw (GtkSnapshot * snapshot, double x, double y, State state)
{
  draw_bg (snapshot, x, y, size, false, state);

  draw_icon_with_shadow (snapshot, x, y, state);

  last_state = state;
}

void
CustomButtonWidget::draw_with_text (
  GtkSnapshot * snapshot,
  double        x,
  double        y,
  double        width,
  State         state)
{
  draw_bg (snapshot, x, y, width, 0, state);

  draw_icon_with_shadow (snapshot, x, y, state);

  /* draw text */
  gtk_snapshot_save (snapshot);
  float text_x = (float) (x + size + 2);
  {
    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
      text_x, (float) ((y + size / 2) - text_height / 2));
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  PangoLayout * layout = this->layout;
  pango_layout_set_text (layout, text.c_str (), -1);
  pango_layout_set_width (layout, pango_units_from_double (x + width - text_x));
  GdkRGBA white = Z_GDK_RGBA_INIT (1, 1, 1, 1);
  gtk_snapshot_append_layout (snapshot, layout, &white);
  gtk_snapshot_restore (snapshot);

  this->width = (int) width;
  last_state = state;
}

void
CustomButtonWidget::set_text (
  PangoLayout *      layout,
  const std::string &text,
  const std::string &font_descr)
{
  z_return_if_fail (!text.empty () && layout);

  this->text = text;
  this->layout = pango_layout_copy (layout);
  PangoFontDescription * desc =
    pango_font_description_from_string (font_descr.c_str ());
  pango_layout_set_font_description (this->layout, desc);
  pango_font_description_free (desc);
  pango_layout_get_pixel_size (this->layout, nullptr, &this->text_height);
}

CustomButtonWidget::~CustomButtonWidget ()
{
  g_object_unref (layout);
  g_object_unref (icon_texture);
}
