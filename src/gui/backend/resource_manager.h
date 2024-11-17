#pragma once

#include <QtQmlIntegration>

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
