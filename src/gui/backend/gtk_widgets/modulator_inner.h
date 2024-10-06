// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_MODULATOR_INNER_H__
#define __GUI_WIDGETS_MODULATOR_INNER_H__

#include "common/utils/types.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define MODULATOR_INNER_WIDGET_TYPE (modulator_inner_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ModulatorInnerWidget,
  modulator_inner_widget,
  Z,
  MODULATOR_INNER_WIDGET,
  GtkBox)

TYPEDEF_STRUCT_UNDERSCORED (KnobWithNameWidget);
TYPEDEF_STRUCT_UNDERSCORED (LiveWaveformWidget);
TYPEDEF_STRUCT_UNDERSCORED (ModulatorWidget);
TYPEDEF_STRUCT_UNDERSCORED (PortConnectionsPopoverWidget);
class Port;

using ModulatorInnerWidget = struct _ModulatorInnerWidget
{
  GtkBox parent_instance;

  GtkBox * toolbar;

  GtkToggleButton * show_hide_ui_btn;

  GtkBox * controls_box;
  GtkBox * waveforms_box;

  std::vector<KnobWithNameWidget *> knobs;

  /** The graphs on the right. */
  GtkOverlay *         waveform_overlays[16];
  GtkButton *          waveform_automate_buttons[16];
  LiveWaveformWidget * waveforms[16];
  int                  num_waveforms;

  Port * ports[16];

  /** Pointer back to the Modulator. */
  ModulatorWidget * parent;

  PortConnectionsPopoverWidget * connections_popover;
};

void
modulator_inner_widget_refresh (ModulatorInnerWidget * self);

/**
 * Creates a new widget.
 */
ModulatorInnerWidget *
modulator_inner_widget_new (ModulatorWidget * parent);

#endif
