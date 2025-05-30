// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <random>

#include "dsp/midi_event.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/engine_dummy.h"
#include "gui/dsp/midi_track.h"
#include "gui/dsp/track.h"
#include "utils/io.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

constexpr auto BUFFER_SIZE = 20;
/* around 100 causes OOM in various CIs */
constexpr auto MAX_FILES = 10;

TEST_F (ZrythmFixture, MidiFilePlayback)
{
  /* create a track for testing */
  Track::create_empty_with_action<MidiTrack> ();

  MidiEventVector events;

  auto midi_files =
    io_get_files_in_dir_ending_in (
      MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE, ".MID")
      .toStdStringVector ();
  ASSERT_NONEMPTY (midi_files);

  /* shuffle array */
  auto rd = std::random_device{};
  auto rng = std::default_random_engine{ rd () };
  std::shuffle (midi_files.begin (), midi_files.end (), rng);

  int      iter = 0;
  Position init_pos;
  init_pos.set_to_bar (1);
  for (auto midi_file : midi_files)
    {
      /*if (!g_str_has_suffix (midi_file, "M23.MID"))*/
      /*continue;*/
      z_info ("testing {}", midi_file);

      FileDescriptor file = FileDescriptor (midi_file);
      Track::create_with_action (
        Track::Type::Midi, nullptr, &file, &PLAYHEAD,
        TRACKLIST->get_num_tracks (), 1, -1, nullptr);
      z_info ("testing {}", midi_file);

      auto track = TRACKLIST->get_last_track<MidiTrack> ();

      auto region = track->lanes_[0]->regions_[0];

      /* set start pos to a bit before the first note, send end pos a few cycles
       * later */
      Position start_pos, stop_pos;
      start_pos = region->midi_notes_[0]->pos_;
      stop_pos = start_pos;
      start_pos.add_frames (-BUFFER_SIZE * 2);
      /* run 80 cycles */
      stop_pos.add_frames (BUFFER_SIZE * 80);

      /* set region end somewhere within the covered positions to check for
       * warnings */
      Position tmp;
      tmp = start_pos;
      tmp.add_frames (BUFFER_SIZE * 32 + BUFFER_SIZE / 3);
      region->end_position_setter_validated (&tmp);

      /* start filling events to see if any warnings occur */
      for (
        signed_frame_t i = start_pos.frames_; i < stop_pos.frames_;
        i += BUFFER_SIZE)
        {
          /* try all possible offsets */
          for (int j = 0; j < BUFFER_SIZE; j++)
            {
              EngineProcessTimeInfo time_nfo = {
                .g_start_frame_ = (unsigned_frame_t) i,
                .g_start_frame_w_offset_ =
                  (unsigned_frame_t) i + (unsigned_frame_t) j,
                .local_offset_ = (nframes_t) j,
                .nframes_ = BUFFER_SIZE,
              };
              track->fill_midi_events (time_nfo, events);
              events.clear ();
            }
        }

      /* sleep to avoid being killed */
      std::this_thread::sleep_for (std::chrono::milliseconds (1000));

      if (iter++ == MAX_FILES)
        break;
    }

  test_project_save_and_reload ();
}
