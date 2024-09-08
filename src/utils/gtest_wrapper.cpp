// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"

bool g_running_in_test = false;

void
set_running_in_test (bool value)
{
  g_running_in_test = value;
}

bool
get_running_in_test ()
{
  return g_running_in_test;
}