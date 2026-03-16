// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/color.h"
#include "utils/icloneable.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::arrangement
{

class ArrangerObjectColor : public QObject
{
  Q_OBJECT
  Q_PROPERTY (bool useColor READ useColor NOTIFY useColorChanged)
  Q_PROPERTY (QColor color READ color WRITE setColor NOTIFY colorChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

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
  void setColor (QColor color);

  Q_INVOKABLE void unsetColor ()
  {
    color_.reset ();
    Q_EMIT useColorChanged (false);
  }

  Q_SIGNAL void colorChanged (QColor color);
  Q_SIGNAL void useColorChanged (bool use_color);

  // ========================================================================

private:
  static constexpr std::string_view kColorKey = "color";
  friend void to_json (nlohmann::json &j, const ArrangerObjectColor &obj);
  friend void from_json (const nlohmann::json &j, ArrangerObjectColor &obj);
  friend void init_from (
    ArrangerObjectColor       &obj,
    const ArrangerObjectColor &other,
    utils::ObjectCloneType     clone_type);

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
