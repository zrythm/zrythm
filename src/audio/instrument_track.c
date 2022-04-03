// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 */

#include "zrythm-config.h"

#include <stdlib.h>

#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/instrument_track.h"
#include "audio/midi_note.h"
#include "audio/position.h"
#include "audio/region.h"
#include "audio/track.h"
#include "audio/velocity.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/track.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/midi.h"
#include "utils/stoat.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

/**
 * Initializes an instrument track.
 */
void
instrument_track_init (Track * self)
{
  self->type = TRACK_TYPE_INSTRUMENT;
  gdk_rgba_parse (&self->color, "#FF9616");

  self->icon_name = g_strdup ("instrument");
}

void
instrument_track_setup (InstrumentTrack * self)
{
  channel_track_setup (self);
}

Plugin *
instrument_track_get_instrument (Track * self)
{
  g_return_val_if_fail (
    self && self->type == TRACK_TYPE_INSTRUMENT
      && self->channel,
    false);

  Plugin * plugin = self->channel->instrument;
  g_return_val_if_fail (plugin, NULL);

  return plugin;
}

/**
 * Returns if the first plugin's UI in the
 * instrument track is visible.
 */
int
instrument_track_is_plugin_visible (Track * self)
{
  Plugin * plugin =
    instrument_track_get_instrument (self);
  g_return_val_if_fail (plugin, 0);

  return plugin->visible;
}

/**
 * Toggles whether the first plugin's UI in the
 * instrument Track is visible.
 */
void
instrument_track_toggle_plugin_visible (Track * self)
{
  Plugin * plugin =
    instrument_track_get_instrument (self);
  g_return_if_fail (plugin);

  plugin->visible = !plugin->visible;

  EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED, plugin);
}
