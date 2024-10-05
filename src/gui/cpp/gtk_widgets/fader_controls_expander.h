// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_FADER_CONTROLS_EXPANDER_H__
#define __GUI_WIDGETS_FADER_CONTROLS_EXPANDER_H__

#include "common/utils/types.h"
#include "gui/cpp/gtk_widgets/expander_box.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define FADER_CONTROLS_EXPANDER_WIDGET_TYPE \
  (fader_controls_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FaderControlsExpanderWidget,
  fader_controls_expander_widget,
  Z,
  FADER_CONTROLS_EXPANDER_WIDGET,
  ExpanderBoxWidget);

class ChannelTrack;
TYPEDEF_STRUCT_UNDERSCORED (FaderControlsGridWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * A TwoColExpanderBoxWidget for showing the ports
 * in the InspectorWidget.
 */
using FaderControlsExpanderWidget = struct _FaderControlsExpanderWidget
{
  ExpanderBoxWidget parent_instance;

  /** Grid containing each separate widget. */
  FaderControlsGridWidget * grid;

  /** Owner track. */
  ChannelTrack * track;
};

/**
 * Refreshes each field.
 */
void
fader_controls_expander_widget_refresh (FaderControlsExpanderWidget * self);

/**
 * Sets up the FaderControlsExpanderWidget.
 */
void
fader_controls_expander_widget_setup (
  FaderControlsExpanderWidget * self,
  ChannelTrack *                track);

/**
 * Prepare for finalization.
 */
void
fader_controls_expander_widget_tear_down (FaderControlsExpanderWidget * self);

/**
 * @}
 */

#endif
