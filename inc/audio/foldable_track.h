/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Common logic for foldable tracks.
 */

#ifndef __AUDIO_FOLDABLE_TRACK_H__
#define __AUDIO_FOLDABLE_TRACK_H__

#include <stdbool.h>

typedef struct Track Track;

typedef enum FoldableTrackMixerStatus {
  FOLDABLE_TRACK_MIXER_STATUS_MUTED,
  FOLDABLE_TRACK_MIXER_STATUS_SOLOED,
  FOLDABLE_TRACK_MIXER_STATUS_IMPLIED_SOLOED,
  FOLDABLE_TRACK_MIXER_STATUS_LISTENED,
} FoldableTrackMixerStatus;

void
foldable_track_init (Track * track);

/**
 * Used to check if soloed/muted/etc.
 */
bool
foldable_track_is_status (
  Track *                  self,
  FoldableTrackMixerStatus status);

#endif /* __AUDIO_FOLDABLE_TRACK_H__ */
