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

/**
 * \file
 *
 * Carla plugin C interface.
 */

#include "config.h"

#ifdef HAVE_CARLA

#ifndef __PLUGINS_CARLA_PLUGIN_INTERFACE_H__
#define __PLUGINS_CARLA_PLUGIN_INTERFACE_H__

#include "CarlaBackend.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void * CarlaPluginHandle;

void
carla_plugin_show_custom_ui (
  CarlaPluginHandle handle,
  const int         show);

void
carla_plugin_ui_idle (
  CarlaPluginHandle handle);

void
carla_plugin_process (
  CarlaPluginHandle handle,
  const float ** const audio_in,
  float ** const audio_out,
  const float ** const cv_in,
  float ** const cv_out,
  const uint32_t frames);

void
carla_plugin_get_real_name (
  CarlaPluginHandle handle,
  char * const      name);

void
carla_plugin_get_maker (
  CarlaPluginHandle handle,
  char * const      name);

int
carla_plugin_get_category (
  CarlaPluginHandle handle);

uint32_t
carla_plugin_get_audio_in_count (
  CarlaPluginHandle handle);

uint32_t
carla_plugin_get_audio_out_count (
  CarlaPluginHandle handle);

uint32_t
carla_plugin_get_cv_in_count (
  CarlaPluginHandle handle);

uint32_t
carla_plugin_get_cv_out_count (
  CarlaPluginHandle handle);

uint32_t
carla_plugin_get_midi_in_count (
  CarlaPluginHandle handle);

uint32_t
carla_plugin_get_midi_out_count (
  CarlaPluginHandle handle);

uint32_t
carla_plugin_get_parameter_count (
  CarlaPluginHandle handle);

uint32_t
carla_plugin_get_parameter_in_count (
  CarlaPluginHandle handle);

uint32_t
carla_plugin_get_parameter_out_count (
  CarlaPluginHandle handle);

int
carla_plugin_save_state_to_file (
  CarlaPluginHandle handle,
  const char * const filename);

int
carla_plugin_load_state_from_file (
  CarlaPluginHandle handle,
  const char * const filename);

#ifdef __cplusplus
}
#endif

#endif // header guard

#endif // HAVE_CARLA_NATIVE_PLUGIN
