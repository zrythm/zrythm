// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "commands/command.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/audio_function.h"

namespace zrythm::commands
{

/**
 * @brief Replaces an audio clip's transform entry list with a before/after
 * snapshot.
 *
 * Entries are pure clip state (no pool or file interaction); undo and redo
 * only swap the remembered lists.
 */
class SetAudioClipTransformsCommand : public Command
{
public:
  /**
   * @param clip The audio clip whose entries are replaced.
   * @param after_entries The entry list applied on every redo.
   * @param text Undo-stack text.
   *
   * @pre @p clip is an AudioClip.
   */
  SetAudioClipTransformsCommand (
    structure::arrangement::ArrangerObjectUuidReference       clip,
    std::vector<structure::arrangement::AudioFunction::Entry> after_entries,
    const QString                                            &text);

  void undo () override;
  void redo () override;

private:
  structure::arrangement::ArrangerObjectUuidReference       clip_;
  std::vector<structure::arrangement::AudioFunction::Entry> before_entries_;
  std::vector<structure::arrangement::AudioFunction::Entry> after_entries_;
};

} // namespace zrythm::commands
