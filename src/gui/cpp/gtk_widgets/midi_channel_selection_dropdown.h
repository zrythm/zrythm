// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_MIDI_CHANNEL_SELECTION_DROPDOWN_H__
#define __GUI_WIDGETS_MIDI_CHANNEL_SELECTION_DROPDOWN_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "common/utils/types.h"

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
