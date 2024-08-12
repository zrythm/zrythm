// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/** @file
 */

#ifndef __GUI_WIDGETS_METER_H__
#define __GUI_WIDGETS_METER_H__

#include "utils/general.h"

#include "gtk_wrapper.h"

#define METER_WIDGET_TYPE (meter_widget_get_type ())
G_DECLARE_FINAL_TYPE (MeterWidget, meter_widget, Z, METER_WIDGET, GtkWidget)

class Meter;

typedef struct _MeterWidget
{
  GtkWidget parent_instance;

  /** Associated meter. */
  Meter * meter;

  /** Hovered or not. */
  int hover;

  Color start_color;
  Color end_color;

  float meter_val;
  float meter_peak;

  /** Caches of last drawn values so that meters are redrawn only when there are
   * changes. */
  float last_meter_val;
  float last_meter_peak;

  /** ID of the source function. */
  guint     source_id;
  GSource * timeout_source;
} MeterWidget;

/**
 * Sets up an existing meter and binds it to the given port.
 *
 * @param port Port this meter is for.
 * @param small Whether this should be smaller than normal size.
 */
void
meter_widget_setup (MeterWidget * self, Port * port, bool small);

/**
 * Creates a new MeterWidget and binds it to the given port.
 *
 * @param port Port this meter is for.
 */
MeterWidget *
meter_widget_new (Port * port, int width);

#endif
