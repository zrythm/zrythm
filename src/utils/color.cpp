// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/color.h"
#include "utils/debug.h"
#include "utils/math.h"

namespace zrythm::utils
{

Color::Color (const std::string &str)
{
  QColor qc (str.c_str ());
  red_ = qc.redF ();
  green_ = qc.greenF ();
  blue_ = qc.blueF ();
  alpha_ = qc.alphaF ();
}

Color::Color (float r, float g, float b, float a)
    : red_ (r), green_ (g), blue_ (b), alpha_ (a)
{
}

Color &
Color::operator= (const GdkRGBA &color)
{
  red_ = color.red;
  green_ = color.green;
  blue_ = color.blue;
  alpha_ = color.alpha;
  return *this;
}

Color &
Color::operator= (const QColor &color)
{
  red_ = color.redF ();
  green_ = color.greenF ();
  blue_ = color.blueF ();
  alpha_ = color.alphaF ();
  return *this;
}

Color &
Color::brighten (float val)
{
  red_ = std::min (red_ + val, 1.f);
  green_ = std::min (green_ + val, 1.f);
  blue_ = std::min (blue_ + val, 1.f);
  return *this;
}

Color &
Color::brighten_default ()
{
  brighten (DEFAULT_BRIGHTEN_VAL);
  return *this;
}

Color &
Color::darken (float val)
{
  red_ = std::max (red_ - val, 0.f);
  green_ = std::max (green_ - val, 0.f);
  blue_ = std::max (blue_ - val, 0.f);
  return *this;
}

Color &
Color::darken_default ()
{
  darken (DEFAULT_BRIGHTEN_VAL);
  return *this;
}

bool
Color::is_same (const Color &other) const
{
  constexpr float epsilon = 0.000001f;
  return math_floats_equal_epsilon (red_, other.red_, epsilon)
         && math_floats_equal_epsilon (green_, other.green_, epsilon)
         && math_floats_equal_epsilon (blue_, other.blue_, epsilon)
         && math_floats_equal_epsilon (alpha_, other.alpha_, epsilon);
}

bool
Color::is_bright () const
{
  return red_ + green_ + blue_ >= 1.5f;
}

bool
Color::is_very_bright () const
{
  return red_ + green_ + blue_ >= 2.0f;
}

bool
Color::is_very_very_bright () const
{
  return red_ + green_ + blue_ >= 2.5f;
}

bool
Color::is_very_dark () const
{
  return red_ + green_ + blue_ < 1.f;
}

bool
Color::is_very_very_dark () const
{
  return red_ + green_ + blue_ < 0.5f;
}

float
Color::get_brightness () const
{
  return (red_ + green_ + blue_) / 3.f;
}

float
Color::get_darkness () const
{
  return 1.f - get_brightness ();
}

Color
Color::get_opposite () const
{
  return { 1.f - red_, 1.f - green_, 1.f - blue_, alpha_ };
}

Color
Color::morph (const Color &other, float amt) const
{
  z_return_val_if_fail_cmp (amt, >=, 0.f, *this);
  z_return_val_if_fail_cmp (amt, <=, 1.f, *this);

  float amt_inv = 1.f - amt;
  return {
    amt_inv * red_ + amt * other.red_, amt_inv * green_ + amt * other.green_,
    amt_inv * blue_ + amt * other.blue_, alpha_
  };
}

GdkRGBA
Color::to_gdk_rgba () const
{
  GdkRGBA color;
  color.red = red_;
  color.green = green_;
  color.blue = blue_;
  color.alpha = alpha_;
  return color;
}

GdkRGBA
Color::to_gdk_rgba_with_alpha (float alpha) const
{
  GdkRGBA color;
  color.red = red_;
  color.green = green_;
  color.blue = blue_;
  color.alpha = alpha;
  return color;
}

QColor
Color::to_qcolor () const
{
  return QColor::fromRgbF (red_, green_, blue_, alpha_);
}

Color
Color::get_mid_color (const Color &c1, const Color &c2, const float transition)
{
  return {
    c1.red_ * transition + c2.red_ * (1.f - transition),
    c1.green_ * transition + c2.green_ * (1.f - transition),
    c1.blue_ * transition + c2.blue_ * (1.f - transition),
    c1.alpha_ * transition + c2.alpha_ * (1.f - transition)
  };
}

Color
Color::get_contrast_color () const
{
#if 0
  if (is_bright ())
    return UI_COLORS->dark_text;
  else
    return UI_COLORS->bright_text;
#endif
  return {};
}

Color
Color::get_arranger_object_color (
  const Color &before_color,
  const bool   is_hovered,
  const bool   is_selected,
  const bool   is_transient,
  const bool   is_muted)
{
  Color color = before_color;
  color.alpha_ = is_transient ? 0.7f : 1.f;
  if (is_muted)
    {
      color.red_ = 0.6f;
      color.green_ = 0.6f;
      color.blue_ = 0.6f;
    }
  if (is_selected)
    {
      color.red_ += is_muted ? 0.2f : 0.4f;
      color.green_ += 0.2f;
      color.blue_ += 0.2f;
      color.alpha_ = 1.f;
    }
  else if (is_hovered)
    {
      if (color.is_very_bright ())
        {
          color.red_ -= 0.1f;
          color.green_ -= 0.1f;
          color.blue_ -= 0.1f;
        }
      else
        {
          color.red_ += 0.1f;
          color.green_ += 0.1f;
          color.blue_ += 0.1f;
        }
    }

  return color;
}

std::string
Color::rgb_to_hex (float red, float green, float blue)
{
  return fmt::format (
    "#{:02x}{:02x}{:02x}", static_cast<int> (red * 255.0),
    static_cast<int> (green * 255.0), static_cast<char> (blue * 255.0));
}

std::string
Color::to_hex () const
{
  return rgb_to_hex (red_, green_, blue_);
}

bool
operator== (const Color &lhs, const Color &rhs)
{
  return math_floats_equal (lhs.red_, rhs.red_)
         && math_floats_equal (lhs.green_, rhs.green_)
         && math_floats_equal (lhs.blue_, rhs.blue_)
         && math_floats_equal (lhs.alpha_, rhs.alpha_);
}

}; // namespace zrythm::utils
