// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_CHORD_REGION_H__
#define __GUI_WIDGETS_CHORD_REGION_H__

#include "dsp/region.h"
#include "gui/widgets/region.h"
#include "utils/ui.h"

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Recreates the pango layout for drawing chord names inside the region.
 */
void
chord_region_recreate_pango_layouts (ChordRegion * self);

/**
 * @}
 */

#endif
