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
#include "plugins/carla/carla_engine_interface.h"
#include <CarlaEngine.hpp>

extern "C"
{

CarlaEngineHandle
carla_engine_get_from_native_plugin (
  const NativePluginDescriptor * descr,
  NativePluginHandle             handle)
{
  return
    (CarlaEngineHandle)
    carla_get_native_plugin_engine (
      descr, handle);
}

#define GET_ENGINE \
  CarlaBackend::CarlaEngine * engine = \
    (CarlaBackend::CarlaEngine *) _engine

int
carla_engine_add_plugin_simple (
  CarlaEngineHandle  _engine,
  const int          plugin_type,
  const char * const filename,
  const char * const name,
  const char * const label,
  const int64_t      unique_id,
  const void * const extra)
{
  GET_ENGINE;
  return
    engine->addPlugin (
      (CarlaBackend::PluginType) plugin_type,
      filename, name, label,
      unique_id, extra);
}

void
carla_engine_set_callback (
  CarlaEngineHandle  _engine,
  void *             func,
  void * const       ptr)
{
  GET_ENGINE;
  engine->setCallback (
    (CarlaBackend::EngineCallbackFunc) func, ptr);
}

CarlaPluginHandle
carla_engine_get_plugin (
  CarlaEngineHandle  _engine,
  const unsigned int id)
{
  GET_ENGINE;
  return
    (CarlaPluginHandle)
    engine->getPlugin (id);
}

#undef GET_ENGINE

}
#endif
