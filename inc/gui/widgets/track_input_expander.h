/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#ifndef __GUI_WIDGETS_TRACK_INPUT_EXPANDER_H__
#define __GUI_WIDGETS_TRACK_INPUT_EXPANDER_H__

#include "gui/widgets/two_col_expander_box.h"

#include <gtk/gtk.h>

#define TRACK_INPUT_EXPANDER_WIDGET_TYPE \
  (track_input_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackInputExpanderWidget,
  track_input_expander_widget,
  Z,
  TRACK_INPUT_EXPANDER_WIDGET,
  TwoColExpanderBoxWidget);

typedef struct _EditableLabelWidget
                           EditableLabelWidget;
typedef struct Track       Track;
typedef struct _KnobWidget KnobWidget;

typedef struct _TrackInputExpanderWidget
{
  TwoColExpanderBoxWidget parent_instance;

  /** Track input port for midi. */
  GtkComboBox * midi_input;

  /** Track input port for audio L. */
  GtkComboBox * stereo_l_input;

  /** Track input port for audio R. */
  GtkComboBox * stereo_r_input;

  /** MIDI channels selector. */
  GtkComboBox * midi_channels;

  /** Size group for audio inputs. */
  GtkSizeGroup * audio_input_size_group;

  /** Mono switch for audio tracks. */
  GtkToggleButton * mono;

  /** Gain knob for audio tracks. */
  GtkBox *     gain_box;
  KnobWidget * gain;

  /** Track the TrackInputExpanderWidget is
   * associated with. */
  Track * track;
} TrackInputExpanderWidget;

/**
 * Refreshes each field.
 */
void
track_input_expander_widget_refresh (
  TrackInputExpanderWidget * self,
  Track *                    track);

/**
 * Sets up the TrackInputExpanderWidget.
 */
void
track_input_expander_widget_setup (
  TrackInputExpanderWidget * self,
  Track *                    track);

#endif
