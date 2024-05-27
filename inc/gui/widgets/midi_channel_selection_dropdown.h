// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 */

#ifndef __GUI_WIDGETS_MIDI_CHANNEL_SELECTION_DROPDOWN_H__
#define __GUI_WIDGETS_MIDI_CHANNEL_SELECTION_DROPDOWN_H__

#include "utils/types.h"

#include "gtk_wrapper.h"

TYPEDEF_STRUCT (Track);

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Recreates the internal model(s) on the given dropdown.
 */
void
midi_channel_selection_dropdown_widget_refresh (
  GtkDropDown * self,
  Track *       track);

/**
 * @}
 */

#endif
