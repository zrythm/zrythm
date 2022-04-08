// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2022 Robert Panovics <robert.panovics@gmail.com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/hash.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/zrythm.h"

static void
test_export (void)
{
  const int max_files = 20;

  char ** midi_files = io_get_files_in_dir_ending_in (
    MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE,
    ".MID", false);
  g_assert_nonnull (midi_files);
  char * export_dir = g_dir_make_tmp (
    "test_midi_export_XXXXXX", NULL);
  char * midi_file;
  int    iter = 0;
  while ((midi_file = midi_files[iter++]))
    {
      g_message ("testing %s", midi_file);

      test_helper_zrythm_init ();

      SupportedFile * file =
        supported_file_new_from_path (midi_file);
      track_create_with_action (
        TRACK_TYPE_MIDI, NULL, file, PLAYHEAD,
        TRACKLIST->num_tracks, 1, NULL);
      supported_file_free (file);

      Track * track = tracklist_get_last_track (
        TRACKLIST, TRACKLIST_PIN_OPTION_BOTH, true);

      g_assert_cmpint (track->num_lanes, >, 0);
      g_assert_cmpint (
        track->lanes[0]->num_regions, >, 0);
      ZRegion * region = track->lanes[0]->regions[0];
      g_assert_true (IS_REGION (region));

      char * basename =
        g_path_get_basename (midi_file);
      char * export_filepath = g_build_filename (
        export_dir, basename, NULL);

      /* export the region again */
      midi_region_export_to_midi_file (
        region, export_filepath, 0, false);
      midi_region_export_to_midi_file (
        region, export_filepath, 0, true);

      g_assert_true (g_file_test (
        export_filepath,
        G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR));

      io_remove (export_filepath);

      g_free (basename);
      g_free (export_filepath);

      test_helper_zrythm_cleanup ();

      if (iter == max_files)
        break;
    }
  g_strfreev (midi_files);

  io_rmdir (export_dir, true);
}

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

static void
compare_files_hash (
  const char * filepath1,
  const char * filepath2)
{
  g_message (
    "Comparing: %s - %s", filepath1, filepath2);

  g_assert_true (g_file_test (
    filepath1,
    G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR));
  g_assert_true (g_file_test (
    filepath2,
    G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR));
  uint32_t file1_hash =
    hash_get_from_file_simple (filepath1);
  uint32_t file2_hash =
    hash_get_from_file_simple (filepath2);

  g_assert_true (file1_hash == file2_hash);
}

static void
setup_region_for_full_export (ZRegion * region)
{
  Position clip_start_pos;
  position_init (&clip_start_pos);
  position_from_ticks (&clip_start_pos, 1920.0);

  position_add_ticks (
    &((ArrangerObject *) region)->pos, 1920.0);
  position_add_ticks (
    &((ArrangerObject *) region)->end_pos, 960.0);
  arranger_object_clip_start_pos_setter (
    (ArrangerObject *) region, &clip_start_pos);
}

static void
test_full_export (void)
{
  const int number_of_loop_tests =
    sizeof (full_export_test_loop_positions)
    / sizeof (double) / 2;

  char * base_midi_file = g_build_filename (
    TESTS_SRCDIR, "loopbase.mid", NULL);

  g_assert_true (g_file_test (
    base_midi_file,
    G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR));

  char * export_dir = g_dir_make_tmp (
    "test_midi_full_export_XXXXXX", NULL);

  test_helper_zrythm_init ();

  SupportedFile * file =
    supported_file_new_from_path (base_midi_file);
  track_create_with_action (
    TRACK_TYPE_MIDI, NULL, file, PLAYHEAD,
    TRACKLIST->num_tracks, 1, NULL);
  supported_file_free (file);

  Track * track = tracklist_get_last_track (
    TRACKLIST, TRACKLIST_PIN_OPTION_BOTH, true);

  g_assert_cmpint (track->num_lanes, >, 0);
  g_assert_cmpint (
    track->lanes[0]->num_regions, >, 0);
  ZRegion * region = track->lanes[0]->regions[0];
  g_assert_true (IS_REGION (region));

  setup_region_for_full_export (region);

  for (int iter = 0; iter < number_of_loop_tests;
       ++iter)
    {
      POSITION_INIT_ON_STACK (loop_start_pos);
      POSITION_INIT_ON_STACK (loop_end_pos);
      position_from_ticks (
        &loop_start_pos,
        full_export_test_loop_positions[iter * 2]);
      position_from_ticks (
        &loop_end_pos,
        full_export_test_loop_positions[iter * 2 + 1]);

      g_message (
        "testing loop %lf, %lf",
        loop_start_pos.ticks, loop_end_pos.ticks);

      arranger_object_loop_start_pos_setter (
        (ArrangerObject *) region, &loop_start_pos);
      arranger_object_loop_end_pos_setter (
        (ArrangerObject *) region, &loop_end_pos);

      char export_file_name[20];
      sprintf (
        export_file_name, "loopbase%d.mid", iter);
      char * export_filepath = g_build_filename (
        export_dir, export_file_name, NULL);

      midi_region_export_to_midi_file (
        region, export_filepath, 0, false);

      compare_files_hash (
        base_midi_file, export_filepath);

      io_remove (export_filepath);
      g_free (export_filepath);

      sprintf (export_file_name, "loop%d.mid", iter);
      export_filepath = g_build_filename (
        export_dir, export_file_name, NULL);

      char reference_file_name[20];
      sprintf (
        reference_file_name, "loop%d_ref.mid", iter);
      char * reference_filepath = g_build_filename (
        TESTS_SRCDIR, reference_file_name, NULL);

      midi_region_export_to_midi_file (
        region, export_filepath, 0, true);

      compare_files_hash (
        reference_filepath, export_filepath);
      io_remove (export_filepath);

      g_free (reference_filepath);
      g_free (export_filepath);
    }

  test_helper_zrythm_cleanup ();
  io_rmdir (export_dir, true);
  g_free (base_midi_file);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/midi_region/"

  g_test_add_func (
    TEST_PREFIX "test full export",
    (GTestFunc) test_full_export);
  g_test_add_func (
    TEST_PREFIX "test export",
    (GTestFunc) test_export);

  return g_test_run ();
}
