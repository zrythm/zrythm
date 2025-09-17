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
    LicenseIdentifierRole,
    LicenseNoticeRole,
    FullLicenseTextRole,
  };

  explicit ThirdPartyLicensesModel (QObject * parent = nullptr);

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames () const override;

private:
  struct LicenseInfo
  {
    /**
     * @brief Name of the module this license is for.
     *
     * Eg, "JUCE".
     */
    QString module_name;

    /**
     * @brief Identifier of the license (SPDX or plain text).
     *
     * Eg, "MIT or Qt proprietary license".
     */
    QString license_identifier;

    /**
     * @brief Brief license notice.
     *
     * Eg, the text at the top of JUCE source code files.
     */
    QString license_notice;

    /**
     * @brief A full copy of the license legal text.
     */
    QString full_license_text;
  };

  static std::vector<LicenseInfo> get_licenses ();
};

} // namespace zrythm::gui
