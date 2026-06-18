// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>
#include <QObject>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{
class ChordDescriptor;
class MusicalScale;
}

namespace zrythm::gui::qquick
{

class ChordHighlighter : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  Q_INVOKABLE static QColor highlightColorForNote (
    QColor                         base,
    int                            note,
    zrythm::dsp::ChordDescriptor * chord,
    zrythm::dsp::MusicalScale *    scale,
    int                            highlightMode,
    qreal                          alphaScale);

  Q_INVOKABLE static QVariantList highlightColors (
    QColor                         base,
    zrythm::dsp::ChordDescriptor * chord,
    zrythm::dsp::MusicalScale *    scale,
    int                            highlightMode,
    qreal                          alphaScale);
};

} // namespace zrythm::gui::qquick
