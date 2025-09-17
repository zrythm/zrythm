// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense OR curl

// Note: the above SPDX field is a hacky way to make sure reuse lint doesn't
// complain about unused licenses.

#include "gui/backend/qml_utils.h"
#include "gui/backend/third_party_licenses_model.h"

namespace zrythm::gui
{

ThirdPartyLicensesModel::ThirdPartyLicensesModel (QObject * parent)
    : QAbstractListModel (parent)
{
}

std::vector<ThirdPartyLicensesModel::LicenseInfo>
ThirdPartyLicensesModel::get_licenses ()
{
  const auto get_spdx_license_text = [] (const QString &license_name) {
    return QmlUtils::readTextFileContent (
      QString (":/qt/qml/Zrythm/licenses/") + license_name + QString (".txt"));
  };

  return {
    {
     .module_name = QString ("Qt %1").arg (QT_VERSION_STR),
     .license_identifier = "LGPL-3.0-only",
     .license_notice =
        "Copyright by The Qt Company. See https://doc.qt.io/qt-6/licensing.html. Qt also incorporates third party libraries under compatible licenses. See https://doc.qt.io/qt-6/licenses-used-in-qt.html for license/copyright notices of other third party libraries.",
     .full_license_text = get_spdx_license_text ("LGPL-3.0-only"),
     },
    {
     .module_name =
        QString ("JUCE %1.%2.%3")
          .arg (JUCE_MAJOR_VERSION)
          .arg (JUCE_MINOR_VERSION)
          .arg (JUCE_BUILDNUMBER),
     .license_identifier = "AGPL-3.0-only",
     .license_notice =
        R"(JUCE itself is licensed under the license shown below. JUCE also includes third-party libraries under compatible licenses. See https://github.com/juce-framework/JUCE/blob/master/LICENSE.md#the-juce-framework-dependencies for license/copyright notices of other third party libraries.

JUCE License Notice:
Copyright (c) Raw Material Software Limited

JUCE is an open source framework subject to commercial
or open source licensing.

By downloading, installing, or using the JUCE framework, or combining the JUCE framework with any other source code, object code, content or any other copyrightable work, you agree to the terms of the JUCE End User Licence Agreement, and all incorporated terms including the JUCE Privacy Policy and the JUCE Website Terms of Service, as applicable, which will bind you. If you do not agree to the terms of these agreements, we will not license the JUCE framework to you, and you must discontinue the installation or download process and cease use of the JUCE framework.

JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
JUCE Privacy Policy: https://juce.com/juce-privacy-policy
JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

Or:

You may also use this code under the terms of the AGPLv3 :
https://www.gnu.org/licenses/agpl-3.0.en.html

THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED)",
     .full_license_text = get_spdx_license_text ("AGPL-3.0-only"),
     },
    {
     .module_name = QString ("kissfft"),
     .license_identifier = "BSD-3-Clause",
     .license_notice = "Copyright by Mark Borgerding",
     .full_license_text = get_spdx_license_text ("BSD-3-Clause"),
     },
    {
     .module_name = QString ("zita-resampler"),
     .license_identifier = "GPL-3.0-or-later",
     .license_notice = "Copyright by Fons Adriaensen",
     .full_license_text = get_spdx_license_text ("GPL-3.0-or-later"),
     },
    {
     .module_name = QString ("SoX Resampler Library"),
     .license_identifier = "LGPL-2.1-or-later",
     .license_notice = "Copyright by robs@users.sourceforge.net",
     .full_license_text = get_spdx_license_text ("LGPL-2.1-or-later"),
     },
    {
     .module_name = QString ("crill"),
     .license_identifier = "BSL-1.0",
     .license_notice = "Copyright by Timur Doumler and Fabian Renn-Giles",
     .full_license_text = get_spdx_license_text ("BSL-1.0"),
     },
    {
     .module_name = QString ("LightweightSemaphore"),
     .license_identifier = "Zlib",
     .license_notice = "Copyright by Cameron Desrochers, Jeff Preshing",
     .full_license_text = get_spdx_license_text ("Zlib"),
     },
    {
     .module_name = QString ("fmt"),
     .license_identifier = "MIT",
     .license_notice = "Copyright by Victor Zverovich",
     .full_license_text = get_spdx_license_text ("MIT"),
     },
    {
     .module_name = QString ("spdlog"),
     .license_identifier = "MIT",
     .license_notice = "Copyright by Gabi Melman",
     .full_license_text = get_spdx_license_text ("MIT"),
     },
    {
     .module_name = QString ("magic_enum"),
     .license_identifier = "MIT",
     .license_notice = "Copyright by Daniil Goncharov",
     .full_license_text = get_spdx_license_text ("MIT"),
     },
    {
     .module_name = QString ("zstd"),
     .license_identifier = "BSD-3-Clause",
     .license_notice = "Copyright by Meta Platforms, Inc. and affiliates",
     .full_license_text = get_spdx_license_text ("BSD-3-Clause"),
     },
    {
     .module_name = QString ("CLAP"),
     .license_identifier = "MIT",
     .license_notice =
        "Copyright by 2014...2022 Alexandre BIQUE <bique.alexandre@gmail.com>",                                                                                                                                                                                                                                                                    .full_license_text = get_spdx_license_text ("MIT"),
     },
    {
     .module_name = QString ("curl"),
     .license_identifier = "curl",
     .license_notice =
        "Copyright by 1996 - 2025, Daniel Stenberg, daniel@haxx.se, and many contributors",
     .full_license_text = get_spdx_license_text ("curl"),
     },
    {
     .module_name = QString ("nlohmann_json"),
     .license_identifier = "MIT",
     .license_notice = "Copyright by Niels Lohmann",
     .full_license_text = get_spdx_license_text ("MIT"),
     },
    {
     .module_name = QString ("scnlib"),
     .license_identifier = "Apache-2.0",
     .license_notice = "Copyright by Elias Kosunen",
     .full_license_text = get_spdx_license_text ("Apache-2.0"),
     },
    {
     .module_name = QString ("type_safe"),
     .license_identifier = "MIT",
     .license_notice = "Copyright (c) 2016-2020 Jonathan Müller",
     .full_license_text = get_spdx_license_text ("MIT"),
     },
  };
}

int
ThirdPartyLicensesModel::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return (int) get_licenses ().size ();
}

QVariant
ThirdPartyLicensesModel::data (const QModelIndex &index, int role) const
{
  if (!index.isValid ())
    return {};

  auto        licenses = get_licenses ();
  const auto &license = licenses.at (index.row ());

  switch (role)
    {
    case Qt::DisplayRole:
    case NameRole:
      return license.module_name;
    case LicenseIdentifierRole:
      return license.license_identifier;
    case LicenseNoticeRole:
      return license.license_notice;
    case FullLicenseTextRole:
      return license.full_license_text;
    default:
      return {};
    }

  return {};
}

QHash<int, QByteArray>
ThirdPartyLicensesModel::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[NameRole] = "name";
  roles[LicenseIdentifierRole] = "licenseIdentifier";
  roles[LicenseNoticeRole] = "licenseNotice";
  roles[FullLicenseTextRole] = "licenseText";
  return roles;
}
}
