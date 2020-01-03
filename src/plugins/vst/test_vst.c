/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/vst_plugin.h"

#include <gtk/gtk.h>

#include <dlfcn.h>

static intptr_t
host_can_do (
  const char * feature)
{
  if (!strcmp (feature, "supplyIdle"))
    return 1;
  if (!strcmp (feature, "sendVstEvents"))
    return 1;
  if (!strcmp (feature, "sendVstMidiEvent"))
    return 1;
  if (!strcmp (feature, "sendVstMidiEventFlagIsRealtime"))
    return 1;
  if (!strcmp (feature, "sendVstTimeInfo"))
    return -1;
  if (!strcmp (feature, "receiveVstEvents"))
    return -1;
  if (!strcmp (feature, "receiveVstMidiEvent"))
    return -1;
  if (!strcmp (feature, "receiveVstTimeInfo"))
    return -1;
  if (!strcmp (feature, "reportConnectionChanges"))
    return -1;
  if (!strcmp (feature, "acceptIOChanges"))
    return 1;
  if (!strcmp (feature, "sizeWindow"))
    return 1;
  if (!strcmp (feature, "offline"))
    return -1;
  if (!strcmp (feature, "openFileSelector"))
    return -1;
  if (!strcmp (feature, "closeFileSelector"))
    return -1;
  if (!strcmp (feature, "startStopProcess"))
    return 1;
  if (!strcmp (feature, "supportShell"))
    return -1;
  if (!strcmp (feature, "shellCategory"))
    return 1;
  if (!strcmp (feature, "NIMKPIVendorSpecificCallbacks"))
    return -1;

  g_return_val_if_reached (0);
}

/**
 * Host callback implementation.
 */
static intptr_t
host_callback (
  AEffect * effect,
  int32_t   opcode,
  int32_t   index,
  intptr_t  value,
  void *    ptr,
  float     opt)
{
  g_message (
    "host callback called with opcode %d",
    opcode);

  switch (opcode)
    {
    case audioMasterVersion:
      return kVstVersion;
    case audioMasterGetVendorString:
      strcpy ((char *) ptr, "Alexandros Theodotou");
      return 1;
    case audioMasterGetProductString:
      strcpy ((char *) ptr, "Zrythm VST test");
      return 1;
    case audioMasterGetVendorVersion:
      return 900;
    case audioMasterCurrentId:
      return effect->uniqueID;
    case audioMasterCanDo:
      return host_can_do ((const char *) ptr);
    default:
      g_message ("Plugin requested value of opcode %d", opcode);
      break;
  }
  return 0;
}

int
main (int    argc,
      char **argv)
{
  char * path = argv[1];

  void * handle = dlopen (path, RTLD_NOW);
  if (!handle)
    {
      g_warning (
        "Failed to load VST plugin from %s: %s",
        path, dlerror ());
      return -1;
    }

  VstPluginMain entry_point =
    (VstPluginMain) dlsym (handle, "VSTPluginMain");
  if (!entry_point)
    entry_point =
      (VstPluginMain) dlsym (handle, "main");
  if (!entry_point)
    {
      g_warning (
        "Failed to get entry point from VST plugin "
        "from %s: %s",
        path, dlerror ());
      dlclose (handle);
      return -1;
    }

  AEffect * effect = entry_point (host_callback);

  if (!effect)
    {
      g_warning (
        "%s: Could not get entry point for "
        "VST plugin", path);
      dlclose (handle);
      return -1;
    }

  /* check plugin's magic number */
  if (effect->magic != kEffectMagic)
    {
      g_warning ("Plugin's magic number is invalid");
      dlclose (handle);
      return -1;
    }

  /* wait for the plugin to initialize */
  g_usleep (10000);

  return 0;
}
