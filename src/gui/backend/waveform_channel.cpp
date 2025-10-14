// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/waveform_channel.h"

namespace zrythm::gui
{

WaveformChannel::WaveformChannel (
  QVector<float> minValues,
  QVector<float> maxValues,
  int            pixelWidth,
  QObject *      parent)
    : QObject (parent), min_values_ (std::move (minValues)),
      max_values_ (std::move (maxValues)), pixel_width_ (pixelWidth)
{
  // Ensure vectors have the same size
  Q_ASSERT (min_values_.size () == max_values_.size ());
  Q_ASSERT (pixel_width_ > 0);
  Q_ASSERT (min_values_.size () == static_cast<qsizetype> (pixel_width_));
}

WaveformChannel::~WaveformChannel ()
{
  z_trace ("deleting WaveformChannel {}", fmt::ptr (this));
}

} // namespace zrythm::gui
