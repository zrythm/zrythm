/*
 * SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Single chord pad.
 */

#ifndef __GUI_WIDGETS_CHORD_PAD_H__
#define __GUI_WIDGETS_CHORD_PAD_H__

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CHORD_PAD_WIDGET_TYPE (chord_pad_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordPadWidget,
  chord_pad_widget,
  Z,
  CHORD_PAD_WIDGET,
  GtkWidget)

/**
 * Single chord pad.
 */
typedef struct _ChordPadWidget
{
  GtkWidget parent_instance;

  /** Main child. */
  GtkOverlay * overlay;

  /** Button. */
  GtkButton * btn;

  GtkGestureDrag * btn_drag;

  double drag_start_x;
  double drag_start_y;

  /** Whether the drag has started. */
  bool drag_started;

  GtkBox *    btn_box;
  GtkButton * edit_chord_btn;
  GtkButton * invert_prev_btn;
  GtkButton * invert_next_btn;

  /** Index of the chord in the chord track. */
  int chord_idx;
} ChordPadWidget;

/**
 * Creates a chord widget.
 */
ChordPadWidget *
chord_pad_widget_new (void);

/**
 * Sets the chord index on the chord widget.
 */
void
chord_pad_widget_refresh (ChordPadWidget * self, int idx);

/**
 * @}
 */

#endif
