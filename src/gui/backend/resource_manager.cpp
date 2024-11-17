#include "resource_manager.h"

QUrl
ResourceManager::getResourceUrl (const QString &relPath)
{
  return QUrl (QStringLiteral ("qrc:/qt/qml/Zrythm/") + relPath);
}

QUrl
ResourceManager::getIconUrl (const QString &iconPack, const QString &iconFileName)
{
  return getResourceUrl (
    QStringLiteral ("icons/") + iconPack + QStringLiteral ("/") + iconFileName);
}
