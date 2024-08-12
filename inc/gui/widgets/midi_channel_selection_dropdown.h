// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_MIDI_CHANNEL_SELECTION_DROPDOWN_H__
#define __GUI_WIDGETS_MIDI_CHANNEL_SELECTION_DROPDOWN_H__

#include "utils/types.h"

#include "gtk_wrapper.h"

class ChannelTrack;

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
  GtkDropDown *  self,
  ChannelTrack * track);

/**
 * @}
 */

#endif
