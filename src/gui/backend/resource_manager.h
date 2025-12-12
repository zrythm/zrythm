// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QtQmlIntegration/qqmlintegration.h>

class ResourceManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  Q_INVOKABLE static QUrl getResourceUrl (const QString &relPath);
  Q_INVOKABLE static QUrl
  getIconUrl (const QString &iconPack, const QString &iconFileName);
};
