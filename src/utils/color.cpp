// SPDX-FileCopyrightText: © 2020-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <ranges>

#include "utils/color.h"
#include "utils/debug.h"
#include "utils/math_utils.h"

#include <nlohmann/json.hpp>

namespace zrythm::utils
{

namespace
{

// A color channel converted to its WCAG 2.x linear-light value
double
linear_channel (double v)
{
  return v <= 0.03928 ? v / 12.92 : std::pow ((v + 0.055) / 1.055, 2.4);
}

// A linear-light channel converted back to its sRGB value, clamped to
// [0, 1]
double
srgb_channel (double v)
{
  return std::clamp (
    v <= 0.0031308 ? 12.92 * v : 1.055 * std::pow (v, 1.0 / 2.4) - 0.055, 0.0,
    1.0);
}

} // namespace

Color::Color (std::string_view hex) : Color (from_hex (hex)) { }

Color::Color (float r, float g, float b, float a) noexcept
    : red_ (r), green_ (g), blue_ (b), alpha_ (a)
{
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
  return utils::math::floats_near (red_, other.red_, epsilon)
         && utils::math::floats_near (green_, other.green_, epsilon)
         && utils::math::floats_near (blue_, other.blue_, epsilon)
         && utils::math::floats_near (alpha_, other.alpha_, epsilon);
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
    (amt_inv * red_) + (amt * other.red_),
    (amt_inv * green_) + (amt * other.green_),
    (amt_inv * blue_) + (amt * other.blue_), alpha_
  };
}

QColor
Color::to_qcolor () const
{
  return QColor::fromRgbF (red_, green_, blue_, alpha_);
}

Oklch
Color::to_oklch () const
{
  const double r = linear_channel (red_);
  const double g = linear_channel (green_);
  const double b = linear_channel (blue_);
  const double lms0 =
    std::cbrt (0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
  const double lms1 =
    std::cbrt (0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
  const double lms2 =
    std::cbrt (0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);
  const double lab_a =
    1.9779984951 * lms0 - 2.4285922050 * lms1 + 0.4505937099 * lms2;
  const double lab_b =
    0.0259040371 * lms0 + 0.7827717662 * lms1 - 0.8086757660 * lms2;
  double hue = std::atan2 (lab_b, lab_a) * 180.0 / std::numbers::pi;
  if (hue < 0)
    hue += 360.0;
  return {
    .lightness = 0.2104542553 * lms0 + 0.7936177850 * lms1 - 0.0040720468 * lms2,
    .chroma = std::hypot (lab_a, lab_b),
    .hue = hue,
  };
}

Color
Color::from_oklch (double lightness, double chroma, double hue)
{
  z_return_val_if_fail_cmp (chroma, >=, 0.0, Color ());

  // bisection iterations for the chroma reduction, and the tolerance the
  // linear channels may leave the [0, 1] range by
  constexpr int    BISECTION_STEPS = 24;
  constexpr double GAMUT_TOLERANCE = 0.001;

  const double rad = hue * std::numbers::pi / 180.0;
  const double ca = std::cos (rad);
  const double cb = std::sin (rad);
  const auto   to_linear_rgb = [lightness, ca, cb] (double c) {
    const double aa = c * ca;
    const double bb = c * cb;
    const double lms0 = std::pow (
      0.9999999985 * lightness + 0.3963377922 * aa + 0.2158037581 * bb, 3.0);
    const double lms1 = std::pow (
      1.0000000089 * lightness - 0.1055613423 * aa - 0.0638541748 * bb, 3.0);
    const double lms2 = std::pow (
      1.0000000547 * lightness - 0.0894841821 * aa - 1.2914855379 * bb, 3.0);
    return std::array<double, 3>{
      4.0767416613 * lms0 - 3.3077115904 * lms1 + 0.2309699287 * lms2,
      -1.2684380041 * lms0 + 2.6097574007 * lms1 - 0.3413193963 * lms2,
      -0.0041960865 * lms0 - 0.7034186145 * lms1 + 1.7076147009 * lms2
    };
  };
  const auto in_gamut = [] (const std::array<double, 3> &rgb) {
    return std::ranges::all_of (rgb, [] (double v) {
      return v >= -GAMUT_TOLERANCE && v <= 1.0 + GAMUT_TOLERANCE;
    });
  };

  const auto rgb = [&] {
    auto candidate = to_linear_rgb (chroma);
    if (in_gamut (candidate))
      return candidate;
    double lo = 0.0;
    double hi = chroma;
    for (const auto _ : std::views::iota (0, BISECTION_STEPS))
      {
        const double mid = (lo + hi) / 2.0;
        if (in_gamut (to_linear_rgb (mid)))
          lo = mid;
        else
          hi = mid;
      }
    return to_linear_rgb (lo);
  }();

  return {
    static_cast<float> (srgb_channel (rgb[0])),
    static_cast<float> (srgb_channel (rgb[1])),
    static_cast<float> (srgb_channel (rgb[2])),
    1.0f,
  };
}

Color
Color::from_oklch (const Oklch &oklch)
{
  return from_oklch (oklch.lightness, oklch.chroma, oklch.hue);
}

double
Color::luminance () const
{
  return 0.2126 * linear_channel (red_) + 0.7152 * linear_channel (green_)
         + 0.0722 * linear_channel (blue_);
}

double
Color::contrast_ratio (const Color &a, const Color &b)
{
  const double la = a.luminance ();
  const double lb = b.luminance ();
  return (std::max (la, lb) + 0.05) / (std::min (la, lb) + 0.05);
}

Color
Color::get_mid_color (const Color &c1, const Color &c2, const float transition)
{
  return {
    (c1.red_ * transition) + (c2.red_ * (1.f - transition)),
    (c1.green_ * transition) + (c2.green_ * (1.f - transition)),
    (c1.blue_ * transition) + (c2.blue_ * (1.f - transition)),
    (c1.alpha_ * transition) + (c2.alpha_ * (1.f - transition))
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
    static_cast<int> (green * 255.0), static_cast<int> (blue * 255.0));
}

std::string
Color::to_hex () const
{
  return rgb_to_hex (red_, green_, blue_);
}

Color
Color::from_hex (std::string_view hex)
{
  QColor qc (hex.data ());
  return Color (qc.redF (), qc.greenF (), qc.blueF (), 1.0f);
}

void
to_json (nlohmann::json &j, const Color &color)
{
  j = color.to_hex ();
}

void
from_json (const nlohmann::json &j, Color &color)
{
  color = Color::from_hex (j.get<std::string> ());
}

bool
operator== (const Color &lhs, const Color &rhs)
{
  return utils::math::floats_equal (lhs.red_, rhs.red_)
         && utils::math::floats_equal (lhs.green_, rhs.green_)
         && utils::math::floats_equal (lhs.blue_, rhs.blue_)
         && utils::math::floats_equal (lhs.alpha_, rhs.alpha_);
}

} // namespace zrythm::utils
