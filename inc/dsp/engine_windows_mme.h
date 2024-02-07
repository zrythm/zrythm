/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */
/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 */

#ifndef __AUDIO_ENGINE_WINDOWS_MME_H__
#define __AUDIO_ENGINE_WINDOWS_MME_H__

#include "zrythm-config.h"

#ifdef _WOE32

#  include <windows.h>

#  include <gtk/gtk.h>

typedef struct AudioEngine      AudioEngine;
typedef struct WindowsMmeDevice WindowsMmeDevice;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Returns the number of MIDI devices available.
 */
int
engine_windows_mme_get_num_devices (int input);

/**
 * Gets the error text for a MIDI input error.
 *
 * @return 0 if successful, non-zero if failed.
 */
int
engine_windows_mme_get_error (
  MMRESULT error_code,
  int      input,
  char *   buf,
  int      buf_size);

/**
 * Prints the error in the log.
 */
void
engine_windows_mme_print_error (MMRESULT error_code, int input);

/**
 * Initialize Port MIDI.
 */
int
engine_windows_mme_setup (AudioEngine * self);

/**
 * Starts all previously scanned devices.
 */
int
engine_windows_mme_activate (AudioEngine * self, bool activate);

/**
 * Rescans for MIDI devices, opens them and keeps
 * track of them.
 *
 * @param start Whether to start receiving events
 *   on the devices or not.
 */
int
engine_windows_mme_rescan_devices (AudioEngine * self, int start);

int
engine_windows_mme_tear_down (AudioEngine * self);

/**
 * Tests if the engine is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_windows_mme_test (GtkWindow * win);

/**
 * @}
 */

#endif // _WOE32
#endif
