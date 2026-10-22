// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/color.h"

#include <QColor>
#include <QObject>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::utils
{

/**
 * @brief Color-space conversion and WCAG contrast helpers exposed to QML,
 * implemented in utils::Color.
 */
class ColorMath : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  /**
   * @brief Converts a color to OKLCH coordinates.
   *
   * @return [lightness, chroma, hue in degrees in [0, 360)]
   */
  Q_INVOKABLE static QVariantList rgbToOklch (const QColor &color)
  {
    const auto oklch = Color (color).to_oklch ();
    return { oklch.lightness, oklch.chroma, oklch.hue };
  }

  /**
   * @brief Converts OKLCH coordinates to a color.
   *
   * The chroma is reduced until the color fits the sRGB gamut. The result
   * is opaque; a negative chroma yields opaque black.
   *
   * @param l Lightness.
   * @param c Chroma.
   * @param h Hue in degrees.
   */
  Q_INVOKABLE static QColor oklchColor (double l, double c, double h)
  {
    return Color::from_oklch (l, c, h).to_qcolor ();
  }

  /**
   * @brief Returns the WCAG 2.x contrast ratio between two colors (1 to 21).
   */
  Q_INVOKABLE static double contrastRatio (const QColor &a, const QColor &b)
  {
    return Color::contrast_ratio (Color (a), Color (b));
  }
};
} // namespace zrythm::utils
