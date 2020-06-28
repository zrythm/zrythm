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

#include "actions/edit_plugins_action.h"
#include "audio/channel.h"
#include "audio/router.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"

#include <glib/gi18n.h>

void
edit_plugins_action_init_loaded (
  EditPluginsAction * self)
{
  mixer_selections_init_loaded (self->ms, false);
}

UndoableAction *
edit_plugins_action_new (
  MixerSelections *     ms,
  EditPluginsActionType type)
{
  EditPluginsAction * self =
    calloc (1, sizeof (
      EditPluginsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UA_EDIT_PLUGINS;

  self->ms =
    mixer_selections_clone (
      ms, ms == MIXER_SELECTIONS);
  self->type = type;

  return ua;
}

int
edit_plugins_action_do (
  EditPluginsAction * self)
{
  /* TODO */
  return 0;
}

/**
 * Edits the plugin.
 */
int
edit_plugins_action_undo (
  EditPluginsAction * self)
{
  /* TODO */
  return 0;
}

char *
edit_plugins_action_stringize (
  EditPluginsAction * self)
{
  g_return_val_if_reached (
    g_strdup (""));
}

void
edit_plugins_action_free (
  EditPluginsAction * self)
{
  free (self);
}
