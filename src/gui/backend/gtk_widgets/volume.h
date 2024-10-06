// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_VOLUME_H__
#define __GUI_WIDGETS_VOLUME_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

class ControlPort;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define VOLUME_WIDGET_TYPE (volume_widget_get_type ())
G_DECLARE_FINAL_TYPE (VolumeWidget, volume_widget, Z, VOLUME_WIDGET, GtkDrawingArea)

using VolumeWidget = struct _VolumeWidget
{
  GtkDrawingArea parent_instance;

  /** Control port to change. */
  ControlPort * port;

  bool hover;

  GtkGestureDrag * drag;

  double last_x;
  double last_y;
};

void
volume_widget_setup (VolumeWidget * self, ControlPort * port);

VolumeWidget *
volume_widget_new (ControlPort * port);

/**
 * @}
 */

#endif
