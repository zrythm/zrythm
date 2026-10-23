// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track.h"

#include "unit/structure/tracks/mock_track.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::tracks
{

class TrackColorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    track = mock_track_factory.createMockTrack (Track::Type::Audio);
  }

  MockTrackFactory           mock_track_factory;
  std::unique_ptr<MockTrack> track;
};

// A track without a user-set color reports an invalid color; the type
// default lives in the QML layer (TrackColors)
TEST_F (TrackColorTest, ColorIsUnsetUntilSet)
{
  EXPECT_FALSE (track->useColor ());
  EXPECT_FALSE (track->color ().isValid ());
  EXPECT_EQ (track->stored_color (), std::nullopt);

  track->setColor (QColor ("#00FF00"));
  EXPECT_TRUE (track->useColor ());
  EXPECT_EQ (track->color (), QColor ("#00FF00"));
  EXPECT_NE (track->stored_color (), std::nullopt);

  track->unsetColor ();
  EXPECT_FALSE (track->useColor ());
  EXPECT_FALSE (track->color ().isValid ());
  EXPECT_EQ (track->stored_color (), std::nullopt);
}

// Invalid colors are refused, leaving the stored state untouched
TEST_F (TrackColorTest, InvalidColorIsRefused)
{
  track->setColor (QColor ("#00FF00"));

  track->setColor (QColor ());
  EXPECT_TRUE (track->useColor ());
  EXPECT_EQ (track->color (), QColor ("#00FF00"));
}

// Unset colors are omitted from the JSON and JSON without the key
// loads as unset; set colors serialize as a hex string
TEST_F (TrackColorTest, SerializationOmitsUnsetColor)
{
  nlohmann::json j;
  to_json (j, *track);
  EXPECT_FALSE (j.contains ("color"));

  track->setColor (QColor ("#123456"));
  to_json (j, *track);
  ASSERT_TRUE (j.contains ("color"));
  EXPECT_EQ (j.at ("color").get<std::string> (), "#123456");

  j.erase ("color");
  auto loaded = mock_track_factory.createMockTrack (Track::Type::Audio);
  loaded->setColor (QColor ("#FFFFFF"));
  from_json (j, *loaded);
  EXPECT_FALSE (loaded->useColor ());
  EXPECT_EQ (loaded->stored_color (), std::nullopt);
}

} // namespace zrythm::structure::tracks
