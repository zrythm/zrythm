// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project_json_builder.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::structure::project;
using namespace std::string_view_literals;

TEST (ProjectJsonBuilderTest, Constants)
{
  // Test static constants
  EXPECT_EQ (ProjectJsonBuilder::FORMAT_MAJOR_VERSION, 2);
  EXPECT_EQ (ProjectJsonBuilder::FORMAT_MINOR_VERSION, 1);
  EXPECT_EQ (ProjectJsonBuilder::DOCUMENT_TYPE, "ZrythmProject"sv);
  EXPECT_EQ (ProjectJsonBuilder::kProject, "project"sv);
  EXPECT_EQ (ProjectJsonBuilder::kDatetimeKey, "datetime"sv);
  EXPECT_EQ (ProjectJsonBuilder::kAppVersionKey, "appVersion"sv);
}

TEST (ProjectJsonBuilderTest, ConstantValidation)
{
  // Verify that all required fields are present in the builder constants

  // Document type should be non-empty and match expected value
  EXPECT_FALSE (ProjectJsonBuilder::DOCUMENT_TYPE.empty ());
  EXPECT_EQ (ProjectJsonBuilder::DOCUMENT_TYPE, "ZrythmProject"sv);

  // Format versions should be reasonable
  EXPECT_GE (ProjectJsonBuilder::FORMAT_MAJOR_VERSION, 1);
  EXPECT_GE (ProjectJsonBuilder::FORMAT_MINOR_VERSION, 0);
  EXPECT_LE (ProjectJsonBuilder::FORMAT_MINOR_VERSION, 99);

  // All keys should be non-empty
  EXPECT_FALSE (ProjectJsonBuilder::kProject.empty ());
  EXPECT_FALSE (ProjectJsonBuilder::kDatetimeKey.empty ());
  EXPECT_FALSE (ProjectJsonBuilder::kAppVersionKey.empty ());
}
