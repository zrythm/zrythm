// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "project_info.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QStringList>
#include <QtQmlIntegration>

namespace zrythm::gui
{

class ProjectTemplatesModel : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

public:
  enum ProjectTemplateRoles
  {
    NameRole = Qt::UserRole + 1,
    PathRole,
    IsBlankRole,
  };

  explicit ProjectTemplatesModel (QObject * parent = nullptr);

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames () const override;

private:
  static std::vector<std::unique_ptr<ProjectInfo>> get_templates ();
};

} // namespace zrythm::gui