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

#include "settings_manager.h"
#include "zrythm_system.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "gui/widget_manager.h"
#include "plugins/plugin_manager.h"

void
zrythm_system_init ()
{
  zrythm_system = malloc (sizeof (Zrythm_System));

  init_settings_manager ();

  init_widget_manager ();

  plugin_manager_init ();

  init_audio_engine ();
}
