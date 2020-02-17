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

#include "actions/move_plugins_action.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "gui/backend/mixer_selections.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"

#include <glib/gi18n.h>

UndoableAction *
move_plugins_action_new (
  MixerSelections * ms,
  Track *    to_tr,
  int        to_slot)
{
  MovePluginsAction * self =
    calloc (1, sizeof (
      MovePluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UA_MOVE_PLUGINS;

  self->to_slot = to_slot;
  Track * pl_track =
    plugin_get_track (ms->plugins[0]);
  g_warn_if_fail (pl_track);
  self->from_track_pos = pl_track->pos;
  if (to_tr)
    self->to_track_pos = to_tr->pos;
  else
    self->is_new_channel = 1;

  self->ms = mixer_selections_clone (ms);
  g_warn_if_fail (
    ms->plugins[0]->id.slot ==
      self->ms->plugins[0]->id.slot);

  return ua;
}

int
move_plugins_action_do (
  MovePluginsAction * self)
{
  Plugin * pl;
  Channel * from_ch =
    TRACKLIST->tracks[self->from_track_pos]->channel;
  Channel * to_ch =
    TRACKLIST->tracks[self->to_track_pos]->channel;
  g_return_val_if_fail (from_ch, -1);
  g_return_val_if_fail (to_ch, -1);

  /*int highest_slot =*/
    /*mixer_selections_get_highest_slot (self->ms);*/

  /* clear selections to readd each plugin moved */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

  /*int diff;*/
  int to_slot;
  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get the plugin */
      pl =
        from_ch->plugins[
          self->ms->plugins[i]->id.slot];
      g_warn_if_fail (
        pl &&
        pl->id.track_pos == from_ch->track_pos);

      /* get difference in slots */
      /*diff = self->ms->slots[i] - highest_slot;*/
      /*g_return_val_if_fail (diff > -1, -1);*/

      /* move and select plugin to to_slot + diff */
      to_slot = self->to_slot + i;
      mixer_move_plugin (
        MIXER, pl, to_ch, to_slot);

      mixer_selections_add_slot (
        MIXER_SELECTIONS, to_ch, to_slot);
    }

  return 0;
}

int
move_plugins_action_undo (
  MovePluginsAction * self)
{
  Plugin * pl;

  /* get original channel */
  Channel * ch =
    TRACKLIST->tracks[
      self->from_track_pos]->channel;
  g_return_val_if_fail (ch, -1);

  /* get the channel the plugins were moved to */
  Channel * current_ch =
    TRACKLIST->tracks[
      self->to_track_pos]->channel;
  g_return_val_if_fail (current_ch, -1);

  /* clear selections to readd each plugin moved */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

  for (int i = 0; i < self->ms->num_slots; i++)
    {
      /* get the actual plugin */
      pl = current_ch->plugins[self->to_slot + i];
      g_return_val_if_fail (pl, -1);

      /* move plugin to its original slot */
      mixer_move_plugin (
        MIXER, pl, ch,
        self->ms->plugins[i]->id.slot);

      /* add to mixer selections */
      mixer_selections_add_slot (
        MIXER_SELECTIONS, ch, pl->id.slot);
    }

  return 0;
}

char *
move_plugins_action_stringize (
  MovePluginsAction * self)
{
  if (self->ms->num_slots == 1)
    return g_strdup_printf (
      _("Move %s"),
      self->ms->plugins[0]->descr->name);
  else
    return g_strdup_printf (
      _("Move %d Plugins"),
      self->ms->num_slots);
}

void
move_plugins_action_free (
  MovePluginsAction * self)
{
  free (self);
}
