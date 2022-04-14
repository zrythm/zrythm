// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_TRACK_CANVAS_H__
#define __GUI_WIDGETS_TRACK_CANVAS_H__

/**
 * \file
 *
 * Track canvas.
 */

#include <gtk/gtk.h>

#define TRACK_CANVAS_WIDGET_TYPE \
  (track_canvas_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackCanvasWidget,
  track_canvas_widget,
  Z,
  TRACK_CANVAS_WIDGET,
  GtkWidget)

typedef struct _TrackWidget TrackWidget;

typedef struct _TrackCanvasWidget
{
  GtkWidget parent_instance;

  TrackWidget * parent;

  /** Layout for drawing the name. */
  PangoLayout * layout;

  /** Layout for automation value. */
  PangoLayout * automation_value_layout;

  /** Used for recreating the pango layout. */
  int last_width;
  int last_height;

  char * last_track_icon_name;

  GdkTexture * track_icon;
} TrackCanvasWidget;

void
track_canvas_widget_setup (
  TrackCanvasWidget * self,
  TrackWidget *       parent);

#endif
