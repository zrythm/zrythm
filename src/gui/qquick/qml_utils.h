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

  /**
   * @brief Computes a track background color with optional track-color tinting.
   *
   * When @p tint is true, blends 12% of @p trackColor into @p baseColor as a
   * base tint, then layers windowText overlays for selection (20%) and hover
   * (5%).
   *
   * When @p tint is false, skips the track color tint and applies only the
   * selection and hover overlays directly to @p baseColor.
   *
   * @param baseColor The base background color (typically palette.window).
   * @param trackColor The track's assigned color (used only when tint is true).
   * @param windowTextColor The window text color (used for selection/hover
   *   overlays).
   * @param isSelected Whether the track is currently selected.
   * @param isHovered Whether the track is currently hovered.
   * @param tint Whether to apply the 12% track-color base tint.
   */
  Q_INVOKABLE static QColor getTrackBackground (
    const QColor &baseColor,
    const QColor &trackColor,
    const QColor &windowTextColor,
    bool          isSelected,
    bool          isHovered,
    bool          tint = true);

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
