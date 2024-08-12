/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Object color chooser dialog.
 */

#ifndef __GUI_WIDGETS_TRACK_ICON_CHOOSER_DIALOG_H__
#define __GUI_WIDGETS_TRACK_ICON_CHOOSER_DIALOG_H__

#include "gtk_wrapper.h"

typedef Track Track;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Dialog for choosing colors of objects like tracks
 * and regions.
 */
typedef struct TrackIconChooserDialogWidget
{
  GtkDialog * dialog;

  GtkTreeModel * icon_model;
  GtkIconView *  icon_view;

  char * selected_icon;

  /** Track. */
  Track * track;
} TrackIconChooserDialogWidget;

/**
 * Creates a new dialog.
 */
TrackIconChooserDialogWidget *
track_icon_chooser_dialog_widget_new (Track * track);

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @return Whether the icon was set or not.
 */
bool
track_icon_chooser_dialog_widget_run (TrackIconChooserDialogWidget * self);

/**
 * @}
 */

#endif
