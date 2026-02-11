// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/isettings_backend.h"

#include <gmock/gmock.h>

namespace zrythm::test_helpers
{

class MockSettingsBackend : public utils::ISettingsBackend
{
public:
  MOCK_METHOD (
    QVariant,
    value,
    (QAnyStringView key, const QVariant &defaultValue),
    (const, override));
  MOCK_METHOD (
    void,
    setValue,
    (QAnyStringView key, const QVariant &value),
    (override));
};

} // namespace zrythm::test_helpers
