#include "gui/backend/backend/zrythm.h"
#include "gui/backend/project_manager.h"
#include "project_templates_model.h"

#include <QSettings>

using namespace zrythm::gui;

ProjectTemplatesModel::ProjectTemplatesModel (QObject * parent)
    : QAbstractListModel (parent)
{
}

std::vector<std::unique_ptr<ProjectInfo>>
ProjectTemplatesModel::get_templates ()
{
  const auto &list = ProjectManager::get_instance ()->get_templates ();

  std::vector<std::unique_ptr<ProjectInfo>> ret;

  // add blank template
  ret.emplace_back (std::make_unique<ProjectInfo> ());
  ret.back ()->setName (tr ("Blank Project"));

  std::transform (
    list.begin (), list.end (), std::back_inserter (ret),
    [] (const auto &pathstr) {
      return std::make_unique<ProjectInfo> (pathstr);
    });

  return ret;
}

int
ProjectTemplatesModel::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return (int) get_templates ().size ();
}

QVariant
ProjectTemplatesModel::data (const QModelIndex &index, int role) const
{
  if (!index.isValid ())
    return {};

  auto        templates = get_templates ();
  const auto &project = templates.at (index.row ()).get ();

  switch (role)
    {
    case PathRole:
      return project->getPath ();
    case Qt::DisplayRole:
    case NameRole:
      return project->getName ();
    case IsBlankRole:
      return project->getName () == tr ("Blank Project");
    default:
      return {};
    }

  return {};
}

QHash<int, QByteArray>
ProjectTemplatesModel::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[PathRole] = "path";
  roles[NameRole] = "name";
  return roles;
}
