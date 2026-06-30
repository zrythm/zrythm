// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/isettings_backend.h"

#include <QHash>
#include <QString>
#include <QVariant>

namespace zrythm::test_helpers
{

/// Simple in-memory ISettingsBackend that stores values in a hash map.
class InMemorySettingsBackend : public utils::ISettingsBackend
{
public:
  QVariant
  value (QAnyStringView key, const QVariant &defaultValue) const override
  {
    auto it = values_.find (key.toString ());
    return it != values_.end () ? *it : defaultValue;
  }

  void setValue (QAnyStringView key, const QVariant &val) override
  {
    values_[key.toString ()] = val;
  }

private:
  QHash<QString, QVariant> values_;
};

} // namespace zrythm::test_helpers
