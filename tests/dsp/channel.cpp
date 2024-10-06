// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/dsp/track.h"
#include "common/utils/flags.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/zrythm.h"

#include "tests/helpers/exporter.h"
#include "tests/helpers/plugin_manager.h"

TEST_F (ZrythmFixture, MidiFxRouting)
{
  /* create an instrument */
  auto setting = test_plugin_manager_get_plugin_setting (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true);
  auto track = Track::create_for_plugin_at_idx_w_action<InstrumentTrack> (
    &setting, TRACKLIST->get_num_tracks ());
  ASSERT_NONNULL (track);

  int num_dests = PORT_CONNECTIONS_MGR->get_sources_or_dests (
    nullptr, track->processor_->midi_out_->id_, false);
  ASSERT_EQ (num_dests, 1);

  /* add MIDI file */
  auto midi_files = io_get_files_in_dir_ending_in (
    MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE, ".MID");
  ASSERT_GT (midi_files.size (), 0);
  FileDescriptor file (midi_files[0]);
  ASSERT_NO_THROW (TRACKLIST->import_files (
    nullptr, &file, track, track->lanes_[0].get (), -1, &PLAYHEAD, nullptr));

  auto export_loop_and_check_for_silence = [&] (bool expect_silence) {
    auto audio_file = test_exporter_export_audio (
      Exporter::TimeRange::Loop, Exporter::Mode::Full);
    ASSERT_NONEMPTY (audio_file);
    ASSERT_EQ (expect_silence, audio_file_is_silent (audio_file.c_str ()));
    io_remove (audio_file);
  };

  /* export loop and check that there is audio */
  export_loop_and_check_for_silence (false);

  /* create MIDI eat plugin and add to MIDI FX */
  setting = test_plugin_manager_get_plugin_setting (
    PLUMBING_BUNDLE_URI, "http://gareus.org/oss/lv2/plumbing#eat1", true);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::MidiFx, *track, 0, setting, 1));

  num_dests = PORT_CONNECTIONS_MGR->get_sources_or_dests (
    nullptr, track->processor_->midi_out_->id_, false);
  ASSERT_EQ (num_dests, 1);

  /* export loop and check that there is no audio */
  export_loop_and_check_for_silence (true);

  /* bypass MIDI FX and check that there is audio */
  auto &midieat = track->channel_->midi_fx_.at (0);
  midieat->set_enabled (false, false);
  ASSERT_FALSE (midieat->is_enabled (false));

  /* export loop and check that there is audio again */
  export_loop_and_check_for_silence (false);
}
