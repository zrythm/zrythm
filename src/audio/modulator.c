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

#include "audio/modulator.h"
#include "plugins/plugin.h"
#include "gui/widgets/modulator.h"

/**
 * Creates a new Modulator.
 *
 * @param descr The descriptor of a plugin to
 *   instantiate.
 * @param track The owner Track.
 */
Modulator *
modulator_new (
  PluginDescriptor * descr,
  Track *            track)
{
  Modulator * self = calloc (1, sizeof (Modulator));

  self->track = track;
  self->plugin =
    plugin_new_from_descr (
      /* FIXME pass correct slot */
      descr, track->pos, 0);
  g_warn_if_fail (self->plugin);
  self->plugin->id.track_pos = track->pos;
  plugin_generate_automation_tracks (
    self->plugin, track);
  int ret =
    plugin_instantiate (self->plugin, true, NULL);
  g_warn_if_fail (!ret);

  /* FIXME check if unique */
  self->name =
    g_strdup (descr->name);

  self->widget =
    modulator_widget_new (self);

  return self;
}
