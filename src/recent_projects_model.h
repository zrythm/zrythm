#pragma once

#include "project_info.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QStringList>
#include <QtQmlIntegration>

namespace zrythm::gui
{

class RecentProjectsModel : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

public:
  enum RecentProjectRoles
  {
    NameRole = Qt::UserRole + 1,
    PathRole,
    DateRole,
  };

  explicit RecentProjectsModel (QObject * parent = nullptr);

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames () const override;

  Q_INVOKABLE void addRecentProject (const QString &path);
  Q_INVOKABLE void removeRecentProject (const QString &path);
  Q_INVOKABLE void clearRecentProjects ();

private:
  static std::vector<std::unique_ptr<ProjectInfo>> get_recent_projects ();
  static void store_recent_projects (const QStringList &list);

  static constexpr int MAX_RECENT_DOCUMENTS = 12;
};

} // namespace zrythm::gui