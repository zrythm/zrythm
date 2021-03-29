/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 */

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/chord_track.h"
#include "audio/region.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/chord_track.h"
#include "gui/widgets/track_top_grid.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ChordTrackWidget,
               chord_track_widget,
               TRACK_WIDGET_TYPE)

/**
 * Creates a new track widget using the given track.
 *
 * 1 track has 1 track widget.
 * The track widget must always have at least 1 automation track in the automation
 * paned.
 */
ChordTrackWidget *
chord_track_widget_new (Track * track)
{
  ChordTrackWidget * self = g_object_new (
                            CHORD_TRACK_WIDGET_TYPE,
                            NULL);
  TRACK_WIDGET_GET_PRIVATE (self);

  track_widget_set_name (
    Z_TRACK_WIDGET (self), track->name);

  /* setup color */
  color_area_widget_set_color (tw_prv->color,
                               &track->color);

  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  return self;
}

/**
 * FIXME delete
 */
void
chord_track_widget_refresh_buttons (
  ChordTrackWidget * self)
{
  /* TODO*/

}

void
chord_track_widget_refresh (ChordTrackWidget * self)
{
}

static void
chord_track_widget_init (ChordTrackWidget * self)
{
  GtkStyleContext * context;
  TRACK_WIDGET_GET_PRIVATE (self);

  /* create buttons */
  self->record =
    z_gtk_toggle_button_new_with_icon (
      "media-record");
  context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self->record));
  gtk_style_context_add_class (
    context, "record-button");
  self->solo =
    z_gtk_toggle_button_new_with_resource (
      ICON_TYPE_ZRYTHM,
      "solo.svg");
  context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self->solo));
  gtk_style_context_add_class (context, "solo-button");
  self->mute =
    z_gtk_toggle_button_new_with_resource (ICON_TYPE_ZRYTHM,
                                    "mute.svg");

  /* set buttons to upper controls */
  gtk_box_pack_start (
    GTK_BOX (tw_prv->top_grid->upper_controls),
    GTK_WIDGET (self->record),
    Z_GTK_NO_EXPAND,
    Z_GTK_NO_FILL,
    0);
  gtk_box_pack_start (
    GTK_BOX (tw_prv->top_grid->upper_controls),
    GTK_WIDGET (self->solo),
    Z_GTK_NO_EXPAND,
    Z_GTK_NO_FILL,
    0);
  gtk_box_pack_start (
    GTK_BOX (tw_prv->top_grid->upper_controls),
    GTK_WIDGET (self->mute),
    Z_GTK_NO_EXPAND,
    Z_GTK_NO_FILL,
    0);

  /* TODO temporary */
  gtk_widget_set_size_request (
    GTK_WIDGET (self), -1, 42);

  /* set icon */
  SET_TRACK_ICON ("minuet-chords");
}

static void
chord_track_widget_class_init (
  ChordTrackWidgetClass * _klass)
{
}
