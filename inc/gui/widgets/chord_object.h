// clang-format off
// SPDX-FileCopyrightText: © 2019, 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Widget for ChordObject.
 */

#ifndef __GUI_WIDGETS_CHORD_OBJECT_H__
#define __GUI_WIDGETS_CHORD_OBJECT_H__

#include "gui/widgets/arranger_object.h"

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CHORD_OBJECT_WIDGET_TRIANGLE_W 10
#define CHORD_OBJECT_NAME_FONT "Bold 8"
#define CHORD_OBJECT_NAME_PADDING 2

/**
 * Recreates the pango layouts for drawing.
 */
void
chord_object_recreate_pango_layouts (ChordObject * self);

/**
 * Draws the given chord object.
 */
void
chord_object_draw (ChordObject * self, GtkSnapshot * snapshot);

/**
 * @}
 */

#endif
