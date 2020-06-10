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
#include "audio/automation_tracklist.h"
#include "audio/audio_bus_track.h"
#include "audio/master_track.h"
#include "audio/track.h"
#include "audio/region.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/master_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_top_grid.h"
#include "gui/widgets/tracklist.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MasterTrackWidget,
               master_track_widget,
               TRACK_WIDGET_TYPE)

/**
 * Creates a new track widget using the given track.
 *
 * 1 track has 1 track widget.
 * The track widget must always have at least 1 automation
 * track in the automation paned.
 */
MasterTrackWidget *
master_track_widget_new (Track * track)
{
  MasterTrackWidget * self = g_object_new (
                            MASTER_TRACK_WIDGET_TYPE,
                            NULL);
  TRACK_WIDGET_GET_PRIVATE (self);

  /* setup color */
  color_area_widget_set_color (tw_prv->color,
                               &track->color);

  /* setup automation tracklist */
  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (track);
  automation_tracklist->widget =
    automation_tracklist_widget_new (automation_tracklist);
  gtk_container_add (
    GTK_CONTAINER (tw_prv->paned),
    GTK_WIDGET (automation_tracklist->widget));

  tw_prv->mute_toggled_handler_id =
    g_signal_connect (
      self->mute, "toggled",
      G_CALLBACK (track_widget_on_mute_toggled),
      self);
  tw_prv->solo_toggled_handler_id =
    g_signal_connect (
      self->solo, "toggled",
      G_CALLBACK (track_widget_on_solo_toggled),
      self);
  g_signal_connect (
    self->show_automation,
    "toggled",
    G_CALLBACK (track_widget_on_show_automation_toggled),
    self);

  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  return self;
}

void
master_track_widget_refresh_buttons (
  MasterTrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  g_signal_handler_block (
    self->solo, tw_prv->solo_toggled_handler_id);
      gtk_toggle_button_set_active (
        self->solo,
        tw_prv->track->solo);
  g_signal_handler_unblock (
    self->solo, tw_prv->solo_toggled_handler_id);

  g_signal_handler_block (
    self->mute, tw_prv->mute_toggled_handler_id);
      gtk_toggle_button_set_active (
        self->mute,
        tw_prv->track->mute);
  g_signal_handler_unblock (
    self->mute, tw_prv->mute_toggled_handler_id);
}

void
master_track_widget_refresh (
  MasterTrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;

  track_widget_set_name (
    Z_TRACK_WIDGET (self), track->name);

  master_track_widget_refresh_buttons (self);

  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (tw_prv->track);
  automation_tracklist_widget_refresh (
    automation_tracklist->widget);
}

static void
master_track_widget_init (MasterTrackWidget * self)
{
  GtkStyleContext * context;
  TRACK_WIDGET_GET_PRIVATE (self);

  /* create buttons */
  self->solo =
    z_gtk_toggle_button_new_with_resource (
      ICON_TYPE_ZRYTHM,
      "solo.svg");
  context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self->solo));
  gtk_style_context_add_class (context, "solo-button");
  self->mute =
    z_gtk_toggle_button_new_with_resource (
      ICON_TYPE_ZRYTHM,
      "mute.svg");
  self->show_automation =
    z_gtk_toggle_button_new_with_icon (
      "node-type-cusp");

  /*set buttons to upper controls*/
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

  /* pack buttons to bot controls */
  gtk_box_pack_start (
    GTK_BOX (tw_prv->top_grid->bot_controls),
    GTK_WIDGET (self->show_automation),
    Z_GTK_NO_EXPAND,
    Z_GTK_NO_FILL,
    0);

  /* set icon */
  SET_TRACK_ICON ("bus");
}

static void
master_track_widget_class_init (
  MasterTrackWidgetClass * _klass)
{
}
