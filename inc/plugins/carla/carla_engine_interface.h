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
 * Carla engine C interface.
 */

#include "config.h"

#ifdef HAVE_CARLA

#ifndef __PLUGINS_CARLA_ENGINE_INTERFACE_H__
#define __PLUGINS_CARLA_ENGINE_INTERFACE_H__

#include "CarlaBackend.h"
#include "CarlaNativePlugin.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void * CarlaEngineHandle;
typedef void * CarlaPluginHandle;

CarlaEngineHandle
carla_engine_get_from_native_plugin (
  const NativePluginDescriptor * descr,
  NativePluginHandle             handle);

int
carla_engine_add_plugin_simple (
  CarlaEngineHandle  engine,
  const int          plugin_type,
  const char * const filename,
  const char * const name,
  const char * const label,
  const int64_t      unique_id,
  const void * const extra);

void
carla_engine_set_callback (
  CarlaEngineHandle  engine,
  void *             func,
  void * const       ptr);

CarlaPluginHandle
carla_engine_get_plugin (
  CarlaEngineHandle  engine,
  const unsigned int id);

#ifdef __cplusplus
}
#endif

#endif // header guard

#endif // HAVE_CARLA
