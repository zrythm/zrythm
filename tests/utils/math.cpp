// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/utils/gtest_wrapper.h"
#include "common/utils/math.h"

TEST (Math, NaNsInConversions)
{
  float ret;
  ret = math_get_fader_val_from_amp (1);
  math_assert_nonnann (ret);
}