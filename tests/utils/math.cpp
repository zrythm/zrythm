// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "utils/math.h"

#include "doctest_wrapper.h"

TEST_SUITE_BEGIN ("utils/math");

TEST_CASE ("NaNs in conversions")
{
  float ret;
  ret = math_get_fader_val_from_amp (1);
  math_assert_nonnann (ret);
}

TEST_SUITE_END;