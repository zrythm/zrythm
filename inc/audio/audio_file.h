// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio file info.
 */

#ifndef __AUDIO_AUDIO_FILE_H__
#define __AUDIO_AUDIO_FILE_H__

typedef struct SupportedFile SupportedFile;

/**
 * An audio file.
 */
typedef struct AudioFile
{
  int channels;

  /** Pointer to parent SupportedFile. */
  SupportedFile * file;
} AudioFile;

#endif
