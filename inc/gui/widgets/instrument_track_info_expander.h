/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file */

#ifndef __GUI_WIDGETS_INSTRUMENT_TRACK_INFO_EXPANDER_H__
#define __GUI_WIDGETS_INSTRUMENT_TRACK_INFO_EXPANDER_H__

#include "gui/widgets/two_col_expander_box.h"

#include <gtk/gtk.h>

#define INSTRUMENT_TRACK_INFO_EXPANDER_WIDGET_TYPE \
  (instrument_track_info_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  InstrumentTrackInfoExpanderWidget,
  instrument_track_info_expander_widget,
  Z,
  INSTRUMENT_TRACK_INFO_EXPANDER_WIDGET,
  TwoColExpanderBoxWidget);

typedef struct _EditableLabelWidget
  EditableLabelWidget;
typedef struct Track Track;

typedef struct _InstrumentTrackInfoExpanderWidget
{
  TwoColExpanderBoxWidget parent_instance;

  /**
   * Editable label for displaying the name.
   *
   * This should take up the whole row.
   */
  EditableLabelWidget *   name;

  /* TODO midi inputs, etc. See Instrument Track
   * Inspector from cubase manual. */

  /** Track the InstrumentTrackInfoExpanderWidget is
   * associated with. */
  Track *                 track;
} InstrumentTrackInfoExpanderWidget;

/**
 * Refreshes each field.
 */
void
instrument_track_info_expander_widget_refresh (
  InstrumentTrackInfoExpanderWidget * self);

/**
 * Sets up the InstrumentTrackInfoExpanderWidget.
 */
void
instrument_track_info_expander_widget_setup (
  InstrumentTrackInfoExpanderWidget * self,
  Track *                             track);

#endif
