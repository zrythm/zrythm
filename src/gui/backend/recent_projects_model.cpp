#include "gui/backend/backend/settings_manager.h"

#include <QSettings>

#include "recent_projects_model.h"

using namespace zrythm::gui;

RecentProjectsModel::RecentProjectsModel (QObject * parent)
    : QAbstractListModel (parent)
{
}

std::vector<std::unique_ptr<ProjectInfo>>
RecentProjectsModel::get_recent_projects ()
{
  auto list =
    zrythm::gui::SettingsManager::get_instance ()->get_recent_projects ();
  // list.append ("test 2");

  std::vector<std::unique_ptr<ProjectInfo>> ret;
  std::ranges::transform (
    list, std::back_inserter (ret), [] (const auto &pathstr) {
      return std::make_unique<ProjectInfo> (fs::path (pathstr.toStdString ()));
    });

  return ret;
}

int
RecentProjectsModel::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return (int) get_recent_projects ().size ();
}

QVariant
RecentProjectsModel::data (const QModelIndex &index, int role) const
{
  if (!index.isValid ())
    return {};

  const auto   recent_projects = get_recent_projects ();
  const auto * project = recent_projects.at (index.row ()).get ();

  switch (role)
    {
    case PathRole:
      return project->getPath ();
    case Qt::DisplayRole:
    case NameRole:
      return project->getName ();
    case DateRole:
      return project->getLastSavedAt ();
    default:
      return {};
    }

  return {};
}

QHash<int, QByteArray>
RecentProjectsModel::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[PathRole] = "path";
  roles[NameRole] = "name";
  roles[DateRole] = "date";
  return roles;
}

void
RecentProjectsModel::addRecentProject (const QString &path)
{
  beginResetModel ();
  auto recent_projects =
    SettingsManager::get_instance ()->get_recent_projects ();
  recent_projects.removeAll (path);
  recent_projects.prepend (path);
  while (recent_projects.size () > MAX_RECENT_DOCUMENTS)
    {
      recent_projects.removeLast ();
    }
  store_recent_projects (recent_projects);
  endResetModel ();
}

void
RecentProjectsModel::removeRecentProject (const QString &path)
{
  beginResetModel ();
  auto recent_projects =
    SettingsManager::get_instance ()->get_recent_projects ();
  recent_projects.removeAll (path);
  store_recent_projects (recent_projects);
  endResetModel ();
}

void
RecentProjectsModel::store_recent_projects (const QStringList &list)
{
  zrythm::gui::SettingsManager::get_instance ()->set_recent_projects (list);
}

void
RecentProjectsModel::clearRecentProjects ()
{
  beginResetModel ();
  store_recent_projects ({});
  endResetModel ();
}