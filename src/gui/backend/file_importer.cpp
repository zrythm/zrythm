// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/file_importer.h"

namespace zrythm::gui::backend
{
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
      auto track_qvar = track_creator_.addEmptyTrackFromType (
        structure::tracks::Track::Type::Audio);
      auto * audio_track = track_qvar.value<structure::tracks::AudioTrack *> ();
      auto * audio_region = arranger_object_creator_.addAudioRegionFromFile (
        audio_track, audio_track->lanes ()->getFirstLane (), filepath, 0);

      // FIXME!!!! temporary test code
      audio_region->prepare_to_play (2048, 44100);
    }
  undo_stack_.endMacro ();
}
}
