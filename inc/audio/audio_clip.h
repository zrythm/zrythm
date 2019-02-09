/*
 * audio/audio_clip.h - Audio clip for audio regions
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __AUDIO_AUDIO_CLIP_H__
#define __AUDIO_AUDIO_CLIP_H__

typedef struct _AudioClipWidget AudioClipWidget;
typedef struct AudioRegion AudioRegion;

/**
 * An audio clip contains information required when playing
 * audio clips.
 */
typedef struct AudioClip
{
  int                 id;

  /**
   * Start position of the clip can be adjusted.
   *
   * Useful when samples contain silence in the beginning.
   */
  long                start_frames;

  /**
   * End position (inclusive).
   */
  long                end_frames;

  /**
   * Position to fade in until.
   */
  long                fade_in_frames;

  /* TODO fade curve support */

  /**
   * Position to start fading out from.
   */
  long                fade_out_frames;

  /**
   * Buffer holding samples/frames.
   */
  float *             buff;
  long                buff_size;

  int                 channels;

  /**
   * Original filename.
   */
  char *              filename;

  AudioClipWidget *   widget;

  /**
   * Owner.
   *
   * For convenience only (cache).
   */
  AudioRegion *       audio_region; ///< cache
} AudioClip;

/* TODO YAML schema */

AudioClip *
audio_clip_new (AudioRegion * region,
                float *       buff,
                long          buff_size,
                int           channels,
                char *        filename);

void
audio_clip_free (AudioClip * self);

#endif // __AUDIO_AUDIO_CLIP_H__
