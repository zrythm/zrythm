// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/qml_utils.h"
#include "utils/string.h"

QString
QmlUtils::toPathString (const QUrl &url)
{
  return utils::string::qurl_to_path_qstring (url);
}