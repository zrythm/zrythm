/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_AUDIO_CLIP_EDITOR_H__
#define __AUDIO_AUDIO_CLIP_EDITOR_H__

#define AUDIO_CLIP_EDITOR (&CLIP_EDITOR->audio_clip_editor)

/**
 * Audio clip editor serializable backend.
 *
 * The actual widgets should reflect the information here.
 */
typedef struct AudioClipEditor
{
  /* TODO */
  int dummy;
} AudioClipEditor;


void
audio_clip_editor_init (AudioClipEditor * self);

#endif
