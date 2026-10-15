// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/file_audio_source.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/audio_function.h"

#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief Re-points an audio clip at its baked source.
 *
 * Redo points the clip at @p after_source (produced by
 * AudioFunction::bake_audio_clip()) and clears its transform entries;
 * undo re-points back and restores them. Both sources stay registered
 * while the command is alive, so their blocks are protected from garbage
 * collection.
 */
class BakeAudioClipCommand : public QUndoCommand
{
public:
  static constexpr int CommandId = 1684239751;

  /**
   * @param clip The audio clip whose entries were baked.
   * @param after_source The source the bake produced.
   * @param text Undo-stack text.
   *
   * @pre @p clip is an AudioClip with a source and at least one
   * transform entry.
   */
  BakeAudioClipCommand (
    structure::arrangement::ArrangerObjectUuidReference clip,
    dsp::FileAudioSourceUuidReference                   after_source,
    const QString                                      &text);

  int  id () const override { return CommandId; }
  void undo () override;
  void redo () override;

private:
  /**
   * @return The clip, or null if it is no longer registered.
   */
  structure::arrangement::AudioClip * clip_of () const;

  structure::arrangement::ArrangerObjectUuidReference       clip_;
  std::vector<structure::arrangement::AudioFunction::Entry> before_entries_;
  dsp::FileAudioSourceUuidReference                         before_source_;
  dsp::FileAudioSourceUuidReference                         after_source_;
};

} // namespace zrythm::commands
