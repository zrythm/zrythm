// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/qml_utils.h"
#include "utils/math.h"
#include "utils/string.h"

QString
QmlUtils::toPathString (const QUrl &url)
{
  return utils::Utf8String::from_qurl (url).to_qstring ();
}

QUrl
QmlUtils::localFileToQUrl (const QString &path)
{
  return QUrl::fromLocalFile (path);
}

float
QmlUtils::amplitudeToDbfs (float amplitude)
{
  return utils::math::amp_to_dbfs (amplitude);
}
