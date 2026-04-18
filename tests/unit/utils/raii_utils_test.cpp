// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/raii_utils.h"

#include <gtest/gtest.h>

namespace zrythm::utils
{

TEST (ScopedBoolTest, SetsTrueOnConstruction)
{
  bool flag = false;
  {
    ScopedBool sb (flag);
  }
  EXPECT_FALSE (flag);
}

TEST (ScopedBoolTest, SetsFalseOnDestruction)
{
  bool flag = false;
  {
    ScopedBool sb (flag);
    EXPECT_TRUE (flag);
  }
  EXPECT_FALSE (flag);
}

TEST (ScopedBoolTest, NestedScopes)
{
  bool flag = false;
  {
    ScopedBool sb1 (flag);
    EXPECT_TRUE (flag);
  }
  EXPECT_FALSE (flag);
  {
    ScopedBool sb2 (flag);
    EXPECT_TRUE (flag);
  }
  EXPECT_FALSE (flag);
}

} // namespace zrythm::utils
