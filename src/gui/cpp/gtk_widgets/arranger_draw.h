// SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Draw functions for ArrangerWidget - split to make
 * file smaller.
 *
 * TODO
 */

#ifndef __GUI_WIDGETS_ARRANGER_DRAW_H__
#define __GUI_WIDGETS_ARRANGER_DRAW_H__

#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

void
arranger_snapshot (GtkWidget * widget, GtkSnapshot * snapshot);

/**
 * @}
 */

#endif
