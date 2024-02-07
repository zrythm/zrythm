/*
 * SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __GUI_WIDGETS_VOLUME_H__
#define __GUI_WIDGETS_VOLUME_H__

#include <gtk/gtk.h>

typedef struct Port Port;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define VOLUME_WIDGET_TYPE (volume_widget_get_type ())
G_DECLARE_FINAL_TYPE (VolumeWidget, volume_widget, Z, VOLUME_WIDGET, GtkDrawingArea)

typedef struct _VolumeWidget
{
  GtkDrawingArea parent_instance;

  /** Control port to change. */
  Port * port;

  bool hover;

  GtkGestureDrag * drag;

  double last_x;
  double last_y;
} VolumeWidget;

void
volume_widget_setup (VolumeWidget * self, Port * port);

VolumeWidget *
volume_widget_new (Port * port);

/**
 * @}
 */

#endif
