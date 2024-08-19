// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/scale.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/scale");

TEST_CASE ("scale contains note")
{
  MusicalScale scale (MusicalScale::Type::Minor, MusicalNote::D);

  REQUIRE (scale.contains_note (MusicalNote::C));
  REQUIRE_FALSE (scale.contains_note (MusicalNote::CSharp));
  REQUIRE (scale.contains_note (MusicalNote::D));
  REQUIRE_FALSE (scale.contains_note (MusicalNote::DSharp));
  REQUIRE (scale.contains_note (MusicalNote::E));
  REQUIRE (scale.contains_note (MusicalNote::F));
  REQUIRE_FALSE (scale.contains_note (MusicalNote::FSharp));
  REQUIRE (scale.contains_note (MusicalNote::G));
  REQUIRE_FALSE (scale.contains_note (MusicalNote::GSharp));
  REQUIRE (scale.contains_note (MusicalNote::A));
  REQUIRE (scale.contains_note (MusicalNote::ASharp));
  REQUIRE_FALSE (scale.contains_note (MusicalNote::B));
}

TEST_SUITE_END;