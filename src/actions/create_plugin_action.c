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

#include "actions/create_plugin_action.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"

#include <glib/gi18n.h>

UndoableAction *
create_plugin_action_new (
  PluginDescriptor *  descr,
  Channel * ch,
  int       slot)
{
	CreatePluginAction * self =
    calloc (1, sizeof (
    	CreatePluginAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_CREATE_PLUGIN;
  plugin_clone_descr (descr, &self->descr);
  self->slot = slot;
  self->ch_id = ch->id;
  return ua;
}

int
create_plugin_action_do (
	CreatePluginAction * self)
{
  Plugin * pl =
    plugin_create_from_descr (&self->descr);
  g_warn_if_fail (pl);

  if (plugin_instantiate (pl))
    {
      GtkDialogFlags flags =
        GTK_DIALOG_DESTROY_WITH_PARENT;
      GtkWidget * dialog =
        gtk_message_dialog_new (
          GTK_WINDOW (MAIN_WINDOW),
          flags,
          GTK_MESSAGE_ERROR,
          GTK_BUTTONS_CLOSE,
          _("Error instantiating plugin %s. "
            "Please see log for details."),
          pl->descr->name);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      plugin_free (pl);
      return -1;
    }

  Channel * ch =
    project_get_channel (self->ch_id);
  g_warn_if_fail (ch);

  /* add to specific channel */
  channel_add_plugin (
    ch, self->slot, pl, 1, 1, 1);

  if (g_settings_get_int (
        S_PREFERENCES,
        "open-plugin-uis-on-instantiate"))
    {
      pl->visible = 1;
      EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED,
                   pl);
    }

  return 0;
}

/**
 * Deletes the plugin.
 */
int
create_plugin_action_undo (
	CreatePluginAction * self)
{
  Channel * ch =
    project_get_channel (self->ch_id);
  g_warn_if_fail (ch);

  channel_remove_plugin (ch, self->slot, 1, 0, 1);

  return 0;
}

void
create_plugin_action_free (
	CreatePluginAction * self)
{
  free (self);
}

