// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "utils/app_settings.h"
#include "utils/logger.h"

namespace zrythm::utils
{
AppSettings::AppSettings (
  std::unique_ptr<ISettingsBackend> backend,
  QObject *                         parent)
    : QObject (parent), backend_ (std::move (backend))
{
}

namespace detail
{
void
log_setting_change (const char * name, const QVariant &value)
{
  z_debug ("setting '{}' to '{}'", name, value);
}
}
}
