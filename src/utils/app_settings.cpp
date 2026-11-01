// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
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

void
AppSettings::reset_preference_settings ()
{
  reset_themeMode ();
  reset_accentToken ();
  reset_uiLocale ();
  reset_transportReturnToCue ();
  reset_startPlaybackOnMidiInput ();
  reset_undoStackLength ();
  reset_openPluginsOnInstantiation ();
  reset_showCacheActivity ();
  reset_lv2SearchPaths ();
  reset_vst3SearchPaths ();
  reset_clapSearchPaths ();
  reset_auSearchPaths ();
}
}
