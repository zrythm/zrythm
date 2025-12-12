// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/app_settings.h"

namespace zrythm::utils
{
AppSettings::AppSettings (
  std::unique_ptr<ISettingsBackend> backend,
  QObject *                         parent)
    : QObject (parent), backend_ (std::move (backend))
{
}
}
