// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QAbstractListModel>
#include <QString>
#include <QtQmlIntegration>

namespace zrythm::gui
{

class ThirdPartyLicensesModel : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

public:
  enum ThirdPartyLicenseRoles
  {
    NameRole = Qt::UserRole + 1,
    CopyrightRole,
    LicenseRole,
  };

  explicit ThirdPartyLicensesModel (QObject * parent = nullptr);

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames () const override;

private:
  struct LicenseInfo
  {
    QString name;
    QString copyright;
    QString license;
  };

  static std::vector<LicenseInfo> get_licenses ();
};

} // namespace zrythm::gui
