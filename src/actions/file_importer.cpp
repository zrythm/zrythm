// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/file_importer.h"

namespace zrythm::actions
{
FileImporter::FileImporter (
  undo::UndoStack                          &undo_stack,
  ::zrythm::actions::ArrangerObjectCreator &arranger_object_creator,
  ::zrythm::actions::TrackCreator          &track_creator,
  QObject *                                 parent)
    : QObject (parent), track_creator_ (track_creator),
      arranger_object_creator_ (arranger_object_creator), undo_stack_ (undo_stack)
{
  // Initialize the audio format manager with basic formats
  audio_format_manager_.registerBasicFormats ();
}

void
FileImporter::importFiles (
  const QStringList         &filePaths,
  double                     startTicks,
  structure::tracks::Track * track) const
{
  z_debug ("Importing {} files: {}", filePaths.size (), filePaths);

  if (filePaths.empty ())
    {
      return;
    }

  undo_stack_.beginMacro (QString::fromUtf8 ("Import Files"));
  for (const auto &filepath : filePaths)
    {
      if (isAudioFile (filepath))
        {
          auto track_qvar = track_creator_.addEmptyTrackFromType (
            structure::tracks::Track::Type::Audio);
          auto * audio_track =
            track_qvar.value<structure::tracks::AudioTrack *> ();
          arranger_object_creator_.addAudioRegionFromFile (
            audio_track, audio_track->lanes ()->getFirstLane (), filepath, 0);
        }
      else if (isMidiFile (filepath))
        {
          auto track_qvar = track_creator_.addEmptyTrackFromType (
            structure::tracks::Track::Type::Midi);
          auto * midi_track =
            track_qvar.value<structure::tracks::MidiTrack *> ();
          arranger_object_creator_.addMidiRegionFromMidiFile (
            midi_track, midi_track->lanes ()->getFirstLane (), filepath, 0, 0);
        }
    }
  undo_stack_.endMacro ();
}

void
FileImporter::importFileToClipSlot (
  const QString                &filePath,
  structure::tracks::Track *    track,
  structure::scenes::Scene *    scene,
  structure::scenes::ClipSlot * clipSlot) const
{
  if (isAudioFile (filePath))
    {
      arranger_object_creator_.addAudioRegionToClipSlotFromFile (
        track, clipSlot, filePath);
    }
  else if (isMidiFile (filePath))
    {
      arranger_object_creator_.addMidiRegionToClipSlotFromFile (
        track, clipSlot, filePath);
    }
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
  auto file = utils::Utf8String::from_qstring (filePath).to_juce_file ();

  if (!file.existsAsFile ())
    {
      return false;
    }

  // Try to read the file as a MIDI file
  juce::FileInputStream stream (file);
  if (!stream.openedOk ())
    {
      return false;
    }

  juce::MidiFile midiFile;
  return midiFile.readFrom (stream);
}
}
