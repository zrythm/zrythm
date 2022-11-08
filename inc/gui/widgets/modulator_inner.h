// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_MODULATOR_INNER_H__
#define __GUI_WIDGETS_MODULATOR_INNER_H__

#include <gtk/gtk.h>

#define MODULATOR_INNER_WIDGET_TYPE \
  (modulator_inner_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ModulatorInnerWidget,
  modulator_inner_widget,
  Z,
  MODULATOR_INNER_WIDGET,
  GtkBox)

typedef struct _KnobWithNameWidget KnobWithNameWidget;
typedef struct _LiveWaveformWidget LiveWaveformWidget;
typedef struct _ModulatorWidget    ModulatorWidget;
typedef struct _PortConnectionsPopoverWidget
  PortConnectionsPopoverWidget;

typedef struct _ModulatorInnerWidget
{
  GtkBox parent_instance;

  GtkBox * toolbar;

  GtkToggleButton * show_hide_ui_btn;

  GtkBox * controls_box;
  GtkBox * waveforms_box;

  KnobWithNameWidget ** knobs;
  int                   num_knobs;
  size_t                knobs_size;

  /** The graphs on the right. */
  GtkOverlay *         waveform_overlays[16];
  GtkButton *          waveform_automate_buttons[16];
  LiveWaveformWidget * waveforms[16];
  int                  num_waveforms;

  Port * ports[16];

  /** Pointer back to the Modulator. */
  ModulatorWidget * parent;

  PortConnectionsPopoverWidget * connections_popover;
} ModulatorInnerWidget;

void
modulator_inner_widget_refresh (ModulatorInnerWidget * self);

/**
 * Creates a new widget.
 */
ModulatorInnerWidget *
modulator_inner_widget_new (ModulatorWidget * parent);

#endif
