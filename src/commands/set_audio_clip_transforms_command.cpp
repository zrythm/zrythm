// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "commands/set_audio_clip_transforms_command.h"
#include "structure/arrangement/audio_clip.h"

namespace zrythm::commands
{

namespace
{

structure::arrangement::AudioClip &
audio_clip_of (const structure::arrangement::ArrangerObjectUuidReference &clip)
{
  auto * audio_clip = clip.get_object_as<structure::arrangement::AudioClip> ();
  assert (audio_clip != nullptr);
  return *audio_clip;
}

} // namespace

SetAudioClipTransformsCommand::SetAudioClipTransformsCommand (
  structure::arrangement::ArrangerObjectUuidReference       clip,
  std::vector<structure::arrangement::AudioFunction::Entry> after_entries,
  const QString                                            &text)
    : QUndoCommand (text), clip_ (std::move (clip)),
      before_entries_ (
        audio_clip_of (clip_).transform_entries ()
        | std::ranges::to<std::vector> ()),
      after_entries_ (std::move (after_entries))
{
}

void
SetAudioClipTransformsCommand::redo ()
{
  audio_clip_of (clip_).set_transform_entries (after_entries_);
}

void
SetAudioClipTransformsCommand::undo ()
{
  audio_clip_of (clip_).set_transform_entries (before_entries_);
}

} // namespace zrythm::commands
