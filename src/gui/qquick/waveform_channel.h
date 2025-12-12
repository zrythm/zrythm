// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

/**
 * @brief Represents waveform data for a single audio channel.
 *
 * This class contains min/max sample values for each pixel position
 * in a waveform display, optimized for QML consumption.
 */
class WaveformChannel : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QVector<float> minValues READ minValues CONSTANT)
  Q_PROPERTY (QVector<float> maxValues READ maxValues CONSTANT)
  Q_PROPERTY (int pixelWidth READ pixelWidth CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /**
   * @brief Construct a WaveformChannel
   * @param minValues Minimum sample values for each pixel
   * @param maxValues Maximum sample values for each pixel
   * @param pixelWidth The pixel width used for generation
   * @param parent Parent QObject
   */
  WaveformChannel (
    QVector<float> minValues,
    QVector<float> maxValues,
    int            pixelWidth,
    QObject *      parent = nullptr);
  ~WaveformChannel () override;

  /**
   * @brief Get the minimum sample values for each pixel
   * @return Vector of minimum values
   */
  const QVector<float> &minValues () const { return min_values_; }

  /**
   * @brief Get the maximum sample values for each pixel
   * @return Vector of maximum values
   */
  const QVector<float> &maxValues () const { return max_values_; }

  /**
   * @brief Get the pixel width used for generation
   * @return Pixel width
   */
  int pixelWidth () const { return pixel_width_; }

private:
  /** Minimum sample values for each pixel position */
  QVector<float> min_values_;

  /** Maximum sample values for each pixel position */
  QVector<float> max_values_;

  /** The pixel width used when generating the waveform data */
  int pixel_width_;
};

} // namespace zrythm::gui
