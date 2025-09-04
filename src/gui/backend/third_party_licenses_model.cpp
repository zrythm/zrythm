// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense OR curl

// Note: the above SPDX field is a hacky way to make sure reuse lint doesn't
// complain about unused licenses.

#include "third_party_licenses_model.h"

namespace zrythm::gui
{

ThirdPartyLicensesModel::ThirdPartyLicensesModel (QObject * parent)
    : QAbstractListModel (parent)
{
}

std::vector<ThirdPartyLicensesModel::LicenseInfo>
ThirdPartyLicensesModel::get_licenses ()
{
  return {
    { "Qt",                    "The Qt Company Ltd",                                      "LGPL-3.0-only"    },
    { "JUCE",                  "Raw Material Software Limited",                           "AGPL-3.0-only"    },
    { "kissfft",               "Mark Borgerding",                                         "BSD-3-Clause"     },
    { "zita-resampler",        "Fons Adriaensen",                                         "GPL-3.0-or-later" },
    { "SoX Resampler Library", "robs@users.sourceforge.net",
     "LGPL-2.1-or-later"                                                                                     },
    { "crill",                 "Timur Doumler and Fabian Renn-Giles",                     "BSL-1.0"          },
    { "LightweightSemaphore",  "Cameron Desrochers, Jeff Preshing",                       "Zlib"             },
    { "fmt",                   "Victor Zverovich",                                        "MIT"              },
    { "spdlog",                "Gabi Melman",                                             "MIT"              },
    { "magic_enum",            "Daniil Goncharov",                                        "MIT"              },
    { "zstd",                  "Meta Platforms, Inc. and affiliates",                     "BSD-3-Clause"     },
    { "CLAP",                  "2014...2022 Alexandre BIQUE <bique.alexandre@gmail.com>", "MIT"              },
    { "curl",
     "1996 - 2025, Daniel Stenberg, daniel@haxx.se, and many contributors",               "curl"             },
    { "nlohmann_json",         "Niels Lohmann",                                           "MIT"              },
    { "scnlib",                "Elias Kosunen",                                           "Apache-2.0"       },
    { "type_safe",             "2016-2020 Jonathan Müller",                               "MIT"              },
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
      return license.name;
    case CopyrightRole:
      return license.copyright;
    case LicenseRole:
      return license.license;
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
  roles[CopyrightRole] = "copyright";
  roles[LicenseRole] = "license";
  return roles;
}
}
