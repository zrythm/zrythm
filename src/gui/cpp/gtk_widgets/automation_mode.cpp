// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "common/utils/gtk.h"
#include "common/utils/ui.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/automation_mode.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

void
AutomationModeWidget::init ()
{
#if 0
  gdk_rgba_parse (
    &self->def_color, UI_COLOR_BUTTON_NORMAL);
  gdk_rgba_parse (
    &self->hovered_color, UI_COLOR_BUTTON_HOVER);
#endif
  def_color_ = Z_GDK_RGBA_INIT (1, 1, 1, 0.1f);
  hovered_color_ = Z_GDK_RGBA_INIT (1, 1, 1, 0.2f);
  toggled_colors_[0] = UI_COLORS->solo_checked.to_gdk_rgba ();
  held_colors_[0] = UI_COLORS->solo_active.to_gdk_rgba ();
  gdk_rgba_parse (&toggled_colors_[1], UI_COLOR_RECORD_CHECKED);
  gdk_rgba_parse (&held_colors_[1], UI_COLOR_RECORD_ACTIVE);
  gdk_rgba_parse (&toggled_colors_[2], "#444444");
  gdk_rgba_parse (&held_colors_[2], "#888888");
  aspect_ = 1.0;
  corner_radius_ = 8.0;

  /* set dimensions */
  int  total_width = 0;
  int  x_px, y_px;
  char txt[400];
#define DO(caps) \
  if (AutomationMode::caps == AutomationMode::Record) \
    { \
      auto str = fmt::format ("{:t}", owner_->record_mode_); \
      strcpy (txt, str.c_str ()); \
    } \
  else \
    { \
      auto str = fmt::format ("{:t}", AutomationMode::caps); \
      strcpy (txt, str.c_str ()); \
    } \
  pango_layout_set_text (layout_, txt, -1); \
  pango_layout_get_pixel_size (layout_, &x_px, &y_px); \
  text_widths_[static_cast<int> (AutomationMode::caps)] = x_px; \
  text_heights_[static_cast<int> (AutomationMode::caps)] = y_px; \
  if (y_px > max_text_height_) \
    max_text_height_ = y_px; \
  total_width += x_px

  DO (Read);
  DO (Record);
  DO (Off);

  width_ =
    total_width + AUTOMATION_MODE_HPADDING * 6
    + AUTOMATION_MODE_HSEPARATOR_SIZE * 2;
}

AutomationModeWidget::AutomationModeWidget (
  int               height,
  PangoLayout *     layout,
  AutomationTrack * owner)
{
  this->owner_ = owner;
  this->height_ = height;
  this->layout_ = pango_layout_copy (layout);
  PangoFontDescription * desc = pango_font_description_from_string ("7");
  pango_layout_set_font_description (this->layout_, desc);
  pango_layout_set_ellipsize (this->layout_, PANGO_ELLIPSIZE_NONE);
  pango_font_description_free (desc);
  init ();
}

AutomationMode
AutomationModeWidget::get_hit_mode (double x) const
{
  for (
    int i = 0; i < static_cast<int> (AutomationMode::NUM_AUTOMATION_MODES) - 1;
    i++)
    {
      int total_widths = 0;
      for (int j = 0; j <= i; j++)
        {
          total_widths += text_widths_[j];
        }
      total_widths += 2 * AUTOMATION_MODE_HPADDING * (i + 1);
      double next_start = x_ + total_widths;
      /*z_info ("[{}] x: {:f} next start: {:f}",*/
      /*i, x, next_start);*/
      if (x < next_start)
        {
          return (AutomationMode) i;
        }
    }
  return AutomationMode::Off;
}

void
AutomationModeWidget::get_color_for_state (
  CustomButtonWidget::State state,
  AutomationMode            mode,
  GdkRGBA *                 c)
{
  switch (state)
    {
    case CustomButtonWidget::State::NORMAL:
      *c = def_color_;
      break;
    case CustomButtonWidget::State::HOVERED:
      *c = hovered_color_;
      break;
    case CustomButtonWidget::State::ACTIVE:
      *c = held_colors_[static_cast<int> (mode)];
      break;
    case CustomButtonWidget::State::TOGGLED:
      *c = toggled_colors_[static_cast<int> (mode)];
      break;
    default:
      z_warn_if_reached ();
    }
}

void
AutomationModeWidget::draw_bg (
  GtkSnapshot * snapshot,
  double        x,
  double        y,
  int           draw_frame,
  bool          keep_clip)
{
  GskRoundedRect  rounded_rect;
  graphene_rect_t graphene_rect = Z_GRAPHENE_RECT_INIT (
    (float) x, (float) y, (float) width_, (float) height_);
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &graphene_rect, (float) corner_radius_);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);

  if (draw_frame)
    {
      const float border_width = 1.f;
      GdkRGBA     border_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.3f);
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
  AutomationMode draw_order[static_cast<int> (
    AutomationMode::NUM_AUTOMATION_MODES)] = {
    AutomationMode::Read, AutomationMode::Off, AutomationMode::Record
  };
  for (
    int idx = 0; idx < static_cast<int> (AutomationMode::NUM_AUTOMATION_MODES);
    idx++)
    {
      AutomationMode cur = draw_order[idx];
      int            i = static_cast<int> (cur);

      CustomButtonWidget::State cur_state = current_states_[i];
      GdkRGBA                   c;
      get_color_for_state (cur_state, cur, &c);
      if (last_states_[i] != cur_state)
        {
          transition_frames_ = CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES;
        }

      /* draw transition if transition frames exist */
      if (transition_frames_)
        {
          Color mid_c = Color::get_mid_color (
            c, last_colors_[i],
            1.f
              - (float) transition_frames_
                  / CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES);
          c = mid_c.to_gdk_rgba ();
          if (cur == AutomationMode::Off)
            transition_frames_--;
        }
      last_colors_[i] = c;

      double new_x = 0;
      int    new_width = 0;
      switch (cur)
        {
        case AutomationMode::Read:
          new_x = x;
          new_width =
            text_widths_[static_cast<int> (AutomationMode::Read)]
            + 2 * AUTOMATION_MODE_HPADDING;
          break;
        case AutomationMode::Record:
          new_x =
            x + text_widths_[static_cast<int> (AutomationMode::Read)]
            + 2 * AUTOMATION_MODE_HPADDING;
          new_width =
            text_widths_[static_cast<int> (AutomationMode::Record)]
            + 2 * AUTOMATION_MODE_HPADDING;
          break;
        case AutomationMode::Off:
          new_x =
            x + text_widths_[static_cast<int> (AutomationMode::Read)]
            + 4 * AUTOMATION_MODE_HPADDING
            + text_widths_[static_cast<int> (AutomationMode::Record)];
          new_width = ((int) x + width_) - (int) new_x;
          break;
        default:
          z_warn_if_reached ();
          break;
        }

      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) new_x, (float) y, (float) new_width, (float) height_);
        gtk_snapshot_append_color (snapshot, &c, &tmp_r);
      }
    }

  if (!keep_clip)
    gtk_snapshot_pop (snapshot);
}

void
AutomationModeWidget::draw (
  GtkSnapshot *             snapshot,
  double                    x,
  double                    y,
  double                    x_cursor,
  CustomButtonWidget::State state)
{
  /* get hit button */
  has_hit_mode_ = 0;
  if (
    state == CustomButtonWidget::State::HOVERED
    || state == CustomButtonWidget::State::ACTIVE)
    {
      has_hit_mode_ = 1;
      hit_mode_ = get_hit_mode (x_cursor);
      /*z_info ("hit mode {}", self->hit_mode);*/
    }

  /* get current states */
  for (
    unsigned int i = 0;
    i < static_cast<unsigned int> (AutomationMode::NUM_AUTOMATION_MODES); i++)
    {
      AutomationMode cur = static_cast<AutomationMode> (i);
      AutomationMode prev_am = owner_->automation_mode_;
      if (has_hit_mode_ && cur == hit_mode_)
        {
          if (
            prev_am != cur
            || (prev_am == cur && state == CustomButtonWidget::State::ACTIVE))
            {
              current_states_[i] = state;
            }
          else
            {
              current_states_[i] = CustomButtonWidget::State::TOGGLED;
            }
        }
      else
        current_states_[i] =
          owner_->automation_mode_ == cur
            ? CustomButtonWidget::State::TOGGLED
            : CustomButtonWidget::State::NORMAL;
    }

  draw_bg (snapshot, x, y, false, true);

  /*draw_icon_with_shadow (self, cr, x, y, state);*/

  /* draw text */
  int total_text_widths = 0;
  for (
    int i = 0; i < static_cast<int> (AutomationMode::NUM_AUTOMATION_MODES); i++)
    {
      AutomationMode cur = static_cast<AutomationMode> (i);
      PangoLayout *  layout = layout_;
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
          (float) (x + AUTOMATION_MODE_HPADDING
                   + i * (2 * AUTOMATION_MODE_HPADDING) + total_text_widths),
          (float) ((y + height_ / 2) - text_heights_[i] / 2));
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      char mode_str[400];
      if (cur == AutomationMode::Record)
        {
          auto str = fmt::format ("{:t}", owner_->record_mode_);
          strcpy (mode_str, str.c_str ());
          pango_layout_set_text (layout, mode_str, -1);
        }
      else
        {
          auto str = fmt::format ("{:t}", (AutomationMode) i);
          strcpy (mode_str, str.c_str ());
          pango_layout_set_text (layout, mode_str, -1);
        }
      GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 1);
      gtk_snapshot_append_layout (snapshot, layout, &tmp_color);
      gtk_snapshot_restore (snapshot);

      total_text_widths += text_widths_[i];

      last_states_[i] = current_states_[i];
    }

  /* pop clip from draw_bg */
  gtk_snapshot_pop (snapshot);
}

AutomationModeWidget::~AutomationModeWidget ()
{
  /* TODO */
}
