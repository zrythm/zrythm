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

#include "config.h"

#ifdef HAVE_CARLA

#include "plugins/carla/carla_plugin_interface.h"
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

void
carla_plugin_get_real_name (
  CarlaPluginHandle handle,
  char * const      name)
{
  GET_PLUGIN;
  plugin->getRealName (name);
}

void
carla_plugin_get_maker (
  CarlaPluginHandle handle,
  char * const      name)
{
  GET_PLUGIN;
  plugin->getMaker (name);
}


int
carla_plugin_get_category (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  return plugin->getCategory ();
}

uint32_t
carla_plugin_get_audio_in_count (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  return plugin->getAudioInCount ();
}

uint32_t
carla_plugin_get_audio_out_count (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  return plugin->getAudioOutCount ();
}

uint32_t
carla_plugin_get_cv_in_count (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  return plugin->getCVInCount ();
}

uint32_t
carla_plugin_get_cv_out_count (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  return plugin->getCVOutCount ();
}

uint32_t
carla_plugin_get_midi_in_count (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  return plugin->getMidiInCount ();
}

uint32_t
carla_plugin_get_midi_out_count (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  return plugin->getMidiOutCount ();
}

uint32_t
carla_plugin_get_parameter_count (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  return plugin->getParameterCount ();
}

uint32_t
carla_plugin_get_parameter_in_count (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  uint32_t ins, outs;
  plugin->getParameterCountInfo (
    ins, outs);
  return ins;
}

uint32_t
carla_plugin_get_parameter_out_count (
  CarlaPluginHandle handle)
{
  GET_PLUGIN;
  uint32_t ins, outs;
  plugin->getParameterCountInfo (
    ins, outs);
  return outs;
}

int
carla_plugin_save_state_to_file (
  CarlaPluginHandle handle,
  const char * const filename)
{
  GET_PLUGIN;
  return plugin->saveStateToFile (filename);
}

int
carla_plugin_load_state_from_file (
  CarlaPluginHandle handle,
  const char * const filename)
{
  GET_PLUGIN;
  return plugin->loadStateFromFile (filename);
}

}

#endif // HAVE_CARLA
