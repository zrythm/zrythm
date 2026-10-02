// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/file_importer.h"
#include "actions/track_creator.h"
#include "structure/scenes/scene.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"
#include "utils/app_settings.h"
#include "utils/io_utils.h"
#include "utils/object_registry.h"

#include <QTemporaryDir>
#include <QTemporaryFile>

#include "helpers/in_memory_settings_backend.h"

#include "unit/actions/mock_undo_stack.h"
#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class FileImporterTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create singleton tracks
    singleton_tracks_ = std::make_unique<structure::tracks::SingletonTracks> ();

    // Create track collection
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (registry_);

    // Create track routing
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (registry_);

    // Create track factory with dependencies
    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,
      registry_,
      soloed_tracks_exist_getter_,
      {},
    };
    track_factory_ = std::make_unique<structure::tracks::TrackFactory> (
      [factory_deps] () -> structure::tracks::FinalTrackDependencies {
        return factory_deps;
      });

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Create and register singleton tracks
    auto master_track_ref =
      track_factory_->create_empty_track<structure::tracks::MasterTrack> ();
    auto chord_track_ref =
      track_factory_->create_empty_track<structure::tracks::ChordTrack> ();
    auto modulator_track_ref =
      track_factory_->create_empty_track<structure::tracks::ModulatorTrack> ();
    auto marker_track_ref =
      track_factory_->create_empty_track<structure::tracks::MarkerTrack> ();

    // Set singleton track pointers
    singleton_tracks_->setMasterTrack (
      master_track_ref.get_object_as<structure::tracks::MasterTrack> ());
    singleton_tracks_->setChordTrack (
      chord_track_ref.get_object_as<structure::tracks::ChordTrack> ());
    singleton_tracks_->setModulatorTrack (
      modulator_track_ref.get_object_as<structure::tracks::ModulatorTrack> ());
    singleton_tracks_->setMarkerTrack (
      marker_track_ref.get_object_as<structure::tracks::MarkerTrack> ());

    // Add singleton tracks to collection
    track_collection_->add_track (master_track_ref);
    track_collection_->add_track (chord_track_ref);
    track_collection_->add_track (modulator_track_ref);
    track_collection_->add_track (marker_track_ref);

    // Create mock snap grids
    snap_grid_timeline = std::make_unique<dsp::SnapGrid> (
      tempo_map_, dsp::notes::NoteLength::Note_1_4, [] () { return 100.0; });
    snap_grid_editor = std::make_unique<dsp::SnapGrid> (
      tempo_map_, dsp::notes::NoteLength::Note_1_4, [] () { return 50.0; });

    // Create arranger object factory with proper dependencies
    app_settings_ = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    structure::arrangement::ArrangerObjectFactory::Dependencies obj_factory_deps{
      .tempo_map_ = tempo_map_wrapper_,
      .registry_ = registry_,
      .last_timeline_obj_len_provider_ = [] () { return 100.0; },
      .last_editor_obj_len_provider_ = [] () { return 50.0; },
      .automation_curve_algorithm_provider_ =
        [] () { return dsp::CurveOptions::Algorithm::Exponent; }
    };

    arranger_object_factory = std::make_unique<
      structure::arrangement::ArrangerObjectFactory> (
      obj_factory_deps, [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });

    // Create track creator
    track_creator_ = std::make_unique<TrackCreator> (
      *undo_stack_, *track_factory_, *track_collection_, *track_routing_,
      *singleton_tracks_);

    // Create arranger object creator
    arranger_object_creator_ = std::make_unique<ArrangerObjectCreator> (
      *undo_stack_, *arranger_object_factory, *snap_grid_timeline,
      *snap_grid_editor);

    // Create file importer
    file_importer_ = std::make_unique<FileImporter> (
      *undo_stack_, *arranger_object_creator_, *track_creator_,
      *track_collection_, tempo_map_);

    // Create test files
    setupTestFiles ();
  }

  void TearDown () override
  {
    file_importer_.reset ();
    arranger_object_creator_.reset ();
    track_creator_.reset ();
    snap_grid_editor.reset ();
    snap_grid_timeline.reset ();
    arranger_object_factory.reset ();
    undo_stack_.reset ();
    track_factory_.reset ();
    track_routing_.reset ();
    track_collection_.reset ();
    singleton_tracks_.reset ();
  }

  void setupTestFiles ()
  {
    // Create temporary directory for test files
    temp_dir_ = utils::io::make_tmp_dir ();
    temp_dir_path_ =
      utils::Utf8String::from_qstring (temp_dir_->path ()).to_path ();

    // Create a mock audio file (WAV header with minimal data)
    audio_file_path_ = temp_dir_path_ / "test_audio.wav";
    createMockWavFile (audio_file_path_);

    // Create a mock MIDI file
    midi_file_path_ = temp_dir_path_ / "test_midi.mid";
    createMockMidiFile (midi_file_path_);

    // Create a non-supported file
    unsupported_file_path_ = temp_dir_path_ / "test_file.txt";
    utils::io::set_file_contents (
      unsupported_file_path_, u8"This is not audio or MIDI");
  }

  void createMockWavFile (const std::filesystem::path &path)
  {
    // Minimal WAV file header (44 bytes) + some silence
    std::vector<uint8_t> wav_data = {
      // RIFF header
      'R',
      'I',
      'F',
      'F', // ChunkID
      0x24,
      0x08,
      0x00,
      0x00, // ChunkSize (36 + data_size)
      'W',
      'A',
      'V',
      'E', // Format

      // fmt subchunk
      'f',
      'm',
      't',
      ' ', // Subchunk1ID
      0x10,
      0x00,
      0x00,
      0x00, // Subchunk1Size (16)
      0x01,
      0x00, // AudioFormat (1 = PCM)
      0x01,
      0x00, // NumChannels (1)
      0x44,
      0xAC,
      0x00,
      0x00, // SampleRate (44100)
      0x44,
      0xAC,
      0x00,
      0x00, // ByteRate (44100)
      0x01,
      0x00, // BlockAlign (1)
      0x08,
      0x00, // BitsPerSample (8)

      // data subchunk
      'd',
      'a',
      't',
      'a', // Subchunk2ID
      0x00,
      0x08,
      0x00,
      0x00, // Subchunk2Size (2048 samples)
    };

    // Add 2048 samples of silence
    wav_data.insert (wav_data.end (), 2048, 0x80);

    utils::io::set_file_contents (
      path, reinterpret_cast<const char *> (wav_data.data ()), wav_data.size ());
  }

  void createMockMidiFile (const std::filesystem::path &path)
  {
    // Minimal MIDI file header and one note
    std::vector<uint8_t> midi_data = {
      // MThd header
      'M', 'T', 'h', 'd',     // Header chunk
      0x00, 0x00, 0x00, 0x06, // Header length (6)
      0x00, 0x00,             // Format type (0)
      0x00, 0x01,             // Number of tracks (1)
      0x00, 0x60,             // Division (96 ticks per quarter note)

      // MTrk header
      'M', 'T', 'r', 'k',     // Track chunk
      0x00, 0x00, 0x00, 0x0C, // Track length (12 bytes)

      // Track events
      0x00,             // Delta time (0)
      0x90, 0x3C, 0x40, // Note on (C4, velocity 64)
      0x60,             // Delta time (96 ticks = 0x60)
      0x80, 0x3C, 0x40, // Note off (C4, velocity 64)
      0x00,             // Delta time (0)
      0xFF, 0x2F, 0x00  // End of track
    };

    utils::io::set_file_contents (
      path, reinterpret_cast<const char *> (midi_data.data ()),
      midi_data.size ());
  }

  // Create minimal dependencies for track creation
  dsp::TempoMap                  tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper           tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry          registry_;
  dsp::graph_test::MockTransport transport_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

  std::unique_ptr<structure::tracks::SingletonTracks> singleton_tracks_;
  std::unique_ptr<structure::tracks::TrackCollection> track_collection_;
  std::unique_ptr<structure::tracks::TrackRouting>    track_routing_;
  std::unique_ptr<structure::tracks::TrackFactory>    track_factory_;
  std::unique_ptr<undo::UndoStack>                    undo_stack_;
  std::unique_ptr<dsp::SnapGrid>                      snap_grid_timeline;
  std::unique_ptr<dsp::SnapGrid>                      snap_grid_editor;
  std::unique_ptr<utils::AppSettings>                 app_settings_;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory>
                                         arranger_object_factory;
  std::unique_ptr<TrackCreator>          track_creator_;
  std::unique_ptr<ArrangerObjectCreator> arranger_object_creator_;
  std::unique_ptr<FileImporter>          file_importer_;

  // Test files
  std::unique_ptr<QTemporaryDir> temp_dir_;
  std::filesystem::path          temp_dir_path_;
  std::filesystem::path          audio_file_path_;
  std::filesystem::path          midi_file_path_;
  std::filesystem::path          unsupported_file_path_;
};

// Test file type detection
TEST_F (FileImporterTest, GetFileType)
{
  // Test audio file detection
  auto audio_file_qstr =
    utils::Utf8String::from_path (audio_file_path_).to_qstring ();
  EXPECT_EQ (
    file_importer_->getFileType (audio_file_qstr),
    FileImporter::FileType::Audio);

  // Test MIDI file detection
  auto midi_file_qstr =
    utils::Utf8String::from_path (midi_file_path_).to_qstring ();
  EXPECT_EQ (
    file_importer_->getFileType (midi_file_qstr), FileImporter::FileType::Midi);

  // Test unsupported file detection
  auto unsupported_file_qstr =
    utils::Utf8String::from_path (unsupported_file_path_).to_qstring ();
  EXPECT_EQ (
    file_importer_->getFileType (unsupported_file_qstr),
    FileImporter::FileType::Unsupported);

  // Test non-existent file
  QString non_existent = "/non/existent/file.wav";
  EXPECT_EQ (
    file_importer_->getFileType (non_existent),
    FileImporter::FileType::Unsupported);
}

// Test audio file detection
TEST_F (FileImporterTest, IsAudioFile)
{
  // Test valid audio file
  auto audio_file_qstr =
    utils::Utf8String::from_path (audio_file_path_).to_qstring ();
  EXPECT_TRUE (file_importer_->isAudioFile (audio_file_qstr));

  // Test MIDI file (should not be detected as audio)
  auto midi_file_qstr =
    utils::Utf8String::from_path (midi_file_path_).to_qstring ();
  EXPECT_FALSE (file_importer_->isAudioFile (midi_file_qstr));

  // Test unsupported file
  auto unsupported_file_qstr =
    utils::Utf8String::from_path (unsupported_file_path_).to_qstring ();
  EXPECT_FALSE (file_importer_->isAudioFile (unsupported_file_qstr));

  // Test non-existent file
  QString non_existent = "/non/existent/file.wav";
  EXPECT_FALSE (file_importer_->isAudioFile (non_existent));
}

// Test MIDI file detection
TEST_F (FileImporterTest, IsMidiFile)
{
  // Test valid MIDI file
  auto midi_file_qstr =
    utils::Utf8String::from_path (midi_file_path_).to_qstring ();
  EXPECT_TRUE (file_importer_->isMidiFile (midi_file_qstr));

  // Test audio file (should not be detected as MIDI)
  auto audio_file_qstr =
    utils::Utf8String::from_path (audio_file_path_).to_qstring ();
  EXPECT_FALSE (file_importer_->isMidiFile (audio_file_qstr));

  // Test unsupported file
  auto unsupported_file_qstr =
    utils::Utf8String::from_path (unsupported_file_path_).to_qstring ();
  EXPECT_FALSE (file_importer_->isMidiFile (unsupported_file_qstr));

  // Test non-existent file
  QString non_existent = "/non/existent/file.mid";
  EXPECT_FALSE (file_importer_->isMidiFile (non_existent));
}

// Test importing single audio file
TEST_F (FileImporterTest, ImportSingleAudioFile)
{
  const auto initial_track_count = track_collection_->track_count ();
  const auto initial_undo_count = undo_stack_->count ();

  QStringList files;
  files.append (utils::Utf8String::from_path (audio_file_path_).to_qstring ());

  file_importer_->importFiles (files, 0.0, nullptr);

  // Should have created one new audio track
  EXPECT_EQ (track_collection_->track_count (), initial_track_count + 1);

  // Should have pushed commands to undo stack
  EXPECT_GT (undo_stack_->count (), initial_undo_count);
}

// Test importing single MIDI file
TEST_F (FileImporterTest, ImportSingleMidiFile)
{
  const auto initial_track_count = track_collection_->track_count ();
  const auto initial_undo_count = undo_stack_->count ();

  QStringList files;
  files.append (utils::Utf8String::from_path (midi_file_path_).to_qstring ());

  file_importer_->importFiles (files, 0.0, nullptr);

  // Should have created one new MIDI track
  ASSERT_EQ (track_collection_->track_count (), initial_track_count + 1);

  // Should have pushed commands to undo stack
  EXPECT_GT (undo_stack_->count (), initial_undo_count);

  const auto * midi_track = qobject_cast<structure::tracks::MidiTrack *> (
    track_collection_->tracks ()[track_collection_->track_count () - 1].get ());
  ASSERT_NE (midi_track, nullptr);

  // the track is named after the imported file
  EXPECT_EQ (midi_track->name (), "test_midi");

  // lane 0 holds one clip containing the mock file's single C4 note
  const auto clips =
    midi_track->lanes ()
      ->at (0)
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_sorted_children_view ();
  ASSERT_EQ (clips.size (), 1);
  const auto * clip = (*clips.begin ());
  const auto   notes = clip->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiNote>::get_sorted_children_view ();
  ASSERT_EQ (notes.size (), 1);
  const auto * note = (*notes.begin ());
  EXPECT_EQ (note->pitch (), 60);
  EXPECT_EQ (note->velocity (), 64);
  EXPECT_DOUBLE_EQ (note->position ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (
    note->position ()->ticks () + note->length ()->ticks (), 960.0);

  // the clip is sized to the note content
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 960.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 960.0);
}

// A multi-track MIDI file creates one clip per MIDI track, each on its own
// lane inside a single imported track
TEST_F (FileImporterTest, ImportMultiTrackMidiFileCreatesLanePerTrack)
{
  const auto initial_track_count = track_collection_->track_count ();

  QStringList files;
  files.append (
    utils::Utf8String::from_path (
      std::filesystem::path (TEST_MIDI_FILES_DIR)
      / "format_1_two_tracks_with_data.mid")
      .to_qstring ());

  file_importer_->importFiles (files, 0.0, nullptr);

  ASSERT_EQ (track_collection_->track_count (), initial_track_count + 1);
  const auto * midi_track = qobject_cast<structure::tracks::MidiTrack *> (
    track_collection_->tracks ()[track_collection_->track_count () - 1].get ());
  ASSERT_NE (midi_track, nullptr);

  const auto lane_count = midi_track->lanes ()->size ();
  ASSERT_GE (lane_count, 3);
  for (const auto lane_index : std::views::iota (0uz, 2uz))
    {
      const auto clips =
        midi_track->lanes ()
          ->at (lane_index)
          ->structure::arrangement::ArrangerObjectOwner<
            structure::arrangement::MidiClip>::get_sorted_children_view ();
      ASSERT_EQ (clips.size (), 1);
      EXPECT_GT ((*clips.begin ())->length ()->ticks (), 0.0);
    }

  // the trailing lane stays empty
  const auto trailing_clips =
    midi_track->lanes ()
      ->at (lane_count - 1)
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_sorted_children_view ();
  EXPECT_TRUE (trailing_clips.empty ());
}

// Importing a MIDI file without notes is refused without creating a track
TEST_F (FileImporterTest, ImportEmptyMidiFileIsRefused)
{
  const auto initial_track_count = track_collection_->track_count ();
  const auto initial_undo_count = undo_stack_->count ();

  QStringList failed_files;
  QObject::connect (
    file_importer_.get (), &FileImporter::importFailed, file_importer_.get (),
    [&failed_files] (const QString &filePath, const QString &) {
      failed_files.append (filePath);
    });

  QStringList files;
  files.append (
    utils::Utf8String::from_path (
      std::filesystem::path (TEST_MIDI_FILES_DIR) / "empty_midi_file_type1.mid")
      .to_qstring ());

  file_importer_->importFiles (files, 0.0, nullptr);

  EXPECT_EQ (failed_files.size (), 1);
  EXPECT_EQ (track_collection_->track_count (), initial_track_count);
  EXPECT_EQ (undo_stack_->count (), initial_undo_count);
}

// Importing an audio file into a MIDI track is refused
TEST_F (FileImporterTest, ImportAudioIntoMidiTrackIsRefused)
{
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);
  auto *     midi_track = track_result.value<structure::tracks::MidiTrack *> ();
  const auto initial_clip_count =
    midi_track->lanes ()
      ->at (0)
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_sorted_children_view ()
      .size ();

  QStringList failed_files;
  QObject::connect (
    file_importer_.get (), &FileImporter::importFailed, file_importer_.get (),
    [&failed_files] (const QString &filePath, const QString &) {
      failed_files.append (filePath);
    });

  QStringList files;
  files.append (utils::Utf8String::from_path (audio_file_path_).to_qstring ());

  file_importer_->importFiles (files, 0.0, midi_track);

  EXPECT_EQ (failed_files.size (), 1);
  EXPECT_EQ (
    midi_track->lanes ()
      ->at (0)
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_sorted_children_view ()
      .size (),
    initial_clip_count);
}

// Test undo/redo of a multi-track MIDI import restores the clips
TEST_F (FileImporterTest, UndoRedoMultiTrackMidiFileImport)
{
  const auto initial_track_count = track_collection_->track_count ();

  QStringList files;
  files.append (
    utils::Utf8String::from_path (
      std::filesystem::path (TEST_MIDI_FILES_DIR)
      / "format_1_two_tracks_with_data.mid")
      .to_qstring ());

  file_importer_->importFiles (files, 0.0, nullptr);
  ASSERT_EQ (track_collection_->track_count (), initial_track_count + 1);

  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->track_count (), initial_track_count);

  undo_stack_->redo ();
  ASSERT_EQ (track_collection_->track_count (), initial_track_count + 1);
  const auto * midi_track = qobject_cast<structure::tracks::MidiTrack *> (
    track_collection_->tracks ()[track_collection_->track_count () - 1].get ());
  const auto clips =
    midi_track->lanes ()
      ->at (0)
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_sorted_children_view ();
  ASSERT_EQ (clips.size (), 1);
  const auto notes =
    (*clips.begin ())
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiNote>::get_sorted_children_view ();
  EXPECT_FALSE (notes.empty ());
}

// Test importing multiple files
TEST_F (FileImporterTest, ImportMultipleFiles)
{
  const auto initial_track_count = track_collection_->track_count ();
  const auto initial_undo_count = undo_stack_->count ();

  QStringList files;
  files.append (utils::Utf8String::from_path (audio_file_path_).to_qstring ());
  files.append (utils::Utf8String::from_path (midi_file_path_).to_qstring ());

  file_importer_->importFiles (files, 100.0, nullptr);

  // Should have created two new tracks (one audio, one MIDI)
  EXPECT_EQ (track_collection_->track_count (), initial_track_count + 2);

  // Should have pushed commands to undo stack
  EXPECT_GT (undo_stack_->count (), initial_undo_count);

  // Verify we have both audio and MIDI tracks
  bool has_audio_track = false;
  bool has_midi_track = false;

  for (const auto &track_ref : track_collection_->tracks ())
    {
      if (track_ref.get ()->type () == structure::tracks::Track::Type::Audio)
        {
          has_audio_track = true;
        }
      else if (track_ref.get ()->type () == structure::tracks::Track::Type::Midi)
        {
          has_midi_track = true;
        }
    }

  EXPECT_TRUE (has_audio_track);
  EXPECT_TRUE (has_midi_track);
}

// Test importing empty file list
TEST_F (FileImporterTest, ImportEmptyFileList)
{
  const auto initial_track_count = track_collection_->track_count ();
  const auto initial_undo_count = undo_stack_->count ();

  QStringList empty_files;
  file_importer_->importFiles (empty_files, 0.0, nullptr);

  // Should not have created any tracks
  EXPECT_EQ (track_collection_->track_count (), initial_track_count);

  // Should not have pushed any commands
  EXPECT_EQ (undo_stack_->count (), initial_undo_count);
}

// Test importing unsupported files
TEST_F (FileImporterTest, ImportUnsupportedFiles)
{
  const auto initial_track_count = track_collection_->track_count ();
  const auto initial_undo_count = undo_stack_->count ();

  QStringList files;
  files.append (
    utils::Utf8String::from_path (unsupported_file_path_).to_qstring ());

  file_importer_->importFiles (files, 0.0, nullptr);

  // Should not have created any tracks for unsupported files
  EXPECT_EQ (track_collection_->track_count (), initial_track_count);

  // Empty macros are discarded, so nothing should be pushed when no files
  // are processed
  EXPECT_EQ (undo_stack_->count (), initial_undo_count);
}

// Test undo/redo functionality for file import
TEST_F (FileImporterTest, UndoRedoFileImport)
{
  const auto initial_track_count = track_collection_->track_count ();

  QStringList files;
  files.append (utils::Utf8String::from_path (audio_file_path_).to_qstring ());

  file_importer_->importFiles (files, 0.0, nullptr);

  // Track should be created after import
  EXPECT_EQ (track_collection_->track_count (), initial_track_count + 1);

  // Undo should remove the track
  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->track_count (), initial_track_count);

  // Redo should add the track back
  undo_stack_->redo ();
  EXPECT_EQ (track_collection_->track_count (), initial_track_count + 1);
}

// Test importing file to clip slot
TEST_F (FileImporterTest, ImportFileToClipSlot)
{
  // Create a test track and scene
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);
  auto * audio_track = track_result.value<structure::tracks::AudioTrack *> ();

  auto scene = utils::make_qobject_unique<structure::scenes::Scene> (
    registry_, *track_collection_);
  const auto &clip_slot = scene->clipSlots ()->clip_slots ()[0];

  const auto initial_undo_count = undo_stack_->count ();

  // Import audio file to clip slot
  file_importer_->importFileToClipSlot (
    utils::Utf8String::from_path (audio_file_path_).to_qstring (), audio_track,
    scene.get (), clip_slot.get ());

  // Should have pushed commands to undo stack
  EXPECT_GT (undo_stack_->count (), initial_undo_count);

  // Clip slot should now have a clip
  EXPECT_NE (clip_slot.get ()->clip (), nullptr);
}

// Test importing MIDI file to clip slot
TEST_F (FileImporterTest, ImportMidiFileToClipSlot)
{
  // Create a test track and scene
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);
  auto * midi_track = track_result.value<structure::tracks::MidiTrack *> ();

  auto scene = utils::make_qobject_unique<structure::scenes::Scene> (
    registry_, *track_collection_);
  const auto &clip_slot = scene->clipSlots ()->clip_slots ()[0];

  const auto initial_undo_count = undo_stack_->count ();

  // Import MIDI file to clip slot
  file_importer_->importFileToClipSlot (
    utils::Utf8String::from_path (midi_file_path_).to_qstring (), midi_track,
    scene.get (), clip_slot.get ());

  // Should have pushed commands to undo stack
  EXPECT_GT (undo_stack_->count (), initial_undo_count);

  // Clip slot should now have a clip with the mock file's note
  const auto * midi_clip = qobject_cast<structure::arrangement::MidiClip *> (
    clip_slot.get ()->clip ());
  ASSERT_NE (midi_clip, nullptr);
  const auto notes = midi_clip->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiNote>::get_sorted_children_view ();
  ASSERT_EQ (notes.size (), 1);
  EXPECT_EQ ((*notes.begin ())->pitch (), 60);
  EXPECT_EQ ((*notes.begin ())->velocity (), 64);
}

// Importing without a clip slot is refused
TEST_F (FileImporterTest, ImportToMissingClipSlotIsRefused)
{
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);
  auto * midi_track = track_result.value<structure::tracks::MidiTrack *> ();

  auto scene = utils::make_qobject_unique<structure::scenes::Scene> (
    registry_, *track_collection_);

  QStringList failed_files;
  QObject::connect (
    file_importer_.get (), &FileImporter::importFailed, file_importer_.get (),
    [&failed_files] (const QString &filePath, const QString &) {
      failed_files.append (filePath);
    });

  file_importer_->importFileToClipSlot (
    utils::Utf8String::from_path (midi_file_path_).to_qstring (), midi_track,
    scene.get (), nullptr);

  EXPECT_EQ (failed_files.size (), 1);
}

// Importing an audio file into a MIDI track's clip slot is refused
TEST_F (FileImporterTest, ImportAudioToMidiClipSlotIsRefused)
{
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);
  auto * midi_track = track_result.value<structure::tracks::MidiTrack *> ();

  auto scene = utils::make_qobject_unique<structure::scenes::Scene> (
    registry_, *track_collection_);
  const auto &clip_slot = scene->clipSlots ()->clip_slots ()[0];

  QStringList failed_files;
  QObject::connect (
    file_importer_.get (), &FileImporter::importFailed, file_importer_.get (),
    [&failed_files] (const QString &filePath, const QString &) {
      failed_files.append (filePath);
    });

  file_importer_->importFileToClipSlot (
    utils::Utf8String::from_path (audio_file_path_).to_qstring (), midi_track,
    scene.get (), clip_slot.get ());

  EXPECT_EQ (failed_files.size (), 1);
  EXPECT_EQ (clip_slot.get ()->clip (), nullptr);
}

// Importing a MIDI file into an audio track's clip slot is refused
TEST_F (FileImporterTest, ImportMidiToAudioClipSlotIsRefused)
{
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);
  auto * audio_track = track_result.value<structure::tracks::AudioTrack *> ();

  auto scene = utils::make_qobject_unique<structure::scenes::Scene> (
    registry_, *track_collection_);
  const auto &clip_slot = scene->clipSlots ()->clip_slots ()[0];

  QStringList failed_files;
  QObject::connect (
    file_importer_.get (), &FileImporter::importFailed, file_importer_.get (),
    [&failed_files] (const QString &filePath, const QString &) {
      failed_files.append (filePath);
    });

  file_importer_->importFileToClipSlot (
    utils::Utf8String::from_path (midi_file_path_).to_qstring (), audio_track,
    scene.get (), clip_slot.get ());

  EXPECT_EQ (failed_files.size (), 1);
  EXPECT_EQ (clip_slot.get ()->clip (), nullptr);
}

// Test importing unsupported file to clip slot
TEST_F (FileImporterTest, ImportUnsupportedFileToClipSlot)
{
  // Create a test track and scene
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);
  auto * audio_track = track_result.value<structure::tracks::AudioTrack *> ();

  auto scene = utils::make_qobject_unique<structure::scenes::Scene> (
    registry_, *track_collection_);
  const auto &clip_slot = scene->clipSlots ()->clip_slots ()[0];

  const auto initial_undo_count = undo_stack_->count ();

  // Import unsupported file to clip slot
  file_importer_->importFileToClipSlot (
    utils::Utf8String::from_path (unsupported_file_path_).to_qstring (),
    audio_track, scene.get (), clip_slot.get ());

  // Should not have pushed any commands for unsupported file
  EXPECT_EQ (undo_stack_->count (), initial_undo_count);

  // Clip slot should still be empty
  EXPECT_EQ (clip_slot.get ()->clip (), nullptr);
}

// Test file type detection priority
TEST_F (FileImporterTest, FileTypeDetectionPriority)
{
  // MIDI files should be detected as MIDI even if they could be interpreted as
  // audio
  auto midi_file_qstr =
    utils::Utf8String::from_path (midi_file_path_).to_qstring ();
  EXPECT_EQ (
    file_importer_->getFileType (midi_file_qstr), FileImporter::FileType::Midi);
  EXPECT_TRUE (file_importer_->isMidiFile (midi_file_qstr));
  EXPECT_FALSE (file_importer_->isAudioFile (midi_file_qstr));
}

// Test with various audio file extensions
TEST_F (FileImporterTest, VariousAudioFileExtensions)
{
  // Test that the audio format manager can handle different extensions
  // Since we created a WAV file, test that it's properly detected
  auto audio_file_qstr =
    utils::Utf8String::from_path (audio_file_path_).to_qstring ();
  EXPECT_TRUE (file_importer_->isAudioFile (audio_file_qstr));
  EXPECT_EQ (
    file_importer_->getFileType (audio_file_qstr),
    FileImporter::FileType::Audio);
}

// Test macro command wrapping
TEST_F (FileImporterTest, MacroCommandWrapping)
{
  const auto initial_undo_count = undo_stack_->count ();

  QStringList files;
  files.append (utils::Utf8String::from_path (audio_file_path_).to_qstring ());
  files.append (utils::Utf8String::from_path (midi_file_path_).to_qstring ());

  file_importer_->importFiles (files, 0.0, nullptr);

  // Should have wrapped all operations in a single macro
  // The exact count depends on implementation, but should be > initial
  EXPECT_GT (undo_stack_->count (), initial_undo_count);

  // Single undo should undo all import operations
  undo_stack_->undo ();
  EXPECT_EQ (
    track_collection_->track_count (), 4); // Only singleton tracks remain

  // Single redo should redo all import operations
  undo_stack_->redo ();
  EXPECT_EQ (
    track_collection_->track_count (), 6); // Singleton + 2 imported tracks
}

// The probed duration of a MIDI file is the furthest note end, in ticks
TEST_F (FileImporterTest, GetFileDurationTicksForMidiFile)
{
  // The mock MIDI file holds one note from tick 0 to file tick 96 at 96
  // PPQN, which is 960 Zrythm ticks
  EXPECT_DOUBLE_EQ (
    file_importer_->getFileDurationTicks (
      utils::Utf8String::from_path (midi_file_path_).to_qstring ()),
    960.0);
}

// The probed duration of an audio file is its length converted to ticks
// at the tempo map's base tempo
TEST_F (FileImporterTest, GetFileDurationTicksForAudioFile)
{
  // The mock WAV file holds 2048 samples at 44100 Hz; at 120 BPM and 960
  // PPQN one second is 1920 ticks
  const auto expected_ticks = 2048.0 / 44100.0 * 1920.0;
  EXPECT_NEAR (
    file_importer_->getFileDurationTicks (
      utils::Utf8String::from_path (audio_file_path_).to_qstring ()),
    expected_ticks, 1e-9);
}

// Durations that cannot be determined are reported as negative values
TEST_F (FileImporterTest, GetFileDurationTicksForUnusableFiles)
{
  EXPECT_DOUBLE_EQ (
    file_importer_->getFileDurationTicks (
      utils::Utf8String::from_path (unsupported_file_path_).to_qstring ()),
    -1.0);
  EXPECT_DOUBLE_EQ (
    file_importer_->getFileDurationTicks (
      utils::Utf8String::from_path (temp_dir_path_ / "missing.mid").to_qstring ()),
    -1.0);
}

// Multi-track MIDI files stack their clips in consecutive lanes starting
// at the target lane
TEST_F (FileImporterTest, ImportFilesToLaneStacksMidiClipsFromTargetLane)
{
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);
  auto * midi_track = track_result.value<structure::tracks::MidiTrack *> ();

  const auto start_ticks = 100.0;

  QStringList files;
  files.append (
    utils::Utf8String::from_path (
      std::filesystem::path (TEST_MIDI_FILES_DIR)
      / "format_1_two_tracks_with_data.mid")
      .to_qstring ());

  file_importer_->importFilesToLane (
    files, start_ticks, midi_track, midi_track->lanes ()->getFirstLane ());

  for (const auto lane_index : std::views::iota (0uz, 2uz))
    {
      const auto clips =
        midi_track->lanes ()
          ->at (lane_index)
          ->structure::arrangement::ArrangerObjectOwner<
            structure::arrangement::MidiClip>::get_sorted_children_view ();
      ASSERT_EQ (clips.size (), 1);
      EXPECT_DOUBLE_EQ ((*clips.begin ())->position ()->ticks (), start_ticks);
    }
}

// Audio files are imported into the target lane
TEST_F (FileImporterTest, ImportFilesToLaneImportsAudioIntoTargetLane)
{
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);
  auto * audio_track = track_result.value<structure::tracks::AudioTrack *> ();

  const auto start_ticks = 50.0;

  QStringList files;
  files.append (utils::Utf8String::from_path (audio_file_path_).to_qstring ());

  file_importer_->importFilesToLane (
    files, start_ticks, audio_track, audio_track->lanes ()->getFirstLane ());

  const auto clips =
    audio_track->lanes ()
      ->getFirstLane ()
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::AudioClip>::get_sorted_children_view ();
  ASSERT_EQ (clips.size (), 1);
  EXPECT_DOUBLE_EQ ((*clips.begin ())->position ()->ticks (), start_ticks);
}

// Files dropped together onto a track stack their clips in consecutive
// lanes, all starting at the drop position
TEST_F (FileImporterTest, ImportFilesToLaneStacksFilesInLanes)
{
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);
  auto * midi_track = track_result.value<structure::tracks::MidiTrack *> ();

  const auto dir =
    utils::Utf8String::from_qstring (temp_dir_->path ()).to_path ();
  const dsp::MidiFile::NoteTrack note{
    .notes = { { .pitch = 60,
                 .velocity = 100,
                 .midi_channel = 0,
                 .start_ticks = units::ticks (0.0),
                 .end_ticks = units::ticks (960.0) } },
  };
  const std::array<const dsp::MidiFile::NoteTrack, 2> two_tracks{ note, note };
  const std::array<const dsp::MidiFile::NoteTrack, 1> one_track{ note };
  const auto two_tracks_path = dir / "two_tracks.mid";
  const auto one_track_path = dir / "one_track.mid";
  dsp::MidiFile::write_to_file (
    two_tracks_path, dsp::MidiFile::Format::MIDI1, units::bpm (120.0),
    dsp::TimeSignature{ 4, 4 }, two_tracks);
  dsp::MidiFile::write_to_file (
    one_track_path, dsp::MidiFile::Format::MIDI0, units::bpm (120.0),
    dsp::TimeSignature{ 4, 4 }, one_track);

  const auto  start_ticks = 100.0;
  QStringList files;
  files.append (utils::Utf8String::from_path (two_tracks_path).to_qstring ());
  files.append (utils::Utf8String::from_path (one_track_path).to_qstring ());

  file_importer_->importFilesToLane (
    files, start_ticks, midi_track, midi_track->lanes ()->getFirstLane ());

  for (const auto lane_index : std::views::iota (0uz, 3uz))
    {
      const auto clips =
        midi_track->lanes ()
          ->at (lane_index)
          ->structure::arrangement::ArrangerObjectOwner<
            structure::arrangement::MidiClip>::get_sorted_children_view ();
      ASSERT_EQ (clips.size (), 1);
      EXPECT_DOUBLE_EQ ((*clips.begin ())->position ()->ticks (), start_ticks)
        << "lane " << lane_index;
    }
}

// An import into a lane beyond the first survives being undone and
// redone, including the lane trimming that undo performs
TEST_F (FileImporterTest, UndoRedoImportIntoHigherLane)
{
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);
  auto * midi_track = track_result.value<structure::tracks::MidiTrack *> ();
  midi_track->lanes ()->create_missing_lanes (1);

  const auto clips_in_lane = [midi_track] (size_t lane_index) {
    if (lane_index >= midi_track->lanes ()->size ())
      {
        return size_t{ 0 };
      }
    return midi_track->lanes ()
      ->at (lane_index)
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_sorted_children_view ()
      .size ();
  };

  QStringList files;
  files.append (utils::Utf8String::from_path (midi_file_path_).to_qstring ());
  file_importer_->importFilesToLane (
    files, 0.0, midi_track, midi_track->lanes ()->at (1));
  EXPECT_EQ (clips_in_lane (1), 1);

  undo_stack_->undo ();
  EXPECT_EQ (clips_in_lane (1), 0);

  undo_stack_->redo ();
  EXPECT_EQ (clips_in_lane (1), 1);
}

} // namespace zrythm::actions
