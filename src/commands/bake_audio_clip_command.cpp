// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "commands/bake_audio_clip_command.h"
#include "structure/arrangement/audio_clip.h"
#include "utils/debug.h"

namespace zrythm::commands
{

namespace
{

dsp::FileAudioSourceUuidReference
source_ref_of (const structure::arrangement::ArrangerObjectUuidReference &clip)
{
  auto * audio_clip = clip.get_object_as<structure::arrangement::AudioClip> ();
  z_return_val_if_fail (
    audio_clip != nullptr && !audio_clip->get_children_view ().empty (),
    dsp::FileAudioSourceUuidReference{ clip.get_registry () });
  return audio_clip->get_children_view ().front ()->audio_source_ref ();
}

} // namespace

BakeAudioClipCommand::BakeAudioClipCommand (
  structure::arrangement::ArrangerObjectUuidReference clip,
  dsp::FileAudioSourceUuidReference                   after_source,
  const QString                                      &text)
    : QUndoCommand (text), clip_ (std::move (clip)),
      before_source_ (source_ref_of (clip_)),
      after_source_ (std::move (after_source))
{
  z_return_if_fail (before_source_.has_value () && after_source_.has_value ());
  auto * audio_clip = clip_of ();
  z_return_if_fail (
    audio_clip != nullptr && !audio_clip->transform_entries ().empty ());
  before_entries_ =
    audio_clip->transform_entries () | std::ranges::to<std::vector> ();
}

void
BakeAudioClipCommand::redo ()
{
  z_return_if_fail (before_source_.has_value () && after_source_.has_value ());
  auto * audio_clip = clip_of ();
  z_return_if_fail (
    audio_clip != nullptr && !audio_clip->get_children_view ().empty ());
  audio_clip->get_children_view ().front ()->set_file_audio_source (
    after_source_);
  audio_clip->set_transform_entries ({});
}

void
BakeAudioClipCommand::undo ()
{
  z_return_if_fail (before_source_.has_value () && after_source_.has_value ());
  auto * audio_clip = clip_of ();
  z_return_if_fail (
    audio_clip != nullptr && !audio_clip->get_children_view ().empty ());
  audio_clip->get_children_view ().front ()->set_file_audio_source (
    before_source_);
  audio_clip->set_transform_entries (before_entries_);
}

structure::arrangement::AudioClip *
BakeAudioClipCommand::clip_of () const
{
  return clip_.get_object_as<structure::arrangement::AudioClip> ();
}

} // namespace zrythm::commands
