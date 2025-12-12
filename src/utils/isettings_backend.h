// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QVariant>

namespace zrythm::utils
{
/**
 * @brief Interface for an app settings provider.
 */
class ISettingsBackend
{
public:
  virtual ~ISettingsBackend () = default;
  virtual QVariant
  value (QAnyStringView key, const QVariant &defaultValue = {}) const = 0;
  virtual void setValue (QAnyStringView key, const QVariant &value) = 0;
};
}
