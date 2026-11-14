// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"
#include "utils/object_registry.h"

#include "operations/operation_environment.h"
#include "operations/param_types.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::operations
{

/**
 * A minimal but real environment, enough for parameter types that do not
 * resolve references.
 */
class ParamTypesTest : public ::testing::Test
{
protected:
  dsp::TempoMap         tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper  tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry registry_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };
  structure::tracks::SingletonTracks        singleton_tracks_;
  structure::tracks::TrackCollection        track_collection_{ registry_ };
  structure::tracks::TrackRouting           track_routing_{ registry_ };
  structure::tracks::FinalTrackDependencies factory_deps_{
    tempo_map_wrapper_,
    registry_,
    soloed_tracks_exist_getter_,
    {}
  };
  structure::tracks::TrackFactory track_factory_{
    [this] () -> structure::tracks::FinalTrackDependencies {
      return factory_deps_;
    }
  };
  undo::UndoStack      undo_stack_{ [] (auto callback, bool) { callback (); } };
  OperationEnvironment environment_{
    registry_,      undo_stack_,    track_collection_,
    track_factory_, track_routing_, singleton_tracks_
  };
};

TEST_F (ParamTypesTest, StringParamAcceptsStringWithinBounds)
{
  const auto parsed =
    StringParam<1, 128>::parse (nlohmann::json ("Intro"), environment_);
  ASSERT_TRUE (parsed.has_value ());
  EXPECT_EQ (parsed->value ().view (), "Intro");
}

TEST_F (ParamTypesTest, StringParamRefusesEmptyAndOversize)
{
  EXPECT_FALSE ((
    StringParam<1, 128>::parse (nlohmann::json (""), environment_).has_value ()));
  EXPECT_FALSE ((
    StringParam<1, 4>::parse (nlohmann::json ("too long"), environment_)
      .has_value ()));
  EXPECT_FALSE (
    (StringParam<1, 128>::parse (nlohmann::json{}, environment_).has_value ()))
    << "null (absent parameter) must be refused";
  EXPECT_FALSE ((
    StringParam<1, 128>::parse (nlohmann::json (5), environment_).has_value ()));
}

TEST_F (ParamTypesTest, StringParamRefusesInvalidUtf8)
{
  const auto parsed = StringParam<1, 128>::parse (
    nlohmann::json (std::string ("Bad\xC3\x28 name")), environment_);
  ASSERT_FALSE (parsed.has_value ());
  EXPECT_NE (parsed.error ().reason.find ("UTF-8"), std::string::npos);
}

TEST_F (ParamTypesTest, FromValueValidatesLikeParse)
{
  const auto ok =
    StringParam<1, 128>::from_value (utils::Utf8String (u8"Intro"));
  ASSERT_TRUE (ok.has_value ());
  EXPECT_EQ (ok->value ().view (), "Intro");
  EXPECT_FALSE (
    (StringParam<1, 128>::from_value (utils::Utf8String (u8"")).has_value ()));

  EXPECT_TRUE ((LimitParam::from_value (10).has_value ()));
  EXPECT_FALSE ((LimitParam::from_value (LimitParam::maximum + 1).has_value ()));

  using structure::tracks::Track;
  EXPECT_TRUE (
    (EnumParam<Track::Type>::from_value (Track::Type::Audio).has_value ()));
  EXPECT_FALSE ((
    EnumParam<Track::Type>::from_value (static_cast<Track::Type> (999))
      .has_value ()));

  EXPECT_FALSE ((TrackParam::from_value (nullptr).has_value ()));
}

TEST_F (ParamTypesTest, FromValueTakesATrackAndDerivesItsUuid)
{
  auto ref = track_factory_.create_empty_track<structure::tracks::AudioTrack> ();
  const auto parsed = TrackParam::from_value (ref.get ());
  ASSERT_TRUE (parsed.has_value ());
  EXPECT_EQ (parsed->value (), ref.get ());
  EXPECT_FALSE (parsed->uuid_string ().empty ());
}

TEST_F (ParamTypesTest, HexColorParamAcceptsSixAndEightDigitHex)
{
  ASSERT_TRUE ((
    HexColorParam::parse (nlohmann::json ("#ff5588"), environment_)
      .has_value ()));
  ASSERT_TRUE ((
    HexColorParam::parse (nlohmann::json ("#ff5588cc"), environment_)
      .has_value ()));
}

TEST_F (ParamTypesTest, HexColorParamRefusesNonHexStrings)
{
  for (const char * bad : { "red", "#ff", "#gg5588", "ff5588", "#ff558" })
    {
      EXPECT_FALSE ((
        HexColorParam::parse (nlohmann::json (bad), environment_).has_value ()))
        << bad;
    }
}

TEST_F (ParamTypesTest, LimitParamDefaultsWhenAbsentAndValidatesRange)
{
  const auto absent = LimitParam::parse (nlohmann::json{}, environment_);
  ASSERT_TRUE (absent.has_value ());
  EXPECT_EQ (absent->value (), LimitParam::default_value);

  const auto present = LimitParam::parse (nlohmann::json (10), environment_);
  ASSERT_TRUE (present.has_value ());
  EXPECT_EQ (present->value (), 10);

  EXPECT_FALSE (
    (LimitParam::parse (nlohmann::json (0), environment_).has_value ()));
  EXPECT_FALSE ((
    LimitParam::parse (nlohmann::json (LimitParam::maximum + 1), environment_)
      .has_value ()));
}

TEST_F (ParamTypesTest, EnumParamAcceptsKnownNamesAndRefusesUnknown)
{
  using structure::tracks::Track;

  const auto audio =
    EnumParam<Track::Type>::parse (nlohmann::json ("Audio"), environment_);
  ASSERT_TRUE (audio.has_value ());
  EXPECT_EQ (audio->value (), Track::Type::Audio);

  const auto lowercase =
    EnumParam<Track::Type>::parse (nlohmann::json ("audio"), environment_);
  ASSERT_FALSE (lowercase.has_value ());
  const auto reason = lowercase.error ().reason;
  EXPECT_NE (reason.find ("'audio'"), std::string::npos);
  EXPECT_NE (reason.find ("Audio"), std::string::npos);
}

TEST_F (ParamTypesTest, EnumParamSchemaListsAllValues)
{
  using structure::tracks::Track;
  const auto schema = EnumParam<Track::Type>::describe ();
  ASSERT_TRUE (schema.contains ("oneOf"));
  const auto &branches = schema.at ("oneOf");
  ASSERT_EQ (branches.size (), 2);

  const auto &by_name = branches.at (0);
  EXPECT_EQ (by_name.at ("type").get<std::string> (), "string");
  const auto &names = by_name.at ("enum");
  EXPECT_EQ (names.size (), magic_enum::enum_count<Track::Type> ());
  bool found_audio = false;
  for (const auto &name : names)
    {
      found_audio |= name.get<std::string> () == "Audio";
    }
  EXPECT_TRUE (found_audio);

  const auto &by_value = branches.at (1);
  EXPECT_EQ (by_value.at ("type").get<std::string> (), "integer");
  const auto &values = by_value.at ("enum");
  EXPECT_EQ (values.size (), magic_enum::enum_count<Track::Type> ());
  bool found_audio_value = false;
  for (const auto &value : values)
    {
      found_audio_value |=
        value.get<int> () == ENUM_VALUE_TO_INT (Track::Type::Audio);
    }
  EXPECT_TRUE (found_audio_value);
}

TEST_F (ParamTypesTest, TrackParamRefusesNonUuidInput)
{
  const auto parsed =
    TrackParam::parse (nlohmann::json ("not-a-uuid"), environment_);
  ASSERT_FALSE (parsed.has_value ());
  EXPECT_NE (parsed.error ().reason.find ("not a UUID"), std::string::npos);
}

TEST_F (ParamTypesTest, TrackParamRefusesUuidNotInCollection)
{
  const auto parsed = TrackParam::parse (
    nlohmann::json ("123e4567-e89b-12d3-a456-426614174000"), environment_);
  ASSERT_FALSE (parsed.has_value ());
  EXPECT_NE (
    parsed.error ().reason.find ("no track with UUID"), std::string::npos);
}

} // namespace zrythm::operations
