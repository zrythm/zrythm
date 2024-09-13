// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2022 Robert Panovics <robert.panovics@gmail.com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "dsp/midi_region.h"
#include "dsp/region.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/hash.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/zrythm_helper.h"

class MidiRegionExportTest
    : public ZrythmFixture,
      public ::testing::WithParamInterface<std::string>
{
protected:
  void SetUp () override
  {
    ZrythmFixture::SetUp ();
    midi_file_ = GetParam ();
  }

  std::string midi_file_;
};

TEST_P (MidiRegionExportTest, Export)
{
  z_info ("testing {}", midi_file_);

  FileDescriptor file (midi_file_);
  Track::create_with_action (
    Track::Type::Midi, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);

  auto track = TRACKLIST->get_last_track<MidiTrack> ();

  ASSERT_NONEMPTY (track->lanes_);
  ASSERT_NONEMPTY (track->lanes_[0]->regions_);
  const auto &region = track->lanes_[0]->regions_[0];

  char * export_dir = g_dir_make_tmp ("test_midi_export_XXXXXX", nullptr);
  auto   basename = Glib::path_get_basename (midi_file_);
  auto   export_filepath = Glib::build_filename (export_dir, basename);

  /* export the region again */
  region->export_to_midi_file (export_filepath, 0, false);
  region->export_to_midi_file (export_filepath, 0, true);

  ASSERT_TRUE (Glib::file_test (
    export_filepath, Glib::FileTest::EXISTS | Glib::FileTest::IS_REGULAR));

  io_remove (export_filepath);

  ASSERT_FALSE (Glib::file_test (export_filepath, Glib::FileTest::EXISTS));

  io_rmdir (export_dir, true);
}

static std::vector<std::string>
LimitedMidiFiles ()
{
  constexpr auto max_files = 20;
  auto           midi_files = io_get_files_in_dir_ending_in (
    MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE, ".MID");
  auto ret = midi_files.toStdStringVector ();
  ret.resize (max_files);
  return ret;
}

INSTANTIATE_TEST_SUITE_P (
  MidiRegionExport,
  MidiRegionExportTest,
  ::testing::ValuesIn (LimitedMidiFiles ()));

static const double full_export_test_loop_positions[] = {
  /* loop before clip start pos */
  0.0, 1920.0,
  /* clip start pos inside loop */
  0.0, 8640.0,
  /* clip start pos inside loop with cuting end of note */
  0.0, 5760.0,
  /* loop after clip start pos with note start before loop start */
  3840.0, 5760.0,
  /* loop after clip start pos with cuting end of note */
  3840.0, 7680.0
};

TEST_F (ZrythmFixture, FullExport)
{
  const int number_of_loop_tests =
    sizeof (full_export_test_loop_positions) / sizeof (double) / 2;

  auto base_midi_file = Glib::build_filename (TESTS_SRCDIR, "loopbase.mid");

  ASSERT_TRUE (Glib::file_test (
    base_midi_file, Glib::FileTest::EXISTS | Glib::FileTest::IS_REGULAR));

  char * export_dir = g_dir_make_tmp ("test_midi_full_export_XXXXXX", nullptr);

  FileDescriptor file (base_midi_file);
  Track::create_with_action (
    Track::Type::Midi, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);

  auto track = TRACKLIST->get_last_track<MidiTrack> ();

  ASSERT_NONEMPTY (track->lanes_);
  ASSERT_NONEMPTY (track->lanes_[0]->regions_);
  auto region = track->lanes_[0]->regions_[0];

  auto setup_region_for_full_export = [] (auto &r) {
    Position clip_start_pos{ 1920.0 };

    r->pos_.add_ticks (1920.0);
    r->end_pos_.add_ticks (960.0);
    r->clip_start_pos_setter (&clip_start_pos);
  };

  setup_region_for_full_export (region);

  auto compare_files_hash = [] (const auto &filepath1, const auto &filepath2) {
    z_info ("Comparing: {} - {}", filepath1, filepath2);

    ASSERT_TRUE (Glib::file_test (
      filepath1, Glib::FileTest::EXISTS | Glib::FileTest::IS_REGULAR));
    ASSERT_TRUE (Glib::file_test (
      filepath2, Glib::FileTest::EXISTS | Glib::FileTest::IS_REGULAR));
    uint32_t file1_hash = hash_get_from_file_simple (filepath1.c_str ());
    uint32_t file2_hash = hash_get_from_file_simple (filepath2.c_str ());

    ASSERT_EQ (file1_hash, file2_hash);
  };

  for (int iter = 0; iter < number_of_loop_tests; ++iter)
    {
      Position loop_start_pos;
      Position loop_end_pos;
      loop_start_pos.from_ticks (full_export_test_loop_positions[iter * 2]);
      loop_end_pos.from_ticks (full_export_test_loop_positions[iter * 2 + 1]);

      region->loop_start_pos_setter (&loop_start_pos);
      region->loop_end_pos_setter (&loop_end_pos);

      auto export_file_name = fmt::format ("loopbase{}.mid", iter);
      auto export_filepath = Glib::build_filename (export_dir, export_file_name);

      region->export_to_midi_file (export_filepath, 0, false);

      compare_files_hash (base_midi_file, export_filepath);

      io_remove (export_filepath);

      export_file_name = fmt::format ("loop{}.mid", iter);
      export_filepath = Glib::build_filename (export_dir, export_file_name);

      auto reference_file_name = fmt::format ("loop{}_ref.mid", iter);
      auto reference_filepath =
        Glib::build_filename (TESTS_SRCDIR, reference_file_name);

      region->export_to_midi_file (export_filepath, 0, true);

      compare_files_hash (reference_filepath, export_filepath);
      io_remove (export_filepath);
    }
  io_rmdir (export_dir, true);
}
