// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "project_json_serializer_test.h"

#include <fmt/format.h>

namespace zrythm::controllers
{

// ============================================================================
// Parameterized Tests for Missing Required Fields
// ============================================================================
// These tests verify that the schema correctly rejects JSON with missing
// required fields. This is a schema behavior test, not a C++ serialization test.
// ============================================================================

namespace
{

class MissingRequiredFieldTest : public testing::TestWithParam<std::string>
{
protected:
  void SetUp () override { j_ = create_minimal_valid_project_json (); }
  nlohmann::json j_;
};

TEST_P (MissingRequiredFieldTest, TopLevelFieldMissing_Throws)
{
  j_.erase (GetParam ());
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j_); },
    utils::exceptions::ZrythmException);
}

INSTANTIATE_TEST_SUITE_P (
  ProjectJsonSerializerValidationTest,
  MissingRequiredFieldTest,
  testing::Values (
    "documentType",
    "schemaVersion",
    "appVersion",
    "datetime",
    "title",
    "projectData"),
  [] (const testing::TestParamInfo<std::string> &param_info) {
    return "Missing_" + param_info.param;
  });

class MissingProjectDataFieldTest : public testing::TestWithParam<std::string>
{
protected:
  void SetUp () override { j_ = create_minimal_valid_project_json (); }
  nlohmann::json j_;
};

TEST_P (MissingProjectDataFieldTest, ProjectDataFieldMissing_Throws)
{
  j_["projectData"].erase (GetParam ());
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j_); },
    utils::exceptions::ZrythmException);
}

INSTANTIATE_TEST_SUITE_P (
  ProjectJsonSerializerValidationTest,
  MissingProjectDataFieldTest,
  testing::Values ("tempoMap", "transport", "tracklist", "registries"),
  [] (const testing::TestParamInfo<std::string> &param_info) {
    return "Missing_" + param_info.param;
  });

class MissingRegistryTest : public testing::TestWithParam<std::string>
{
protected:
  void SetUp () override { j_ = create_minimal_valid_project_json (); }
  nlohmann::json j_;
};

TEST_P (MissingRegistryTest, RegistryMissing_Throws)
{
  j_["projectData"]["registries"].erase (GetParam ());
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j_); },
    utils::exceptions::ZrythmException);
}

INSTANTIATE_TEST_SUITE_P (
  ProjectJsonSerializerValidationTest,
  MissingRegistryTest,
  testing::Values (
    "portRegistry",
    "paramRegistry",
    "pluginRegistry",
    "trackRegistry",
    "arrangerObjectRegistry",
    "fileAudioSourceRegistry"),
  [] (const testing::TestParamInfo<std::string> &param_info) {
    return "Missing_" + param_info.param;
  });

class MissingTracklistFieldTest : public testing::TestWithParam<std::string>
{
protected:
  void SetUp () override { j_ = create_minimal_valid_project_json (); }
  nlohmann::json j_;
};

TEST_P (MissingTracklistFieldTest, TracklistFieldMissing_Throws)
{
  j_["projectData"]["tracklist"].erase (GetParam ());
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j_); },
    utils::exceptions::ZrythmException);
}

INSTANTIATE_TEST_SUITE_P (
  ProjectJsonSerializerValidationTest,
  MissingTracklistFieldTest,
  testing::Values ("tracks", "pinnedTracksCutoff", "trackRoutes"),
  [] (const testing::TestParamInfo<std::string> &param_info) {
    return "Missing_" + param_info.param;
  });

} // namespace

// ============================================================================
// Schema Validation Tests - Invalid Data Rejection
// ============================================================================
// These tests verify that the schema correctly rejects invalid data values.
// This is a schema behavior test (constraints, ranges, formats).
// ============================================================================

TEST (ProjectJsonSerializerValidationTest, InvalidJsonWrongDocumentType)
{
  auto j = create_minimal_valid_project_json ();
  j["documentType"] = "WrongDocumentType";

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonInvalidUuidFormat)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "not-a-valid-uuid";
  track["type"] = 2;
  track["name"] = "Bad Track";

  j["projectData"]["registries"]["trackRegistry"].push_back (track);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonInvalidColorFormat)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 2;
  track["name"] = "Bad Color Track";
  track["color"] = "red";

  j["projectData"]["registries"]["trackRegistry"].push_back (track);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonInvalidTrackType)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 999;
  track["name"] = "Invalid Type Track";

  j["projectData"]["registries"]["trackRegistry"].push_back (track);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonEmpty)
{
  nlohmann::json j = nlohmann::json::object ();

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonNotObject)
{
  nlohmann::json j = "not an object";

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

// ============================================================================
// Edge Case Tests
// ============================================================================
// These tests verify edge cases that are hard to create via C++ objects
// or test schema behavior with unusual but valid inputs.
// ============================================================================

TEST (ProjectJsonSerializerValidationTest, ValidateJson_EmptyTitle)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = "";
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_UnicodeInTitle)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = "测试项目 🎵";
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_UnicodeInTrackName)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 2;
  track["name"] = "钢琴轨道 🎹";
  j["projectData"]["registries"]["trackRegistry"].push_back (track);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_WhitespaceInStrings)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = "  Project with spaces  ";
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_SpecialCharactersInTitle)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = "Project \"with quotes\" and \\backslash\\";
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_VeryLongTitle)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = std::string (1000, 'a');
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

}
