// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cmath>

#include "dsp/chord_descriptor.h"
#include "dsp/musical_scale.h"
#include "gui/qquick/chord_highlighter.h"

namespace zrythm::gui::qquick
{

namespace
{

constexpr std::array<qreal, 4> kRotations = {
  0.0, 210.0 / 360.0, 120.0 / 360.0, 330.0 / 360.0
};

constexpr std::array<qreal, 4> kBaseAlphas = { 0.6, 0.65, 0.65, 0.75 };

using zrythm::dsp::ChordDescriptor;
using zrythm::dsp::MusicalNote;
using zrythm::dsp::MusicalScale;

int
levelForNote (
  int                     note,
  const ChordDescriptor * chord,
  const MusicalScale *    scale,
  int                     mode)
{
  if (note < 0 || note > 11 || mode == 0)
    return -1;

  const bool showChord = mode == 1 || mode == 3;
  const bool showScale = mode == 2 || mode == 3;
  const auto mn = static_cast<MusicalNote> (note);
  const bool inChord = chord != nullptr && chord->isKeyInChord (mn);
  const bool isBass = chord != nullptr && chord->isKeyBass (mn);
  const bool inScale = scale != nullptr && scale->containsNote (mn);

  if (showChord && isBass)
    return 3;
  if (showChord && showScale && inChord && inScale)
    return 2;
  if (showScale && inScale)
    return 0;
  if (showChord && inChord)
    return 1;
  return -1;
}

QColor
colorForLevel (const QColor &base, int level, qreal alpha)
{
  const QColor hsv = base.toHsv ();
  qreal        h = hsv.hueF ();
  if (h < 0.0)
    h = 0.0;
  qreal rotatedH = std::fmod (h + kRotations[level], 1.0);
  if (rotatedH < 0.0)
    rotatedH += 1.0;
  QColor out;
  out.setHsvF (
    static_cast<float> (rotatedH), 0.8f, 0.85f, static_cast<float> (alpha));
  return out;
}

} // namespace

QColor
ChordHighlighter::highlightColorForNote (
  QColor            base,
  int               note,
  ChordDescriptor * chord,
  MusicalScale *    scale,
  int               highlightMode,
  qreal             alphaScale)
{
  const int level = levelForNote (note, chord, scale, highlightMode);
  if (level < 0)
    return QColor (Qt::transparent);
  return colorForLevel (base, level, kBaseAlphas[level] * alphaScale);
}

QVariantList
ChordHighlighter::highlightColors (
  QColor            base,
  ChordDescriptor * chord,
  MusicalScale *    scale,
  int               highlightMode,
  qreal             alphaScale)
{
  QVariantList result;
  result.reserve (12);
  for (int i = 0; i < 12; ++i)
    result.append (
      highlightColorForNote (base, i, chord, scale, highlightMode, alphaScale));
  return result;
}

} // namespace zrythm::gui::qquick
