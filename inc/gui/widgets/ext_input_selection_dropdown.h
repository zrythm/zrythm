// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 */

#ifndef __GUI_WIDGETS_EXT_INPUT_SELECTION_DROPDOWN_H__
#define __GUI_WIDGETS_EXT_INPUT_SELECTION_DROPDOWN_H__

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
 *
 * @param left Whether this is for the left channel, otherwise right (if audio).
 */
void
ext_input_selection_dropdown_widget_refresh (
  GtkDropDown *  self,
  ChannelTrack * track,
  bool           left);

/**
 * @}
 */

#endif
