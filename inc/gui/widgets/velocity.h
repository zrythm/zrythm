/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Velocity widget.
 */

#ifndef __GUI_WIDGETS_VELOCITY_H__
#define __GUI_WIDGETS_VELOCITY_H__

#include "dsp/velocity.h"
#include "gui/widgets/arranger_object.h"
#include "utils/ui.h"

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define VELOCITY_WIDTH 8
#define VELOCITY_LINE_WIDTH 4
#define VELOCITY_RESIZE_THRESHOLD 16

/**
 * Draws the Velocity in the given cairo context in
 * relative coordinates.
 */
void
velocity_draw (Velocity * self, GtkSnapshot * snapshot);

/**
 * @}
 */

#endif
