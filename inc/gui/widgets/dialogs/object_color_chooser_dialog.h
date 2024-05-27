/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Object color chooser dialog.
 */

#ifndef __GUI_WIDGETS_OBJECT_COLOR_CHOOSER_DIALOG_H__
#define __GUI_WIDGETS_OBJECT_COLOR_CHOOSER_DIALOG_H__

#include "gtk_wrapper.h"

typedef struct Track               Track;
typedef struct Region              Region;
typedef struct TracklistSelections TracklistSelections;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @param track Track, if track.
 * @param sel TracklistSelections, if multiple
 *   tracks.
 * @param region Region, if region.
 *
 * @return Whether the color was set or not.
 */
bool
object_color_chooser_dialog_widget_run (
  GtkWindow *           parent,
  Track *               track,
  TracklistSelections * sel,
  Region *              region);

/**
 * @}
 */

#endif
