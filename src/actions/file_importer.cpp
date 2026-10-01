// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "actions/file_importer.h"
#include "commands/rename_track_command.h"
#include "utils/exceptions.h"

namespace zrythm::actions
{
namespace
{
QString
path_to_qstring (const std::filesystem::path &path)
{
  return utils::Utf8String::from_path (path).to_qstring ();
}
}

FileImporter::FileImporter (
  undo::UndoStack                          &undo_stack,
  ::zrythm::actions::ArrangerObjectCreator &arranger_object_creator,
  ::zrythm::actions::TrackCreator          &track_creator,
  structure::tracks::TrackCollection       &track_collection,
  QObject *                                 parent)
    : QObject (parent), track_creator_ (track_creator),
      arranger_object_creator_ (arranger_object_creator),
      track_collection_ (track_collection), undo_stack_ (undo_stack)
{
  // Initialize the audio format manager with basic formats
  audio_format_manager_.registerBasicFormats ();
}

void
FileImporter::importFiles (
  const QStringList         &filePaths,
  double                     startTicks,
  structure::tracks::Track * track)
{
  z_debug ("Importing {} files: {}", filePaths.size (), filePaths);

  if (filePaths.empty ())
    {
      return;
    }

  undo::UndoStack::ScopedMacro macro (
    undo_stack_, QString::fromUtf8 ("Import Files"));
  for (const auto &filepath : filePaths)
    {
      const auto file_path =
        utils::Utf8String::from_qstring (filepath).to_path ();
      try
        {
          if (isMidiFile (filepath))
            {
              import_midi_file (file_path, startTicks, track, nullptr);
            }
          else if (isAudioFile (filepath))
            {
              import_audio_file (file_path, startTicks, track, nullptr);
            }
          else
            {
              Q_EMIT importFailed (filepath, tr ("Unsupported file type"));
            }
        }
      catch (const ZrythmException &e)
        {
          Q_EMIT importFailed (
            filepath,
            tr ("Failed to import file (%1)")
              .arg (e.what_string ().to_qstring ()));
        }
      catch (const std::exception &e)
        {
          Q_EMIT importFailed (
            filepath,
            tr ("Failed to import file (%1)")
              .arg (
                utils::Utf8String::from_utf8_encoded_string (e.what ())
                  .to_qstring ()));
        }
    }
}

void
FileImporter::importFileToClipSlot (
  const QString                &filePath,
  structure::tracks::Track *    track,
  structure::scenes::Scene *    scene,
  structure::scenes::ClipSlot * clipSlot)
{
  if (track == nullptr || scene == nullptr || clipSlot == nullptr)
    {
      Q_EMIT importFailed (filePath, tr ("No clip slot found for the track"));
      return;
    }

  const auto file_path = utils::Utf8String::from_qstring (filePath).to_path ();
  try
    {
      if (isMidiFile (filePath))
        {
          const auto track_type = track->type ();
          if (
            track_type != structure::tracks::Track::Type::Midi
            && track_type != structure::tracks::Track::Type::Instrument)
            {
              Q_EMIT importFailed (
                filePath,
                tr (
                  "MIDI files can only be imported into MIDI or instrument tracks"));
              return;
            }

          const auto note_tracks = parse_midi_note_tracks (file_path);
          if (!note_tracks.has_value ())
            {
              Q_EMIT importFailed (
                filePath,
                tr ("Failed to read MIDI file (%1)")
                  .arg (note_tracks.error ().to_qstring ()));
              return;
            }
          if (note_tracks->empty ())
            {
              Q_EMIT importFailed (filePath, tr ("MIDI file contains no notes"));
              return;
            }

          const auto clip =
            arranger_object_creator_.add_midi_clip_to_clip_slot_from_note_tracks (
              track, clipSlot, *note_tracks);
          if (!clip)
            {
              Q_EMIT importFailed (
                filePath,
                tr ("Failed to import MIDI file (%1)")
                  .arg (clip.error ().to_qstring ()));
            }
        }
      else if (isAudioFile (filePath))
        {
          if (track->type () != structure::tracks::Track::Type::Audio)
            {
              Q_EMIT importFailed (
                filePath,
                tr ("Audio files can only be imported into audio tracks"));
              return;
            }
          try
            {
              if (
                arranger_object_creator_.addAudioClipToClipSlotFromFile (
                  track, clipSlot, filePath)
                == nullptr)
                {
                  Q_EMIT importFailed (
                    filePath, tr ("Failed to import audio file"));
                }
            }
          catch (const ZrythmException &e)
            {
              Q_EMIT importFailed (
                filePath,
                tr ("Failed to import audio file (%1)")
                  .arg (e.what_string ().to_qstring ()));
            }
        }
      else
        {
          Q_EMIT importFailed (filePath, tr ("Unsupported file type"));
        }
    }
  catch (const ZrythmException &e)
    {
      Q_EMIT importFailed (
        filePath,
        tr ("Failed to import file (%1)").arg (e.what_string ().to_qstring ()));
    }
  catch (const std::exception &e)
    {
      Q_EMIT importFailed (
        filePath,
        tr ("Failed to import file (%1)")
          .arg (
            utils::Utf8String::from_utf8_encoded_string (e.what ())
              .to_qstring ()));
    }
}

bool
FileImporter::import_audio_file (
  const std::filesystem::path   &filePath,
  double                         startTicks,
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane)
{
  auto * audio_track = qobject_cast<structure::tracks::AudioTrack *> (track);
  if (track != nullptr && audio_track == nullptr)
    {
      Q_EMIT importFailed (
        path_to_qstring (filePath),
        tr ("Audio files can only be imported into audio tracks"));
      return false;
    }

  if (audio_track == nullptr)
    {
      audio_track =
        track_creator_
          .addEmptyTrackFromType (structure::tracks::Track::Type::Audio)
          .value<structure::tracks::AudioTrack *> ();
      rename_track_to_file_basename (*audio_track, filePath);
    }

  auto * target_lane =
    lane != nullptr ? lane : audio_track->lanes ()->getFirstLane ();
  try
    {
      if (
        arranger_object_creator_.addAudioClipFromFile (
          audio_track, target_lane, path_to_qstring (filePath), startTicks)
        != nullptr)
        {
          return true;
        }
    }
  catch (const ZrythmException &e)
    {
      Q_EMIT importFailed (
        path_to_qstring (filePath),
        tr ("Failed to import audio file (%1)")
          .arg (e.what_string ().to_qstring ()));
      return false;
    }
  Q_EMIT importFailed (
    path_to_qstring (filePath), tr ("Failed to import audio file"));
  return false;
}

bool
FileImporter::import_midi_file (
  const std::filesystem::path   &filePath,
  double                         startTicks,
  structure::tracks::Track *     track,
  structure::tracks::TrackLane * lane)
{
  const auto track_type =
    track != nullptr ? track->type () : structure::tracks::Track::Type::Midi;
  const bool track_accepts_midi =
    track_type == structure::tracks::Track::Type::Midi
    || track_type == structure::tracks::Track::Type::Instrument;
  if (track != nullptr && !track_accepts_midi)
    {
      Q_EMIT importFailed (
        path_to_qstring (filePath),
        tr ("MIDI files can only be imported into MIDI or instrument tracks"));
      return false;
    }

  const auto note_tracks = parse_midi_note_tracks (filePath);
  if (!note_tracks.has_value ())
    {
      Q_EMIT importFailed (
        path_to_qstring (filePath),
        tr ("Failed to read MIDI file (%1)")
          .arg (note_tracks.error ().to_qstring ()));
      return false;
    }
  if (note_tracks->empty ())
    {
      Q_EMIT importFailed (
        path_to_qstring (filePath), tr ("MIDI file contains no notes"));
      return false;
    }

  auto * midi_track =
    track != nullptr
      ? track
      : track_creator_
          .addEmptyTrackFromType (structure::tracks::Track::Type::Midi)
          .value<structure::tracks::Track *> ();
  if (track == nullptr)
    {
      rename_track_to_file_basename (*midi_track, filePath);
    }

  auto * lanes = midi_track->lanes ();
  size_t first_lane_index = 0;
  if (lane != nullptr)
    {
      const auto lane_index = lanes->indexOfLane (lane);
      if (!lane_index.has_value ())
        {
          Q_EMIT importFailed (
            path_to_qstring (filePath),
            tr ("The track lane could not be found for importing"));
          return false;
        }
      first_lane_index = *lane_index;
    }

  for (const auto i : std::views::iota (0uz, note_tracks->size ()))
    {
      lanes->create_missing_lanes (first_lane_index + i);
      const auto clip = arranger_object_creator_.add_midi_clip_from_note_tracks (
        midi_track, lanes->at (first_lane_index + i), startTicks,
        std::span<const dsp::MidiFile::NoteTrack>{ &(*note_tracks)[i], 1 });
      if (!clip)
        {
          Q_EMIT importFailed (
            path_to_qstring (filePath),
            tr ("Failed to import MIDI track %1 (%2)")
              .arg (static_cast<int> (i) + 1)
              .arg (clip.error ().to_qstring ()));
          return false;
        }
    }
  return true;
}

std::expected<std::vector<dsp::MidiFile::NoteTrack>, utils::Utf8String>
FileImporter::parse_midi_note_tracks (const std::filesystem::path &filePath)
{
  try
    {
      dsp::MidiFile                         midi_file{ filePath };
      std::vector<dsp::MidiFile::NoteTrack> note_tracks;
      for (const auto i : std::views::iota (0, midi_file.num_note_tracks ()))
        {
          note_tracks.push_back (midi_file.read_note_track (i));
        }
      return note_tracks;
    }
  catch (const ZrythmException &e)
    {
      return std::unexpected (e.what_string ());
    }
}

void
FileImporter::rename_track_to_file_basename (
  structure::tracks::Track    &track,
  const std::filesystem::path &filePath)
{
  const auto base_name = utils::Utf8String::from_path (filePath.stem ());
  const auto unique_name =
    track_collection_.get_unique_name_for_track (track.get_uuid (), base_name);
  undo_stack_.push (
    new commands::RenameTrackCommand (track, unique_name.to_qstring ()));
}

FileImporter::FileType
FileImporter::getFileType (const QString &filePath) const
{
  if (isMidiFile (filePath))
    {
      return FileType::Midi;
    }

  if (isAudioFile (filePath))
    {
      return FileType::Audio;
    }

  return FileType::Unsupported;
}

bool
FileImporter::isAudioFile (const QString &filePath) const
{
  auto file = utils::Utf8String::from_qstring (filePath).to_juce_file ();

  if (!file.existsAsFile ())
    {
      return false;
    }

  // Try to create an audio reader for the file
  const auto reader = std::unique_ptr<juce::AudioFormatReader> (
    audio_format_manager_.createReaderFor (file));

  return reader != nullptr;
}

bool
FileImporter::isMidiFile (const QString &filePath) const
{
  return dsp::MidiFile::is_midi_file (
    utils::Utf8String::from_qstring (filePath).to_path ());
}
}
