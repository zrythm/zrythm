// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QItemSelectionModel>
#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

class QmlUtils : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  Q_INVOKABLE static QString toPathString (const QUrl &url);
  Q_INVOKABLE static QUrl    localFileToQUrl (const QString &path);
  Q_INVOKABLE static QString pathBasename (const QString &path);

  Q_INVOKABLE static float       amplitudeToDbfs (float amplitude);
  Q_INVOKABLE static QStringList splitTextLines (const QString &text);
  Q_INVOKABLE static QStringList removeDuplicates (const QStringList &list);
  Q_INVOKABLE static QString     readTextFileContent (const QString &filePath);

  Q_INVOKABLE static QColor saturate (const QColor &color, float perc);
  Q_INVOKABLE static QColor makeBrighter (const QColor &color, float perc);
  Q_INVOKABLE static QColor
  adjustOpacity (const QColor &color, float newOpacity);

  Q_INVOKABLE static QVector<float>
  getAutomationRegionValues (QObject * automationRegion, int pixelWidth);

  Q_INVOKABLE static bool rectanglesIntersect (QRectF a, QRectF b);
  Q_INVOKABLE static bool rectanglesIntersect (QRect a, QRect b);

  /**
   * @brief Helper to create a selection from a list of rows in a single column.
   */
  Q_INVOKABLE static QItemSelection createRowSelection (
    QAbstractItemModel * model,
    const QList<int>    &rows,
    int                  column = 0);

  /**
   * @brief Helper to create a contiguous block/range selection.
   */
  Q_INVOKABLE static QItemSelection createRangeSelection (
    QAbstractItemModel * model,
    int                  startRow,
    int                  endRow,
    int                  startCol = 0,
    int                  endCol = 0);
};
}
