// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/isettings_backend.h"

#include <QSettings>

namespace zrythm::utils
{
/**
 * @brief Interface for an app settings provider.
 */
class QSettingsBackend : public ISettingsBackend
{
public:
  QVariant
  value (QAnyStringView key, const QVariant &defaultValue = {}) const override
  {
    return settings_.value (key, defaultValue);
  }
  void setValue (QAnyStringView key, const QVariant &value) override
  {
    settings_.setValue (key, value);
    settings_.sync ();
  }

  void reset_and_sync ()
  {
    settings_.clear ();
    settings_.sync ();
  }

private:
  QSettings settings_;
};
}
