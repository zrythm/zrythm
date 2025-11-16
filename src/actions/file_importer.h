// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "actions/arranger_object_creator.h"
#include "actions/track_creator.h"
#include "structure/scenes/scene.h"

#include <juce_wrapper.h>

namespace zrythm::actions
{

class FileImporter : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("One instance per project")

public:
  explicit FileImporter (
    undo::UndoStack                          &undo_stack,
    ::zrythm::actions::ArrangerObjectCreator &arranger_object_creator,
    ::zrythm::actions::TrackCreator          &track_creator,
    QObject *                                 parent = nullptr);

  /**
   * @brief Enumeration for supported file types.
   */
  enum class FileType
  {
    Audio,
    Midi,
    Unsupported
  };

  /**
   * @brief Imports the given file.
   *
   * @param path File to import.
   * @param startTicks Start ticks to place file contents at, or 0.
   * @param track Track to import in (optional).
   */
  Q_INVOKABLE void importFiles (
    const QStringList         &filePaths,
    double                     startTicks,
    structure::tracks::Track * track) const;

  Q_INVOKABLE void importFileToClipSlot (
    const QString                &filePath,
    structure::tracks::Track *    track,
    structure::scenes::Scene *    scene,
    structure::scenes::ClipSlot * clipSlot) const;

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

private:
  ::zrythm::actions::TrackCreator          &track_creator_;
  ::zrythm::actions::ArrangerObjectCreator &arranger_object_creator_;
  undo::UndoStack                          &undo_stack_;

  /** Audio format manager for detecting audio files. */
  mutable juce::AudioFormatManager audio_format_manager_;
};
}
