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

#include "plugins/carla/plugin_interface.h"
#include "CarlaPlugin.hpp"

extern "C"
{

#define GET_PLUGIN \
  CarlaBackend::CarlaPlugin * plugin = \
    (CarlaBackend::CarlaPlugin *) handle

void
carla_plugin_show_custom_ui (
  CarlaPluginHandle handle,
  const int         show)
{
  GET_PLUGIN;
  plugin->showCustomUI (show);
}

void
carla_plugin_ui_idle (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  plugin->uiIdle ();
}

void
carla_plugin_process (
  CarlaPluginHandle handle,
  const float ** const audio_in,
  float ** const audio_out,
  const float ** const cv_in,
  float ** const cv_out,
  const uint32_t frames)
{
  GET_PLUGIN;
  plugin->process (
    audio_in, audio_out, cv_in, cv_out, frames);
}

}
