/*
 * SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Marker widget.
 */

#ifndef __GUI_WIDGETS_MARKER_H__
#define __GUI_WIDGETS_MARKER_H__

#include "gui/widgets/arranger_object.h"

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MARKER_NAME_FONT "Bold 8"
#define MARKER_NAME_PADDING 2

/**
 * Recreates the pango layouts for drawing.
 */
void
marker_recreate_pango_layouts (Marker * self);

/**
 * Draws the given marker.
 */
void
marker_draw (Marker * self, GtkSnapshot * snapshot);

/**
 * @}
 */

#endif
