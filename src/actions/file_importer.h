// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <vector>

#include "actions/arranger_object_creator.h"
#include "actions/track_creator.h"
#include "dsp/midi_file.h"
#include "structure/scenes/scene.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace zrythm::actions
{

class FileImporter : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("One instance per project")

public:
  explicit FileImporter (
    undo::UndoStack                    &undo_stack,
    ArrangerObjectCreator              &arranger_object_creator,
    TrackCreator                       &track_creator,
    structure::tracks::TrackCollection &track_collection,
    const dsp::TempoMap                &tempo_map,
    QObject *                           parent = nullptr);

  /**
   * @brief Enumeration for supported file types.
   */
  enum class FileType
  {
    Audio,
    Midi,
    Unsupported
  };

  Q_ENUM (FileType)

  /**
   * @brief Imports the given files.
   *
   * Each file is imported into @p track, when given, otherwise into a newly
   * created track. Newly created tracks are named after the imported file.
   * All files are imported in a single undoable macro.
   *
   * Files that cannot be imported are reported through importFailed().
   *
   * @param filePaths Files to import.
   * @param startTicks Timeline position to import the contents at.
   * @param track Track to import into, or null to create a new track.
   */
  Q_INVOKABLE void importFiles (
    const QStringList         &filePaths,
    double                     startTicks,
    structure::tracks::Track * track);

  /**
   * @brief Imports the given files, targeting a specific lane.
   *
   * Like importFiles(), but audio files land in @p lane and multi-track
   * MIDI files stack their clips in consecutive lanes starting at @p lane.
   * @p lane is only used when @p track is given and accepts the file type.
   *
   * @param filePaths Files to import.
   * @param startTicks Timeline position to import the contents at.
   * @param track Track to import into, or null to create a new track.
   * @param lane Lane to import into, or null for the first lane.
   */
  Q_INVOKABLE void importFilesToLane (
    const QStringList             &filePaths,
    double                         startTicks,
    structure::tracks::Track *     track,
    structure::tracks::TrackLane * lane);

  Q_INVOKABLE void importFileToClipSlot (
    const QString                &filePath,
    structure::tracks::Track *    track,
    structure::scenes::Scene *    scene,
    structure::scenes::ClipSlot * clipSlot);

Q_SIGNALS:
  /** Emitted for each file that was refused or failed to import. */
  void importFailed (const QString &filePath, const QString &reason);

public:
  /**
   * @brief Determines the type of a file.
   *
   * @param filePath Path to the file to check.
   * @return FileType indicating whether the file is audio, MIDI, or unsupported.
   */
  Q_INVOKABLE FileType getFileType (const QString &filePath) const;

  /**
   * @brief Checks if a file is an audio file.
   *
   * @param filePath Path to the file to check.
   * @return true if the file is a supported audio format, false otherwise.
   */
  Q_INVOKABLE bool isAudioFile (const QString &filePath) const;

  /**
   * @brief Checks if a file is a MIDI file.
   *
   * @param filePath Path to the file to check.
   * @return true if the file is a valid MIDI file, false otherwise.
   */
  Q_INVOKABLE bool isMidiFile (const QString &filePath) const;

  /**
   * @brief Returns the duration of a file in timeline ticks.
   *
   * Audio durations are converted at the tempo map's current tempo; MIDI
   * durations are the furthest note end.
   *
   * @param filePath Path to the file to probe.
   * @return The duration in ticks, or a negative value when it cannot be
   * determined.
   */
  Q_INVOKABLE double getFileDurationTicks (const QString &filePath) const;

private:
  /**
   * @brief Imports an audio file.
   *
   * @param lane_index Index of the lane to place the clip in, or none for
   * the first lane.
   * @return The number of clips created.
   */
  size_t import_audio_file (
    const std::filesystem::path &filePath,
    double                       startTicks,
    structure::tracks::Track *   track,
    std::optional<size_t>        lane_index);

  /**
   * @brief Imports a MIDI file, placing one clip per note track in
   * consecutive lanes starting at @p lane_index.
   *
   * @return The number of clips created.
   */
  size_t import_midi_file (
    const std::filesystem::path &filePath,
    double                       startTicks,
    structure::tracks::Track *   track,
    std::optional<size_t>        lane_index);

  /**
   * @brief Parses the note content of a MIDI file.
   *
   * @return The note tracks (possibly empty when the file contains no
   * notes), or an error reason when the file cannot be read.
   */
  [[nodiscard]] static std::
    expected<std::vector<dsp::MidiFile::NoteTrack>, utils::Utf8String>
    parse_midi_note_tracks (const std::filesystem::path &filePath);

  /** Pushes a rename of @p track to @p filePath's basename. */
  void rename_track_to_file_basename (
    structure::tracks::Track    &track,
    const std::filesystem::path &filePath);

  ::zrythm::actions::TrackCreator          &track_creator_;
  ::zrythm::actions::ArrangerObjectCreator &arranger_object_creator_;
  structure::tracks::TrackCollection       &track_collection_;
  undo::UndoStack                          &undo_stack_;
  const dsp::TempoMap                      &tempo_map_;

  /** Audio format manager for detecting audio files. */
  mutable juce::AudioFormatManager audio_format_manager_;
};
}
