// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/color.h"

#include <QtQmlIntegration>

namespace zrythm::structure::arrangement
{

class ArrangerObjectColor : public QObject
{
  Q_OBJECT
  Q_PROPERTY (bool useColor READ useColor NOTIFY useColorChanged)
  Q_PROPERTY (QColor color READ color WRITE setColor NOTIFY colorChanged)
  QML_ELEMENT

public:
  ArrangerObjectColor (QObject * parent = nullptr) noexcept : QObject (parent)
  {
  }
  ~ArrangerObjectColor () noexcept override = default;
  Z_DISABLE_COPY_MOVE (ArrangerObjectColor)

  using Color = zrythm::utils::Color;

  // ========================================================================
  // QML Interface
  // ========================================================================

  bool useColor () const { return color_.has_value (); }

  QColor color () const
  {
    if (useColor ())
      {
        return color_->to_qcolor ();
      }
    return {};
  }
  void setColor (QColor color)
  {
    if (useColor () && color_.value ().to_qcolor () == color)
      return;

    if (!color.isValid ())
      return;

    const bool emit_use_color_changed = !useColor ();
    color_ = color;
    Q_EMIT colorChanged (color);
    if (emit_use_color_changed)
      Q_EMIT useColorChanged (true);
  }

  Q_INVOKABLE void unsetColor ()
  {
    color_.reset ();
    Q_EMIT useColorChanged (false);
  }

  Q_SIGNAL void colorChanged (QColor color);
  Q_SIGNAL void useColorChanged (bool use_color);

  // ========================================================================

private:
  friend void init_from (
    ArrangerObjectColor       &obj,
    const ArrangerObjectColor &other,
    utils::ObjectCloneType     clone_type)
  {
    obj.color_ = other.color_;
  }

  static constexpr std::string_view kColorKey = "color";
  friend void to_json (nlohmann::json &j, const ArrangerObjectColor &obj)
  {
    j[kColorKey] = obj.color_;
  }
  friend void from_json (const nlohmann::json &j, ArrangerObjectColor &obj)
  {
    j.at (kColorKey).get_to (obj.color_);
  }

private:
  /**
   * Color independent of owner (Track/Region etc.).
   *
   * If nullopt, the owner color will be used.
   */
  std::optional<Color> color_;

  BOOST_DESCRIBE_CLASS (ArrangerObjectColor, (), (), (), (color_))
};

} // namespace zrythm::structure::arrangement
