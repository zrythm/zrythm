/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Fader controls expander.
 */

#ifndef __GUI_WIDGETS_FADER_CONTROLS_EXPANDER_H__
#define __GUI_WIDGETS_FADER_CONTROLS_EXPANDER_H__

#include "gui/widgets/expander_box.h"

#include <gtk/gtk.h>

#define FADER_CONTROLS_EXPANDER_WIDGET_TYPE \
  (fader_controls_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FaderControlsExpanderWidget,
  fader_controls_expander_widget,
  Z,
  FADER_CONTROLS_EXPANDER_WIDGET,
  ExpanderBoxWidget);

typedef struct Track Track;
typedef struct _FaderControlsGridWidget
  FaderControlsGridWidget;

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
fader_controls_expander_widget_refresh (
  FaderControlsExpanderWidget * self);

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
fader_controls_expander_widget_tear_down (
  FaderControlsExpanderWidget * self);

/**
 * @}
 */

#endif
