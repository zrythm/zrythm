// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "project_json_serializer_test.h"

#include <fmt/format.h>

namespace zrythm::structure::project
{

// ============================================================================
// Parameterized Tests for Missing Required Fields
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
// Valid JSON Tests
// ============================================================================

TEST (ProjectJsonSerializerValidationTest, ValidateMinimalValidJson)
{
  auto j = create_minimal_valid_project_json ();
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJsonWithOptionalFields)
{
  auto  j = create_minimal_valid_project_json ();
  auto &pd = j["projectData"];

  pd["snapGridTimeline"] = nlohmann::json::object ();
  pd["snapGridTimeline"]["snapEnabled"] = true;
  pd["snapGridTimeline"]["snapNoteLength"] = 5;
  pd["snapGridTimeline"]["snapNoteType"] = 0;

  pd["snapGridEditor"] = nlohmann::json::object ();
  pd["snapGridEditor"]["snapEnabled"] = false;

  pd["audioPool"] = nlohmann::json::object ();
  pd["audioPool"]["fileHashes"] = nlohmann::json::array ();

  pd["portConnectionsManager"] = nlohmann::json::object ();
  pd["portConnectionsManager"]["connections"] = nlohmann::json::array ();

  pd["tempoObjectManager"] = nlohmann::json::object ();
  pd["tempoObjectManager"]["tempoObjects"] = nlohmann::json::array ();
  pd["tempoObjectManager"]["timeSignatureObjects"] = nlohmann::json::array ();

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJsonWithValidTrack)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 2;
  track["name"] = "Test MIDI Track";
  track["visible"] = true;
  track["mainHeight"] = 80.0;
  track["enabled"] = true;
  track["color"] = "#ff5588";

  j["projectData"]["registries"]["trackRegistry"].push_back (track);
  j["projectData"]["tracklist"]["tracks"].push_back (
    "550e8400-e29b-41d4-a716-446655440000");

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJsonWithValidPort)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json port;
  port["id"] = "660e8400-e29b-41d4-a716-446655440001";
  port["type"] = 0;
  port["flow"] = 1;
  port["label"] = "Stereo Out L";

  j["projectData"]["registries"]["portRegistry"].push_back (port);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJsonWithValidParameter)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json param;
  param["id"] = "770e8400-e29b-41d4-a716-446655440002";
  param["uniqueId"] = "fader-volume";
  param["baseValue"] = 0.75;

  j["projectData"]["registries"]["paramRegistry"].push_back (param);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJsonWithValidMidiNote)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json note;
  note["id"] = "880e8400-e29b-41d4-a716-446655440003";
  note["type"] = 0;
  note["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  note["length"] = {
    { "mode",  0     },
    { "value", 960.0 }
  };
  note["pitch"] = 60;
  note["velocity"] = 100;

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (note);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJsonWithValidAutomationPoint)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json ap;
  ap["id"] = "990e8400-e29b-41d4-a716-446655440004";
  ap["type"] = 7;
  ap["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ap["normalizedValue"] = 0.5;
  ap["curveOptions"] = {
    { "curviness", 0.0 },
    { "algo",      0   }
  };

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (ap);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (
  ProjectJsonSerializerValidationTest,
  ValidateJsonWithAudioRegionFadeProperties)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json ar;
  ar["id"] = "aa0e8400-e29b-41d4-a716-446655440005";
  ar["type"] = 4;
  ar["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ar["length"] = {
    { "mode",  0      },
    { "value", 3840.0 }
  };
  ar["clipStartPosition"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ar["loopStartPosition"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ar["loopEndPosition"] = {
    { "mode",  0      },
    { "value", 3840.0 }
  };
  ar["audioSources"] = nlohmann::json::array ();
  ar["gain"] = 1.5;
  ar["musicalMode"] = 2;
  ar["fadeInOffset"] = {
    { "mode",  0     },
    { "value", 120.0 }
  };
  ar["fadeOutOffset"] = {
    { "mode",  0     },
    { "value", 240.0 }
  };
  ar["fadeInOpts"] = {
    { "curviness", 0.5 },
    { "algo",      1   }
  };
  ar["fadeOutOpts"] = {
    { "curviness", -0.3 },
    { "algo",      2    }
  };

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (ar);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

// ============================================================================
// Invalid JSON Tests
// ============================================================================

TEST (ProjectJsonSerializerValidationTest, InvalidJsonAudioRegionGainOutOfRange)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json ar;
  ar["id"] = "bb0e8400-e29b-41d4-a716-446655440006";
  ar["type"] = 4;
  ar["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ar["length"] = {
    { "mode",  0      },
    { "value", 1920.0 }
  };
  ar["audioSources"] = nlohmann::json::array ();
  ar["gain"] = 3.0;

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (ar);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

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

TEST (ProjectJsonSerializerValidationTest, InvalidJsonMidiNotePitchOutOfRange)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json note;
  note["id"] = "880e8400-e29b-41d4-a716-446655440003";
  note["type"] = 0;
  note["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  note["pitch"] = 128;
  note["velocity"] = 100;

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (note);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (
  ProjectJsonSerializerValidationTest,
  InvalidJsonAutomationPointValueOutOfRange)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json ap;
  ap["id"] = "990e8400-e29b-41d4-a716-446655440004";
  ap["type"] = 7;
  ap["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ap["normalizedValue"] = 1.5;
  ap["curveOptions"] = {
    { "curviness", 0.0 },
    { "algo",      0   }
  };

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (ap);

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

TEST (ProjectJsonSerializerValidationTest, ValidateJson_LongTrackName)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 2;
  track["name"] = std::string (500, 'x');
  j["projectData"]["registries"]["trackRegistry"].push_back (track);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_ManyTracks)
{
  auto j = create_minimal_valid_project_json ();

  for (int i = 0; i < 100; ++i)
    {
      nlohmann::json track;
      track["id"] = fmt::format ("550e8400-e29b-41d4-a716-{:012d}", i);
      track["type"] = 2;
      track["name"] = "Track " + std::to_string (i);
      j["projectData"]["registries"]["trackRegistry"].push_back (track);
    }

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_ManyArrangerObjects)
{
  auto j = create_minimal_valid_project_json ();

  for (int i = 0; i < 1000; ++i)
    {
      nlohmann::json note;
      note["id"] = fmt::format ("880e8400-e29b-41d4-a716-{:012d}", i);
      note["type"] = 0;
      note["position"] = {
        { "mode",  0         },
        { "value", i * 960.0 }
      };
      note["pitch"] = i % 128;
      note["velocity"] = 100;
      j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (note);
    }

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_BoundaryValues)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json note_min;
  note_min["id"] = "880e8400-e29b-41d4-a716-446655440000";
  note_min["type"] = 0;
  note_min["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  note_min["pitch"] = 0;
  note_min["velocity"] = 0;
  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (note_min);

  nlohmann::json note_max;
  note_max["id"] = "880e8400-e29b-41d4-a716-446655440001";
  note_max["type"] = 0;
  note_max["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  note_max["pitch"] = 127;
  note_max["velocity"] = 127;
  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (note_max);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (
  ProjectJsonSerializerValidationTest,
  ValidateJson_AutomationPointBoundaryValues)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json ap_min;
  ap_min["id"] = "990e8400-e29b-41d4-a716-446655440000";
  ap_min["type"] = 7;
  ap_min["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ap_min["normalizedValue"] = 0.0;
  ap_min["curveOptions"] = {
    { "curviness", -1.0 },
    { "algo",      0    }
  };
  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (ap_min);

  nlohmann::json ap_max;
  ap_max["id"] = "990e8400-e29b-41d4-a716-446655440001";
  ap_max["type"] = 7;
  ap_max["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ap_max["normalizedValue"] = 1.0;
  ap_max["curveOptions"] = {
    { "curviness", 1.0 },
    { "algo",      0   }
  };
  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (ap_max);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}
}
