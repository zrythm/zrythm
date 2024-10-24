// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL

#include "common/utils/gtest_wrapper.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "tests/helpers/exporter.h"

std::string
test_exporter_export_audio (Exporter::TimeRange time_range, Exporter::Mode mode)
{
  EXPECT_FALSE (TRANSPORT->isRolling ());
  EXPECT_EQ (TRANSPORT->playhead_pos_.frames_, 0);

  constexpr auto filename = "test_export.wav";

  Exporter::Settings settings;
  settings.format_ = Exporter::Format::WAV;
  settings.artist_ = "Test Artist";
  settings.title_ = "Test Title";
  settings.genre_ = "Test Genre";
  settings.depth_ = BitDepth::BIT_DEPTH_16;
  settings.time_range_ = time_range;
  if (mode == Exporter::Mode::Full)
    {
      settings.mode_ = Exporter::Mode::Full;
      TRACKLIST->mark_all_tracks_for_bounce (false);
      settings.bounce_with_parents_ = false;
    }
  else
    {
      settings.mode_ = Exporter::Mode::Tracks;
      TRACKLIST->mark_all_tracks_for_bounce (true);
      settings.bounce_with_parents_ = true;
    }
  auto exports_dir = PROJECT->get_path (ProjectPath::EXPORTS, false);
  settings.file_uri_ = Glib::build_filename (exports_dir, filename);
  Exporter exporter (settings);
  exporter.prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

  /* start exporting in a new thread */
  exporter.begin_generic_thread ();
  exporter.join_generic_thread ();
  exporter.post_export ();

  EXPECT_FALSE (AUDIO_ENGINE->exporting_);

  return settings.file_uri_;
}
