// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/file_importer.h"
#include "actions/track_creator.h"
#include "structure/scenes/scene.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"
#include "utils/io_utils.h"

#include <QTemporaryDir>
#include <QTemporaryFile>

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
    // Set up dependencies
    track_registry_ = std::make_unique<structure::tracks::TrackRegistry> ();

    // Create singleton tracks
    singleton_tracks_ = std::make_unique<structure::tracks::SingletonTracks> ();

    // Create track collection
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (*track_registry_);

    // Create track routing
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (*track_registry_);

    // Create track factory with dependencies
    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,
      file_audio_source_registry_,
      plugin_registry_,
      port_registry_,
      param_registry_,
      obj_registry_,
      *track_registry_,
      transport_,
      soloed_tracks_exist_getter_,
    };
    track_factory_ =
      std::make_unique<structure::tracks::TrackFactory> (factory_deps);

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
      tempo_map_, utils::NoteLength::Note_1_4, [] () { return 100.0; });
    snap_grid_editor = std::make_unique<dsp::SnapGrid> (
      tempo_map_, utils::NoteLength::Note_1_4, [] () { return 50.0; });

    // Create arranger object factory with proper dependencies
    structure::arrangement::ArrangerObjectFactory::Dependencies obj_factory_deps{
      .tempo_map_ = tempo_map_,
      .object_registry_ = obj_registry_,
      .file_audio_source_registry_ = file_audio_source_registry_,
      .musical_mode_getter_ = [] () { return true; },
      .last_timeline_obj_len_provider_ = [] () { return 100.0; },
      .last_editor_obj_len_provider_ = [] () { return 50.0; },
      .automation_curve_algorithm_provider_ =
        [] () { return dsp::CurveOptions::Algorithm::Exponent; }
    };

    arranger_object_factory = std::make_unique<
      structure::arrangement::ArrangerObjectFactory> (
      obj_factory_deps, [] () { return units::sample_rate (44100); },
      [] () { return 120.0; });

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
      *undo_stack_, *arranger_object_creator_, *track_creator_);

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
    track_registry_.reset ();
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

  void createMockWavFile (const fs::path &path)
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

  void createMockMidiFile (const fs::path &path)
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
  dsp::TempoMap                   tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper            tempo_map_wrapper_{ tempo_map_ };
  dsp::FileAudioSourceRegistry    file_audio_source_registry_;
  plugins::PluginRegistry         plugin_registry_;
  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  structure::arrangement::ArrangerObjectRegistry obj_registry_;
  dsp::graph_test::MockTransport                 transport_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

  std::unique_ptr<structure::tracks::TrackRegistry>   track_registry_;
  std::unique_ptr<structure::tracks::SingletonTracks> singleton_tracks_;
  std::unique_ptr<structure::tracks::TrackCollection> track_collection_;
  std::unique_ptr<structure::tracks::TrackRouting>    track_routing_;
  std::unique_ptr<structure::tracks::TrackFactory>    track_factory_;
  std::unique_ptr<undo::UndoStack>                    undo_stack_;
  std::unique_ptr<dsp::SnapGrid>                      snap_grid_timeline;
  std::unique_ptr<dsp::SnapGrid>                      snap_grid_editor;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory>
                                         arranger_object_factory;
  std::unique_ptr<TrackCreator>          track_creator_;
  std::unique_ptr<ArrangerObjectCreator> arranger_object_creator_;
  std::unique_ptr<FileImporter>          file_importer_;

  // Test files
  std::unique_ptr<QTemporaryDir> temp_dir_;
  fs::path                       temp_dir_path_;
  fs::path                       audio_file_path_;
  fs::path                       midi_file_path_;
  fs::path                       unsupported_file_path_;
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
  EXPECT_EQ (track_collection_->track_count (), initial_track_count + 1);

  // Should have pushed commands to undo stack
  EXPECT_GT (undo_stack_->count (), initial_undo_count);
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
      if (
        track_ref.get_object_base ()->type ()
        == structure::tracks::Track::Type::Audio)
        {
          has_audio_track = true;
        }
      else if (
        track_ref.get_object_base ()->type ()
        == structure::tracks::Track::Type::Midi)
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

  // Should still push macro command even if no files are processed
  EXPECT_GT (undo_stack_->count (), initial_undo_count);
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
    obj_registry_, *track_collection_);
  const auto &clip_slot = scene->clipSlots ()->clip_slots ()[0];

  const auto initial_undo_count = undo_stack_->count ();

  // Import audio file to clip slot
  file_importer_->importFileToClipSlot (
    utils::Utf8String::from_path (audio_file_path_).to_qstring (), audio_track,
    scene.get (), clip_slot.get ());

  // Should have pushed commands to undo stack
  EXPECT_GT (undo_stack_->count (), initial_undo_count);

  // Clip slot should now have a region
  EXPECT_NE (clip_slot->region (), nullptr);
}

// TODO: unimplememented
#if 0
// Test importing MIDI file to clip slot
TEST_F (FileImporterTest, ImportMidiFileToClipSlot)
{
  // Create a test track and scene
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);
  auto * midi_track = track_result.value<structure::tracks::MidiTrack *> ();

  auto scene = utils::make_qobject_unique<structure::scenes::Scene> (
    obj_registry_, *track_collection_);
  const auto &clip_slot = scene->clipSlots ()->clip_slots ()[0];

  const auto initial_undo_count = undo_stack_->count ();

  // Import MIDI file to clip slot
  file_importer_->importFileToClipSlot (
    utils::Utf8String::from_path (midi_file_path_).to_qstring (), midi_track,
    scene.get (), clip_slot.get ());

  // Should have pushed commands to undo stack
  EXPECT_GT (undo_stack_->count (), initial_undo_count);

  // Clip slot should now have a region
  EXPECT_NE (clip_slot->region (), nullptr);
}
#endif

// Test importing unsupported file to clip slot
TEST_F (FileImporterTest, ImportUnsupportedFileToClipSlot)
{
  // Create a test track and scene
  auto track_result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);
  auto * audio_track = track_result.value<structure::tracks::AudioTrack *> ();

  auto scene = utils::make_qobject_unique<structure::scenes::Scene> (
    obj_registry_, *track_collection_);
  const auto &clip_slot = scene->clipSlots ()->clip_slots ()[0];

  const auto initial_undo_count = undo_stack_->count ();

  // Import unsupported file to clip slot
  file_importer_->importFileToClipSlot (
    utils::Utf8String::from_path (unsupported_file_path_).to_qstring (),
    audio_track, scene.get (), clip_slot.get ());

  // Should not have pushed any commands for unsupported file
  EXPECT_EQ (undo_stack_->count (), initial_undo_count);

  // Clip slot should still be empty
  EXPECT_EQ (clip_slot->region (), nullptr);
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

} // namespace zrythm::actions
