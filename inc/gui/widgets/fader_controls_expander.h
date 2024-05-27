/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Fader controls expander.
 */

#ifndef __GUI_WIDGETS_FADER_CONTROLS_EXPANDER_H__
#define __GUI_WIDGETS_FADER_CONTROLS_EXPANDER_H__

#include "gui/widgets/expander_box.h"

#include "gtk_wrapper.h"

#define FADER_CONTROLS_EXPANDER_WIDGET_TYPE \
  (fader_controls_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FaderControlsExpanderWidget,
  fader_controls_expander_widget,
  Z,
  FADER_CONTROLS_EXPANDER_WIDGET,
  ExpanderBoxWidget);

typedef struct Track                    Track;
typedef struct _FaderControlsGridWidget FaderControlsGridWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * A TwoColExpanderBoxWidget for showing the ports
 * in the InspectorWidget.
 */
typedef struct _FaderControlsExpanderWidget
{
  ExpanderBoxWidget parent_instance;

  /** Grid containing each separate widget. */
  FaderControlsGridWidget * grid;

  /** Owner track. */
  Track * track;
} FaderControlsExpanderWidget;

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
  Track *                       track);

/**
 * Prepare for finalization.
 */
void
fader_controls_expander_widget_tear_down (FaderControlsExpanderWidget * self);

/**
 * @}
 */

#endif
