/*
 * project.c - A project (or song), containing all the project data
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "zrythm_app.h"
#include "project.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/transport.h"
#include "plugins/lv2_plugin.h"
#include "utils/io.h"
#include "utils/smf.h"
#include "utils/xml.h"

#include <gtk/gtk.h>


void
project_setup (char * filename)
{

  // set title
  g_message ("Setting up project %s...", filename);
  PROJECT->title = g_strdup (filename);

  transport_init ();
  AUDIO_ENGINE->run = 1;
}

void
project_save (char * filename)
{
  /* write XML */
  char ** parts = g_strsplit (filename, ".", 2);
  char * file_part = parts[0];
  char * ext_part = parts[1];
  char * tmp = g_strdup_printf ("%s_ports.%s", file_part, ext_part);
  xml_write_ports (tmp);
  g_free (tmp);

  /*  write regions & their corresponding MIDI */
  smf_save_regions (filename);
  tmp = g_strdup_printf ("%s_regions.%s", file_part, ext_part);
  xml_write_regions (tmp);
  g_free (tmp);

  /* write plugin states */
  char * dir = io_get_dir (filename);
  io_mkdir (dir);
  char * separator = io_get_separator ();
  char * state_dir = g_strdup_printf ("%s%sstates",
                                      dir,
                                      separator);
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      char * state_dir_channel = g_strdup_printf ("%s%s%d",
                                          state_dir,
                                          separator,
                                          channel->id);
      for (int j = 0; j < MAX_PLUGINS; j++)
        {
          Plugin * plugin = channel->strip[j];

          if (plugin)
            {
              if (plugin->descr->protocol == PROT_LV2)
                {
                  char * state_dir_plugin = g_strdup_printf ("%s%s%d",
                                              state_dir_channel,
                                              separator,
                                              j);
                  LV2_Plugin * lv2_plugin = (LV2_Plugin *) plugin->original_plugin;
                  g_message ("symap size before saving state %d",
                             lv2_plugin->symap->size);
                  lv2_save_state (lv2_plugin,
                                  state_dir_plugin);
                  g_free (state_dir_plugin);
                }
              else
                {
                  //
                }
            }
        }
      g_free (state_dir_channel);
    }

  tmp = g_strdup_printf ("%s.%s", file_part, ext_part);
  xml_write_project (tmp);
  g_free (tmp);

  g_free (dir);
  g_free (separator);
  g_free (state_dir);
  g_strfreev (parts);
}

void
project_load (char * filename)
{

}
