/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Monitor section panel.
 */

#ifndef __GUI_WIDGETS_CONTROL_ROOM_H__
#define __GUI_WIDGETS_CONTROL_ROOM_H__

#include "gtk_wrapper.h"

#define MONITOR_SECTION_WIDGET_TYPE (monitor_section_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MonitorSectionWidget,
  monitor_section_widget,
  Z,
  MONITOR_SECTION_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MONITOR_SECTION MW_RIGHT_DOCK_EDGE->monitor_section

typedef struct _KnobWithNameWidget     KnobWithNameWidget;
typedef struct ControlRoom             ControlRoom;
typedef struct _SliderBarWidget        SliderBarWidget;
typedef struct _MeterWidget            MeterWidget;
typedef struct _ActiveHardwareMbWidget ActiveHardwareMbWidget;

typedef struct _MonitorSectionWidget
{
  GtkBox parent_instance;

  GtkLabel *  soloed_tracks_lbl;
  GtkButton * soloing_btn;
  GtkLabel *  muted_tracks_lbl;
  GtkButton * muting_btn;
  GtkLabel *  listened_tracks_lbl;
  GtkButton * listening_btn;

  GtkBox *             mute_level_box;
  KnobWithNameWidget * mute_level;
  GtkBox *             listen_level_box;
  KnobWithNameWidget * listen_level;
  GtkBox *             dim_level_box;
  KnobWithNameWidget * dim_level;

  GtkToggleButton * mono_toggle;
  GtkToggleButton * dim_toggle;
  GtkToggleButton * mute_toggle;

  /** Output knob. */
  GtkBox *             monitor_level_box;
  KnobWithNameWidget * monitor_level;

  /* --- hardware outputs --- */

  GtkBox *                 left_output_box;
  GtkLabel *               l_label;
  ActiveHardwareMbWidget * left_outputs;

  GtkBox *                 right_output_box;
  GtkLabel *               r_label;
  ActiveHardwareMbWidget * right_outputs;

  /** Pointer to backend. */
  ControlRoom * control_room;

} MonitorSectionWidget;

void
monitor_section_widget_refresh (MonitorSectionWidget * self);

void
monitor_section_widget_setup (
  MonitorSectionWidget * self,
  ControlRoom *          control_room);

/**
 * Creates a MonitorSectionWidget.
 */
MonitorSectionWidget *
monitor_section_widget_new (void);

/**
 * @}
 */

#endif
