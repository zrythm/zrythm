// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/dsp/exporter.h"
#include "common/io/file_descriptor.h"
#include "common/utils/chromaprint.h"
#include "common/utils/progress_info.h"
#include "gui/backend/backend/actions/arranger_selections.h"
#include "gui/backend/backend/actions/channel_send_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "helpers/plugin_manager.h"
#include "tests/helpers/zrythm_helper.h"

#include <sndfile.h>

constexpr midi_byte_t DUMMY_NOTE_PITCH = 70;
constexpr midi_byte_t DUMMY_NOTE_VELOCITY = 70;

static void
print_progress_and_sleep (ProgressInfo &info)
{
  while (info.get_status () != ProgressInfo::Status::COMPLETED)
    {
      auto [progress, progress_str] = info.get_progress ();
      z_info ("progress: {:.1f}", progress * 100.0);
      constexpr auto sleep_time = 10'000;
      std::this_thread::sleep_for (std::chrono::milliseconds (sleep_time));
    }
}

TEST_F (ZrythmFixture, BounceWithNoteAtStart)
{
  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false, 1);
  auto ins_track = TRACKLIST->get_last_track<InstrumentTrack> ();
  ins_track->select (true, true, false);

  /* add region with note */
  Position start, end;
  start.zero ();
  end.set_to_bar (4);
  auto r =
    std::make_shared<MidiRegion> (start, end, ins_track->name_hash_, 0, 0);
  ins_track->add_region (r, nullptr, 0, true, false);
  start.zero ();
  end.set_to_bar (4);
  auto mn = std::make_shared<MidiNote> (
    r->id_, start, end, DUMMY_NOTE_PITCH, DUMMY_NOTE_VELOCITY);
  r->append_object (mn);
  r->select (true, false, false);

  /* bounce the loop */
  Exporter::Settings settings;
  settings.mode_ = Exporter::Mode::Regions;
  settings.set_bounce_defaults (Exporter::Format::WAV, "", r->name_);
  settings.time_range_ = Exporter::TimeRange::Loop;
  TL_SELECTIONS->mark_for_bounce (settings.bounce_with_parents_);

  Exporter exporter (settings);
  exporter.prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

  /* start exporting in a new thread */
  exporter.begin_generic_thread ();
  print_progress_and_sleep (*exporter.progress_info_);
  exporter.join_generic_thread ();
  exporter.post_export ();

  ASSERT_FALSE (audio_file_is_silent (exporter.get_exported_path ().c_str ()));
}

TEST_F (ZrythmFixture, MixdownMidi)
{
  /* create a MIDI track with 2 adjacent regions
   * with 1 MIDI note each */
  auto track = Track::create_empty_with_action<MidiTrack> ();
  track->select (true, true, false);

  Position start, end;
  start.zero ();
  end.set_to_bar (4);
  auto r = std::make_shared<MidiRegion> (start, end, track->name_hash_, 0, 0);
  track->add_region (r, nullptr, 0, true, false);

  /* midi note 1 */
  start.zero ();
  end.set_to_bar (2);
  auto mn = std::make_shared<MidiNote> (
    r->id_, start, end, DUMMY_NOTE_PITCH, DUMMY_NOTE_VELOCITY);
  r->append_object (mn);

  /* midi note 2 */
  start.set_to_bar (3);
  end.set_to_bar (4);
  mn = std::make_shared<MidiNote> (
    r->id_, start, end, DUMMY_NOTE_PITCH, DUMMY_NOTE_VELOCITY);
  r->append_object (mn);

  /* region 2 */
  start.set_to_bar (2);
  end.set_to_bar (3);
  r = std::make_shared<MidiRegion> (start, end, track->name_hash_, 0, 1);
  track->add_region (r, nullptr, 0, true, false);

  /* midi note 3 */
  start.zero ();
  end.set_to_bar (2);
  mn = std::make_shared<MidiNote> (
    r->id_, start, end, DUMMY_NOTE_PITCH, DUMMY_NOTE_VELOCITY);
  r->append_object (mn);

  /* bounce */
  Exporter::Settings settings;
  settings.mode_ = Exporter::Mode::Full;
  settings.set_bounce_defaults (Exporter::Format::Midi1, "", __func__);
  settings.time_range_ = Exporter::TimeRange::Loop;

  Exporter exporter (settings);
  exporter.prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

  /* start exporting in a new thread */
  exporter.begin_generic_thread ();
  print_progress_and_sleep (*exporter.progress_info_);
  exporter.join_generic_thread ();
  exporter.post_export ();

  /* create a MIDI track from the MIDI file */
  FileDescriptor file (exporter.get_exported_path ());
  Track::create_with_action (
    Track::Type::Midi, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);
  auto exported_track = TRACKLIST->get_last_track<MidiTrack> ();

  /* verify correct data */
  ASSERT_SIZE_EQ (exported_track->lanes_, 2);
  ASSERT_SIZE_EQ (exported_track->lanes_[0]->regions_, 1);
  ASSERT_EMPTY (exported_track->lanes_[1]->regions_);
  r = exported_track->lanes_[0]->regions_[0];
  ASSERT_SIZE_EQ (r->midi_notes_, 3);
  mn = r->midi_notes_[0];
  start.set_to_bar (1);
  end.set_to_bar (2);
  ASSERT_POSITION_EQ (mn->pos_, start);
  ASSERT_POSITION_EQ (mn->end_pos_, end);
  mn = r->midi_notes_[1];
  start.set_to_bar (2);
  end.set_to_bar (3);
  ASSERT_POSITION_EQ (mn->pos_, start);
  ASSERT_POSITION_EQ (mn->end_pos_, end);
  mn = r->midi_notes_[2];
  start.set_to_bar (3);
  end.set_to_bar (4);
  ASSERT_POSITION_EQ (mn->pos_, start);
  ASSERT_POSITION_EQ (mn->end_pos_, end);
}

TEST_F (ZrythmFixture, ExportWav)
{
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test.wav");
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);

  char * tmp_dir = g_dir_make_tmp ("test_wav_prj_XXXXXX", nullptr);
  PROJECT->save (tmp_dir, 0, 0, false);
  g_free (tmp_dir);

  for (int i = 0; i < 2; i++)
    {
      for (int j = 0; j < 2; j++)
        {
          ASSERT_FALSE (TRANSPORT->isRolling ());
          ASSERT_EQ (TRANSPORT->playhead_pos_.frames_, 0);

          auto filename = fmt::format ("test_wav{}.wav", i);

          Exporter::Settings settings;
          settings.format_ = Exporter::Format::WAV;
          settings.artist_ = "Test Artist";
          settings.title_ = "Test Title";
          settings.genre_ = "Test Genre";
          settings.depth_ = BitDepth::BIT_DEPTH_16;
          settings.time_range_ = Exporter::TimeRange::Loop;
          if (j == 0)
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
          print_progress_and_sleep (*exporter.progress_info_);
          exporter.join_generic_thread ();

          exporter.post_export ();

          ASSERT_FALSE (AUDIO_ENGINE->exporting_.load ());

          z_chromaprint_check_fingerprint_similarity (
            file.abs_path_.c_str (), settings.file_uri_.c_str (), 83, 6);
          ASSERT_TRUE (audio_files_equal (
            file.abs_path_.c_str (), settings.file_uri_.c_str (), 151199,
            0.0001f));

          io_remove (settings.file_uri_);

          ASSERT_FALSE (TRANSPORT->isRolling ());
          ASSERT_EQ (TRANSPORT->playhead_pos_.frames_, 0);
        }
    }
}

static void
bounce_region (bool with_bpm_automation)
{
#ifdef HAVE_GEONKICK
  test_helper_zrythm_init ();

  Position pos, end_pos;
  pos.set_to_bar (2);
  end_pos.set_to_bar (4);

  if (with_bpm_automation)
    {
      /* create bpm automation */
      AutomationTrack * at = automation_track_find_from_port (
        P_TEMPO_TRACK->bpm_port, P_TEMPO_TRACK, false);
      Region * r = automation_region_new (
        &pos, &end_pos, P_TEMPO_TRACK->get_name_hash (), at->index, 0);
      bool success = track_add_region (P_TEMPO_TRACK, r, at, 0, 1, 0, nullptr);
      ASSERT_TRUE (success);
      pos.set_to_bar (1);
      AutomationPoint * ap =
        automation_point_new_float (168.434006f, 0.361445993f, &pos);
      automation_region_add_ap (r, ap, false);
      pos.set_to_bar (2);
      ap = automation_point_new_float (297.348999f, 0.791164994f, &pos);
      automation_region_add_ap (r, ap, false);
    }

  /* create the plugin track */
  test_plugin_manager_create_tracks_from_plugin (
    GEONKICK_BUNDLE, GEONKICK_URI, true, false, 1);
  Track * track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  track->select (true, true, false);

  /* create midi region */
  char * midi_file =
    g_build_filename (MIDILIB_TEST_MIDI_FILES_PATH, "M71.MID", nullptr);
  int      lane_pos = 0;
  int      idx_in_lane = 0;
  Region * region = midi_region_new_from_midi_file (
    &pos, midi_file, track->get_name_hash (), lane_pos, idx_in_lane, 0);
  bool success =
    track_add_region (track, region, nullptr, lane_pos, true, false, nullptr);
  ASSERT_TRUE (success);
  (ArrangerObject *) region->select (true, false, false);
  arranger_selections_action_perform_create (TL_SELECTIONS, nullptr);

  /* bounce it */
  ExportSettings * settings = export_settings_new ();
  settings->mode = Exporter::Mode::EXPORT_MODE_REGIONS;
  export_settings_set_bounce_defaults (
    settings, Exporter::Format::EXPORT_FORMAT_WAV, "", region->name);
  timeline_selections_mark_for_bounce (
    TL_SELECTIONS, settings->bounce_with_parents);
  position_add_ms (&settings->custom_end, 4000);

  EngineState state;
  GPtrArray * conns = exporter_prepare_tracks_for_export (settings, &state);

  /* start exporting in a new thread */
  GThread * thread = g_thread_new (
    "bounce_thread", (GThreadFunc) exporter_generic_export_thread, settings);

  print_progress_and_sleep (*settings->progress_info);

  g_thread_join (thread);

  exporter_post_export (settings, conns, &state);

  if (!with_bpm_automation)
    {
      char * filepath = g_build_filename (
        TESTS_SRCDIR, "test_mixdown_midi_routed_to_instrument_track.ogg",
        nullptr);
      z_chromaprint_check_fingerprint_similarity (
        filepath, settings->file_uri, 97, 34);
      g_free (filepath);
    }

  export_settings_free (settings);

  test_helper_zrythm_cleanup ();
#endif
}

TEST (Exporter, BounceRegion)
{
  bounce_region (false);
}

#if 0
static void
test_bounce_with_bpm_automation (void)
{
  bounce_region (true);
}
#endif

/**
 * Export the audio mixdown when a MIDI track with
 * data is routed to an instrument track.
 */
TEST_F (ZrythmFixture, MixdownMidiRoutedToInstrumentTrack)
{
#ifdef HAVE_GEONKICK
  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    GEONKICK_BUNDLE, GEONKICK_URI, true, false, 1);
  Track * ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  ins_track->select (true, true, false);

  char * midi_file =
    g_build_filename (MIDILIB_TEST_MIDI_FILES_PATH, "M71.MID", nullptr);

  /* create the MIDI track from a MIDI file */
  FileDescriptor * file = supported_file_new_from_path (midi_file);
  track_create_with_action (
    Track::Type::MIDI, nullptr, file, PLAYHEAD, TRACKLIST->tracks.size (), 1,
    -1, nullptr, nullptr);
  Track * midi_track = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);
  midi_track->select (true, true, false);

  /* route the MIDI track to the instrument track */
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, ins_track, nullptr);

  for (int k = 0; k < 2; k++)
    {
      /* bounce it */
      ExportSettings * settings = export_settings_new ();
      settings->mode = Exporter::Mode::EXPORT_MODE_FULL;
      export_settings_set_bounce_defaults (
        settings,
        k == 0
          ? Exporter::Format::EXPORT_FORMAT_WAV
          : Exporter::Format::EXPORT_FORMAT_FLAC,
        "", __func__);
      settings->time_range = ExportTimeRange::TIME_RANGE_LOOP;

      EngineState state;
      GPtrArray * conns = exporter_prepare_tracks_for_export (settings, &state);

      /* start exporting in a new thread */
      GThread * thread = g_thread_new (
        "bounce_thread", (GThreadFunc) exporter_generic_export_thread, settings);

      print_progress_and_sleep (*settings->progress_info);

      g_thread_join (thread);

      exporter_post_export (settings, conns, &state);

      char * filepath = g_build_filename (
        TESTS_SRCDIR, "test_mixdown_midi_routed_to_instrument_track.ogg",
        nullptr);
      z_chromaprint_check_fingerprint_similarity (
        filepath, settings->file_uri, 97, 34);
      g_free (filepath);

      export_settings_free (settings);
    }
#endif
}

TEST_F (ZrythmFixture, BounceRegionWithFirstNote)
{
#ifdef HAVE_HELM
  Position pos, end_pos;
  pos.set_to_bar (2);
  end_pos.set_to_bar (4);

  /* create the plugin track */
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  auto track = TRACKLIST->get_last_track<InstrumentTrack> ();
  track->select (true, true, false);

  /* create midi region */
  const auto midi_file =
    Glib::build_filename (MIDILIB_TEST_MIDI_FILES_PATH, "M1.MID");
  const int lane_pos = 0;
  const int idx_in_lane = 0;
  auto      region = std::make_shared<MidiRegion> (
    pos, midi_file, track->get_name_hash (), lane_pos, idx_in_lane, 0);
  track->add_region (region, nullptr, lane_pos, true, false);
  region->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));

  pos.zero ();
  pos.add_beats (3);
  region->loop_start_pos_setter (&pos);
  region->clip_start_pos_setter (&pos);

  while (region->midi_notes_.size () > 1)
    {
      region->remove_object (*region->midi_notes_.back ());
    }
  ASSERT_EQ (
    region->midi_notes_[0]->pos_.frames_, region->loop_start_pos_.frames_);

  for (int k = 0; k < 2; k++)
    {
      /* bounce it */
      Exporter::Settings settings;
      settings.mode_ = Exporter::Mode::Regions;
      settings.set_bounce_defaults (
        k == 0 ? Exporter::Format::WAV : Exporter::Format::FLAC, "",
        region->name_);
      TL_SELECTIONS->mark_for_bounce (settings.bounce_with_parents_);
      settings.custom_end_.add_ms (4000);
      Exporter exporter (settings);
      exporter.prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

      /* start exporting in a new thread */
      exporter.begin_generic_thread ();
      print_progress_and_sleep (*exporter.progress_info_);
      exporter.join_generic_thread ();
      exporter.post_export ();

      /* assert non silent */
      AudioClip clip (exporter.settings_.file_uri_);
      bool      has_audio = false;
      for (unsigned_frame_t i = 0; i < clip.num_frames_; i++)
        {
          for (channels_t j = 0; j < clip.channels_; j++)
            {
              if (
                std::abs (clip.ch_frames_.getSample ((int) j, (int) i)) > 1e-10f)
                {
                  has_audio = true;
                  break;
                }
            }
        }
      ASSERT_TRUE (has_audio);
    }
#endif
}

static void
test_bounce_midi_track_routed_to_instrument_track (
  BounceStep bounce_step,
  bool       with_parents)
{
#ifdef HAVE_GEONKICK
  test_helper_zrythm_init ();

  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    GEONKICK_BUNDLE, GEONKICK_URI, true, false, 1);
  Track * ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  ins_track->select (true, true, false);

  char * midi_file =
    g_build_filename (MIDILIB_TEST_MIDI_FILES_PATH, "M71.MID", nullptr);

  /* create the MIDI track from a MIDI file */
  FileDescriptor * file = supported_file_new_from_path (midi_file);
  track_create_with_action (
    Track::Type::MIDI, nullptr, file, PLAYHEAD, TRACKLIST->tracks.size (), 1,
    -1, nullptr, nullptr);
  Track * midi_track = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);
  midi_track->select (true, true, false);

  /* route the MIDI track to the instrument track */
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, ins_track, nullptr);

  /* bounce it */
  ExportSettings * settings = export_settings_new ();
  settings->mode = Exporter::Mode::EXPORT_MODE_TRACKS;
  export_settings_set_bounce_defaults (
    settings, Exporter::Format::EXPORT_FORMAT_WAV, "", __func__);
  settings->time_range = ExportTimeRange::TIME_RANGE_LOOP;
  settings->bounce_with_parents = with_parents;
  settings->bounce_step = bounce_step;

  /* mark the track for bounce */
  tracklist_selections_mark_for_bounce (
    TRACKLIST_SELECTIONS, settings->bounce_with_parents, F_NO_MARK_MASTER);

  EngineState state;
  GPtrArray * conns = exporter_prepare_tracks_for_export (settings, &state);

  /* start exporting in a new thread */
  GThread * thread = g_thread_new (
    "bounce_thread", (GThreadFunc) exporter_generic_export_thread, settings);

  print_progress_and_sleep (*settings->progress_info);

  g_thread_join (thread);

  exporter_post_export (settings, conns, &state);

  if (with_parents)
    {
      char * filepath = g_build_filename (
        TESTS_SRCDIR, "test_mixdown_midi_routed_to_instrument_track.ogg",
        nullptr);
      z_chromaprint_check_fingerprint_similarity (
        filepath, settings->file_uri, 97, 34);
      ASSERT_TRUE (
        audio_files_equal (filepath, settings->file_uri, 120000, 0.01f));
      g_free (filepath);
    }
  else
    {
      /* assume silence */
      ASSERT_TRUE (audio_file_is_silent (settings->file_uri));
    }
  io_remove (settings->file_uri);

  export_settings_free (settings);

  test_helper_zrythm_cleanup ();
#endif
}

TEST (Exporter, BounceMidiTrackRoutedToInstrumentTrack)
{
  test_bounce_midi_track_routed_to_instrument_track (
    BounceStep::PostFader, true);
  test_bounce_midi_track_routed_to_instrument_track (
    BounceStep::PostFader, false);
}

static void
test_bounce_instrument_track (BounceStep bounce_step, bool with_parents)
{
  z_info ("=== Bounce instrument track start ===");
#if defined(HAVE_GEONKICK) && defined(HAVE_MVERB)
  ZrythmFixture fixture;

  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    GEONKICK_BUNDLE, GEONKICK_URI, true, false, 1);
  Track * ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  ins_track->select (true, true, false);

  /* CM Kleer Arp */
  /*plugin_set_selected_preset_by_name (*/
  /*ins_track->channel->instrument, "CM Supersaw");*/
  /*z_warn_if_reached ();*/

  /* create a MIDI region on the instrument track */
  char * midi_file =
    g_build_filename (MIDILIB_TEST_MIDI_FILES_PATH, "M71.MID", nullptr);
  Region * r = midi_region_new_from_midi_file (
    PLAYHEAD, midi_file, ins_track->get_name_hash (), 0, 0, 0);
  g_free (midi_file);
  bool success =
    track_add_region (ins_track, r, nullptr, 0, true, false, nullptr);
  ASSERT_TRUE (success);
  (ArrangerObject *) r->select (true, false, false);
  arranger_selections_action_perform_create (TL_SELECTIONS, nullptr);

  /* add MVerb insert */
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (MVERB_BUNDLE, MVERB_URI, false);
  mixer_selections_action_perform_create (
    zrythm::plugins::PluginSlotType::Insert, ins_track->get_name_hash (), 0,
    setting, 1, nullptr);

  /* adjust fader */
  Fader * fader = track_get_fader (ins_track, true);
  Port *  port = fader->amp;
  port_action_perform (
    PORT_ACTION_SET_CONTROL_VAL, &port->id_, 0.5f, false, nullptr);
  ASSERT_NEAR (port->control, 0.5f, 0.00001f);

  /* bounce it */
  ExportSettings * settings = export_settings_new ();
  settings->mode = Exporter::Mode::EXPORT_MODE_TRACKS;
  export_settings_set_bounce_defaults (
    settings, Exporter::Format::EXPORT_FORMAT_WAV, "", __func__);
  settings->time_range = ExportTimeRange::TIME_RANGE_LOOP;
  settings->bounce_with_parents = with_parents;
  settings->bounce_step = bounce_step;

  /* mark the track for bounce */
  tracklist_selections_mark_for_bounce (
    TRACKLIST_SELECTIONS, settings->bounce_with_parents, F_NO_MARK_MASTER);

  {
    EngineState state;
    GPtrArray * conns = exporter_prepare_tracks_for_export (settings, &state);

    /* start exporting in a new thread */
    GThread * thread = g_thread_new (
      "bounce_thread", (GThreadFunc) exporter_generic_export_thread, settings);

    print_progress_and_sleep (*settings->progress_info);

    g_thread_join (thread);

    exporter_post_export (settings, conns, &state);
  }

#  define CHECK_SAME_AS_FILE(dirname, x, match_rate) \
    char * filepath = g_build_filename (dirname, x, nullptr); \
    if (match_rate == 100) \
      { \
        ASSERT_TRUE ( \
          audio_files_equal (filepath, settings->file_uri, 151199, 0.01f)); \
      } \
    else \
      { \
        z_chromaprint_check_fingerprint_similarity ( \
          filepath, settings->file_uri, match_rate, 34); \
      } \
    g_free (filepath)

  if (with_parents || bounce_step == BounceStep::BOUNCE_STEP_POST_FADER)
    {
      CHECK_SAME_AS_FILE (
        TESTS_BUILDDIR,
        "test_mixdown_midi_routed_to_instrument_track_w_reverb_half_gain.ogg",
        /* FIXME was 94 */
        71);
    }
  else if (bounce_step == BounceStep::BOUNCE_STEP_BEFORE_INSERTS)
    {
      CHECK_SAME_AS_FILE (
        TESTS_SRCDIR, "test_mixdown_midi_routed_to_instrument_track.ogg", 100);
    }
  else if (bounce_step == BounceStep::BOUNCE_STEP_PRE_FADER)
    {
      CHECK_SAME_AS_FILE (
        TESTS_BUILDDIR,
        "test_mixdown_midi_routed_to_instrument_track_w_reverb.ogg", 85);
    }

#  undef CHECK_SAME_AS_FILE

  /* --- check bounce song with offset --- */

  /* move playhead to bar 3 */
  TRANSPORT->set_playhead_to_bar (3);

  /* move start marker and region to bar 2 */
  Marker * start_marker = marker_track_get_start_marker (P_MARKER_TRACK);
  (ArrangerObject *) start_marker->select (true, false, false);
  (ArrangerObject *) r->select (true, true, false);
  success = arranger_selections_action_perform_move_timeline (
    TL_SELECTIONS, TRANSPORT->ticks_per_bar, 0, 0, F_NOT_ALREADY_MOVED, nullptr,
    nullptr);
  ASSERT_TRUE (success);

  /* move end marker to bar 6 */
  Marker *         end_marker = marker_track_get_end_marker (P_MARKER_TRACK);
  ArrangerObject * end_marker_obj = (ArrangerObject *) end_marker;
  end_marker_obj->select (true, false, false);
  success = arranger_selections_action_perform_move_timeline (
    TL_SELECTIONS, TRANSPORT->ticks_per_bar * 6 - end_marker_obj->pos.ticks, 0,
    0, F_NOT_ALREADY_MOVED, nullptr, nullptr);
  ASSERT_TRUE (success);

  /* export again */
  export_settings_free (settings);
  settings = export_settings_new ();
  settings->mode = Exporter::Mode::EXPORT_MODE_TRACKS;
  export_settings_set_bounce_defaults (
    settings, Exporter::Format::EXPORT_FORMAT_WAV, "", __func__);
  settings->time_range = ExportTimeRange::TIME_RANGE_SONG;
  settings->bounce_with_parents = with_parents;
  settings->bounce_step = bounce_step;

  /* mark the track for bounce */
  tracklist_selections_mark_for_bounce (
    TRACKLIST_SELECTIONS, settings->bounce_with_parents, F_NO_MARK_MASTER);

  {
    EngineState state;
    GPtrArray * conns = exporter_prepare_tracks_for_export (settings, &state);

    /* start exporting in a new thread */
    GThread * thread = g_thread_new (
      "bounce_thread", (GThreadFunc) exporter_generic_export_thread, settings);

    print_progress_and_sleep (*settings->progress_info);

    g_thread_join (thread);

    exporter_post_export (settings, conns, &state);
  }

  /* create audio track with bounced material */
  ArrangerObject * start_marker_obj = (ArrangerObject *) start_marker;
  exporter_create_audio_track_after_bounce (settings, &start_marker_obj->pos);

  /* assert exported material starts at start
   * marker and ends at end marker */
  Track *  audio_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  Region * bounced_r = audio_track->lanes[0]->regions[0];
  ArrangerObject * bounce_r_obj = (ArrangerObject *) bounced_r;
  ASSERT_POSITION_EQ (start_marker_obj->pos, bounce_r_obj->pos);
  ASSERT_POSITION_EQ (end_marker_obj->pos, bounce_r_obj->end_pos_);
#endif
  z_info ("=== Bounce instrument track end ===");
}

TEST (Exporter, BounceInstrumentTrack)
{
  test_bounce_instrument_track (BounceStep::PostFader, true);
  test_bounce_instrument_track (BounceStep::BeforeInserts, false);
  test_bounce_instrument_track (BounceStep::PreFader, false);
  test_bounce_instrument_track (BounceStep::PostFader, false);
}

/**
 * Export the audio mixdown when the chord track with
 * data is routed to an instrument track.
 */
TEST_F (ZrythmFixture, ExportChordTrackRoutedToInstrumentTrack)
{
#ifdef HAVE_GEONKICK
  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    GEONKICK_BUNDLE, GEONKICK_URI, true, false, 1);
  Track * ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  ins_track->select (true, true, false);

  /* create the chords */
  Position start_pos, end_pos;
  start_pos.zero ();
  end_pos.set_to_bar (3);
  Region * r =
    chord_region_new (&start_pos, &end_pos, P_CHORD_TRACK->num_chord_regions);
  bool success =
    track_add_region (P_CHORD_TRACK, r, nullptr, -1, true, true, nullptr);
  ASSERT_TRUE (success);
  ChordObject *    chord = chord_object_new (&r->id, 0, r->num_chord_objects);
  ArrangerObject * chord_obj = (ArrangerObject *) chord;
  chord_region_add_chord_object (r, chord, true);
  arranger_object_set_position (
    chord_obj, &start_pos, ArrangerObject::PositionType::START, F_NO_VALIDATE);
  chord = chord_object_new (&r->id, 0, r->num_chord_objects);
  chord_obj = (ArrangerObject *) chord;
  chord_region_add_chord_object (r, chord, true);
  start_pos.set_to_bar (2);
  arranger_object_set_position (
    chord_obj, &start_pos, ArrangerObject::PositionType::START, F_NO_VALIDATE);
  P_CHORD_TRACK->select (true, true, false);

  /* route the chord track to the instrument track */
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, ins_track, nullptr);

  for (int i = 0; i < 2; i++)
    {
      /* bounce */
      ExportSettings * settings = export_settings_new ();
      settings->mode =
        i == 0
          ? Exporter::Mode::EXPORT_MODE_FULL
          : Exporter::Mode::EXPORT_MODE_TRACKS;
      export_settings_set_bounce_defaults (
        settings, Exporter::Format::EXPORT_FORMAT_WAV, "", __func__);
      settings->time_range = ExportTimeRange::TIME_RANGE_LOOP;

      if (i == 1)
        {
          tracklist_mark_all_tracks_for_bounce (TRACKLIST, false);
          for (int j = 0; j < TRACKLIST->tracks.size (); j++)
            {
              Track * track = TRACKLIST->tracks[j];
              track_mark_for_bounce (
                track, F_BOUNCE, F_MARK_REGIONS, F_NO_MARK_CHILDREN,
                F_MARK_PARENTS);
            }
        }

      EngineState state;
      GPtrArray * conns = exporter_prepare_tracks_for_export (settings, &state);

      /* start exporting in a new thread */
      GThread * thread = g_thread_new (
        "bounce_thread", (GThreadFunc) exporter_generic_export_thread, settings);

      print_progress_and_sleep (*settings->progress_info);

      g_thread_join (thread);

      exporter_post_export (settings, conns, &state);

      ASSERT_FALSE (audio_file_is_silent (settings->file_uri));

      export_settings_free (settings);
    }
#endif
}

/**
 * Export send track only (stem export).
 */
TEST_F (ZrythmFixture, ExportSendTrackOnly)
{
  /* create an audio track */
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test.wav");
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);
  auto audio_track = TRACKLIST->get_last_track<AudioTrack> ();

  /* create an audio FX track */
  auto audio_fx_track = Track::create_empty_with_action<AudioBusTrack> ();

  /* on first iteration, there is no send connected
   * to the audio fx track so we expect it to be
   * silent. on second iteration, there is a send
   * connection so we expect the audio to not be
   * silent */
  for (int i = 0; i < 2; i++)
    {
      /* on first iteration we use a pre-fader
       * send and on second iteration we use a
       * post-fader send. in both cases we expect
       * there to be audio */
      for (int j = 0; j < 2; j++)
        {
          if (i == 1)
            {
              /* create a send to the audio fx track */
              ASSERT_NO_THROW (UNDO_MANAGER->perform (
                std::make_unique<ChannelSendConnectStereoAction> (
                  *audio_track->channel_->sends_.at (
                    j == 0 ? 0 : CHANNEL_SEND_POST_FADER_START_SLOT),
                  *audio_fx_track->processor_->stereo_in_,
                  *PORT_CONNECTIONS_MGR)));
            }

          /* bounce */
          Exporter::Settings settings;
          settings.set_bounce_defaults (Exporter::Format::WAV, "", __func__);
          settings.time_range_ = Exporter::TimeRange::Loop;
          settings.bounce_with_parents_ = true;
          settings.mode_ = Exporter::Mode::Tracks;

          /* unmark all tracks for bounce */
          TRACKLIST->mark_all_tracks_for_bounce (false);

          /* mark only the audio fx track for bounce */
          audio_fx_track->mark_for_bounce (true, true, true, true);

          Exporter exporter (settings);
          exporter.prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

          /* start exporting in a new thread */
          exporter.begin_generic_thread ();
          print_progress_and_sleep (*exporter.progress_info_);
          exporter.join_generic_thread ();
          exporter.post_export ();

          ASSERT_EQ (
            audio_file_is_silent (exporter.get_exported_path ().c_str ()),
            i == 0);

          /* skip unnecessary iteration */
          if (i == 0 && j == 0)
            j = 1;
        }
    }
}

TEST_F (ZrythmFixture, ExportMidiRange)
{
  /* create a MIDI track with 1 region with 1 MIDI note */
  auto track = Track::create_empty_with_action<MidiTrack> ();
  track->select (true, true, false);

  Position start, end;
  start.set_to_bar (2);
  end.set_to_bar (4);
  auto r = std::make_shared<MidiRegion> (start, end, track->name_hash_, 0, 0);
  track->add_region (r, nullptr, 0, true, false);

  /* midi note 1 */
  start.zero ();
  end.set_to_bar (2);
  auto mn = std::make_shared<MidiNote> (r->id_, start, end, 70, 70);
  r->append_object (mn);

  /* bounce */
  Exporter::Settings settings;
  settings.mode_ = Exporter::Mode::Full;
  settings.set_bounce_defaults (Exporter::Format::Midi1, "", __func__);
  settings.time_range_ = Exporter::TimeRange::Custom;
  settings.custom_start_.set_to_bar (2);
  settings.custom_end_.set_to_bar (4);

  Exporter exporter (settings);
  exporter.prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

  /* start exporting in a new thread */
  exporter.begin_generic_thread ();
  print_progress_and_sleep (*exporter.progress_info_);
  exporter.join_generic_thread ();
  exporter.post_export ();

  /* create a MIDI track from the MIDI file */
  FileDescriptor file (exporter.get_exported_path ());
  Track::create_with_action (
    Track::Type::Midi, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);
  auto exported_track = TRACKLIST->get_last_track<MidiTrack> ();

  /* verify correct data */
  ASSERT_SIZE_EQ (exported_track->lanes_, 2);
  ASSERT_SIZE_EQ (exported_track->lanes_[0]->regions_, 1);
  ASSERT_EMPTY (exported_track->lanes_[1]->regions_);
  r = exported_track->lanes_[0]->regions_[0];
  ASSERT_SIZE_EQ (r->midi_notes_, 1);
  mn = r->midi_notes_[0];
  start.set_to_bar (1);
  end.set_to_bar (2);
  ASSERT_POSITION_EQ (mn->pos_, start);
  ASSERT_POSITION_EQ (mn->end_pos_, end);
}
