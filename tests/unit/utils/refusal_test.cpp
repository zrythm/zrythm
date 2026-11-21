// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/refusal.h"

#include <gtest/gtest.h>

namespace zrythm::utils
{

// The wire names are a stability contract: peers match on these strings
TEST (RefusalKindTest, WireNamesAreStable)
{
  EXPECT_EQ (to_string (RefusalKind::MissingParameter), "missing_parameter");
  EXPECT_EQ (to_string (RefusalKind::InvalidParameter), "invalid_parameter");
  EXPECT_EQ (to_string (RefusalKind::OutOfRange), "out_of_range");
  EXPECT_EQ (to_string (RefusalKind::NotFound), "not_found");
  EXPECT_EQ (to_string (RefusalKind::InvalidState), "invalid_state");
  EXPECT_EQ (to_string (RefusalKind::Unavailable), "unavailable");
  EXPECT_EQ (to_string (RefusalKind::StaleRevision), "stale_revision");
  EXPECT_EQ (to_string (RefusalKind::CyclicRoute), "cyclic_route");
  EXPECT_EQ (to_string (RefusalKind::TypeMismatch), "type_mismatch");
  EXPECT_EQ (to_string (RefusalKind::AlreadyExists), "already_exists");
  EXPECT_EQ (to_string (RefusalKind::Unsupported), "unsupported");
}

} // namespace zrythm::utils
