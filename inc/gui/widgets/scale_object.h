/*
 * SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Widget for ScaleObject.
 */

#ifndef __GUI_WIDGETS_SCALE_OBJECT_H__
#define __GUI_WIDGETS_SCALE_OBJECT_H__

#include "gui/widgets/arranger_object.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define SCALE_OBJECT_WIDGET_TRIANGLE_W 10
#define SCALE_OBJECT_NAME_FONT "Sans SemiBold 8"
#define SCALE_OBJECT_NAME_PADDING 2

/**
 * Recreates the pango layouts for drawing.
 *
 * @param width Full width of the marker.
 */
void
scale_object_recreate_pango_layouts (ScaleObject * self);

/**
 * Draws the given scale object.
 */
void
scale_object_draw (ScaleObject * self, GtkSnapshot * snapshot);

/**
 * @}
 */

#endif
