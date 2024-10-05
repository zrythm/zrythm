// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_CHORD_REGION_H__
#define __GUI_WIDGETS_CHORD_REGION_H__

#include "common/dsp/region.h"
#include "common/utils/ui.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/region.h"

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
