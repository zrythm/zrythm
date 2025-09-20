// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QtQmlIntegration>

class QmlUtils : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  Q_INVOKABLE static QString toPathString (const QUrl &url);
  Q_INVOKABLE static QUrl    localFileToQUrl (const QString &path);

  Q_INVOKABLE static float       amplitudeToDbfs (float amplitude);
  Q_INVOKABLE static QStringList splitTextLines (const QString &text);
  Q_INVOKABLE static QStringList removeDuplicates (const QStringList &list);
  Q_INVOKABLE static QString     readTextFileContent (const QString &filePath);

  Q_INVOKABLE static QColor saturate (const QColor &color, float perc);
  Q_INVOKABLE static QColor makeBrighter (const QColor &color, float perc);
};
