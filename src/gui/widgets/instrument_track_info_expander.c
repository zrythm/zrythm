/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/instrument_track_info_expander.h"
#include "project.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (InstrumentTrackInfoExpanderWidget,
               instrument_track_info_expander_widget,
               TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

/**
 * Refreshes each field.
 */
void
instrument_track_info_expander_widget_refresh (
  InstrumentTrackInfoExpanderWidget * self)
{
  /* TODO */

}

/**
 * Sets up the InstrumentTrackInfoExpanderWidget.
 */
void
instrument_track_info_expander_widget_setup (
  InstrumentTrackInfoExpanderWidget * self,
  Track *                             track)
{
  g_warn_if_fail (track);
  self->track = track;

  editable_label_widget_setup (
    self->name,
    track, track_get_name, track_set_name);
  instrument_track_info_expander_widget_refresh (
    self);
}

static void
instrument_track_info_expander_widget_class_init (
  InstrumentTrackInfoExpanderWidgetClass * klass)
{
}

static void
instrument_track_info_expander_widget_init (
  InstrumentTrackInfoExpanderWidget * self)
{
  self->name =
    editable_label_widget_new (
      NULL, NULL, NULL, 11);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->name));

  /* set name and icon */
  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    _("Track Info"));
  expander_box_widget_set_icon_resource (
    Z_EXPANDER_BOX_WIDGET (self),
    ICON_TYPE_ZRYTHM,
    "instrument.svg");
}
