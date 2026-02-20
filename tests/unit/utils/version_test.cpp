// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/version.h"

#include <gtest/gtest.h>

using namespace zrythm::utils;

TEST (VersionTest, Equality)
{
  EXPECT_EQ ((Version{ 1, 0, {} }), (Version{ 1, 0, {} }));
  EXPECT_EQ ((Version{ 1, 0, 0 }), (Version{ 1, 0, 0 }));
  EXPECT_EQ ((Version{ 2, 5, 3 }), (Version{ 2, 5, 3 }));
  EXPECT_NE ((Version{ 1, 0, {} }), (Version{ 1, 1, {} }));
  EXPECT_NE ((Version{ 1, 0, 0 }), (Version{ 1, 0, 1 }));
  EXPECT_NE ((Version{ 1, 0, {} }), (Version{ 2, 0, {} }));
}

TEST (VersionTest, Comparison)
{
  // Major version comparison
  EXPECT_LT ((Version{ 1, 0, {} }), (Version{ 2, 0, {} }));
  EXPECT_GT ((Version{ 2, 0, {} }), (Version{ 1, 0, {} }));

  // Minor version comparison
  EXPECT_LT ((Version{ 1, 0, {} }), (Version{ 1, 1, {} }));
  EXPECT_GT ((Version{ 1, 1, {} }), (Version{ 1, 0, {} }));

  // Patch version comparison
  EXPECT_LT ((Version{ 1, 0, 0 }), (Version{ 1, 0, 1 }));
  EXPECT_GT ((Version{ 1, 0, 1 }), (Version{ 1, 0, 0 }));

  // Missing patch treated as 0
  EXPECT_EQ ((Version{ 1, 0, {} }), (Version{ 1, 0, 0 }));
  EXPECT_LE ((Version{ 1, 0, {} }), (Version{ 1, 0, 0 }));
  EXPECT_GE ((Version{ 1, 0, {} }), (Version{ 1, 0, 0 }));

  // Complex comparisons
  EXPECT_LT ((Version{ 1, 2, 3 }), (Version{ 2, 0, 0 }));
  EXPECT_LT ((Version{ 1, 2, 3 }), (Version{ 1, 3, 0 }));
  EXPECT_LT ((Version{ 1, 2, 3 }), (Version{ 1, 2, 4 }));
}

TEST (VersionTest, JsonSerialization)
{
  // Version without patch
  {
    Version        v{ 2, 1, {} };
    nlohmann::json j = v;

    EXPECT_TRUE (j.is_object ());
    EXPECT_EQ (j["major"].get<int> (), 2);
    EXPECT_EQ (j["minor"].get<int> (), 1);
    EXPECT_FALSE (j.contains ("patch"));
  }

  // Version with patch
  {
    Version        v{ 2, 1, 5 };
    nlohmann::json j = v;

    EXPECT_TRUE (j.is_object ());
    EXPECT_EQ (j["major"].get<int> (), 2);
    EXPECT_EQ (j["minor"].get<int> (), 1);
    EXPECT_TRUE (j.contains ("patch"));
    EXPECT_EQ (j["patch"].get<int> (), 5);
  }
}

TEST (VersionTest, JsonDeserialization)
{
  // Version without patch
  {
    nlohmann::json j = {
      { "major", 2 },
      { "minor", 1 }
    };
    Version v = j.get<Version> ();

    EXPECT_EQ (v.major, 2);
    EXPECT_EQ (v.minor, 1);
    EXPECT_FALSE (v.patch.has_value ());
  }

  // Version with patch
  {
    nlohmann::json j = {
      { "major", 2 },
      { "minor", 1 },
      { "patch", 5 }
    };
    Version v = j.get<Version> ();

    EXPECT_EQ (v.major, 2);
    EXPECT_EQ (v.minor, 1);
    EXPECT_TRUE (v.patch.has_value ());
    EXPECT_EQ (*v.patch, 5);
  }
}

TEST (VersionTest, JsonRoundTrip)
{
  // Without patch
  {
    Version        original{ 3, 7, {} };
    nlohmann::json j = original;
    Version        restored = j.get<Version> ();

    EXPECT_EQ ((original), (restored));
  }

  // With patch
  {
    Version        original{ 3, 7, 2 };
    nlohmann::json j = original;
    Version        restored = j.get<Version> ();

    EXPECT_EQ ((original), (restored));
  }
}

TEST (VersionTest, ZeroVersion)
{
  Version v{ 0, 0, {} };
  EXPECT_EQ (v.major, 0);
  EXPECT_EQ (v.minor, 0);
  EXPECT_FALSE (v.patch.has_value ());

  nlohmann::json j = v;
  EXPECT_EQ (j["major"].get<int> (), 0);
  EXPECT_EQ (j["minor"].get<int> (), 0);
}

TEST (VersionTest, LargeVersionNumbers)
{
  Version v{ 999, 999, 999 };
  EXPECT_EQ (v.major, 999);
  EXPECT_EQ (v.minor, 999);
  EXPECT_EQ (*v.patch, 999);

  nlohmann::json j = v;
  Version        restored = j.get<Version> ();
  EXPECT_EQ ((v), (restored));
}
