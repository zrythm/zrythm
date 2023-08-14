// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2015 Tim Mayberry <mojofunk@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "zrythm-config.h"

#ifdef _WOE32

#  include "dsp/windows_mmcss.h"

#  include <gtk/gtk.h>

typedef HANDLE (WINAPI * AvSetMmThreadCharacteristicsA_t) (
  LPCSTR  TaskName,
  LPDWORD TaskIndex);

typedef BOOL (WINAPI * AvRevertMmThreadCharacteristics_t) (
  HANDLE AvrtHandle);

typedef BOOL (WINAPI * AvSetMmThreadPriority_t) (
  HANDLE        AvrtHandle,
  AVRT_PRIORITY Priority);

static HMODULE avrt_dll = NULL;

static AvSetMmThreadCharacteristicsA_t
  AvSetMmThreadCharacteristicsA = NULL;
static AvRevertMmThreadCharacteristics_t
  AvRevertMmThreadCharacteristics = NULL;
static AvSetMmThreadPriority_t AvSetMmThreadPriority = NULL;

int
windows_mmcss_initialize ()
{
  if (avrt_dll != NULL)
    return 0;

  avrt_dll = LoadLibraryA ("avrt.dll");

  if (avrt_dll == NULL)
    {
      g_critical ("Unable to load avrt.dll");
      return -1;
    }
  int unload_dll = 0;

  AvSetMmThreadCharacteristicsA =
    (AvSetMmThreadCharacteristicsA_t) GetProcAddress (
      avrt_dll, "AvSetMmThreadCharacteristicsA");

  if (AvSetMmThreadCharacteristicsA == NULL)
    {
      g_critical (
        "Unable to resolve "
        "AvSetMmThreadCharacteristicsA");
      unload_dll = 1;
    }

  AvRevertMmThreadCharacteristics =
    (AvRevertMmThreadCharacteristics_t) GetProcAddress (
      avrt_dll, "AvRevertMmThreadCharacteristics");

  if (AvRevertMmThreadCharacteristics == NULL)
    {
      g_critical (
        "Unable to resolve "
        "AvRevertMmThreadCharacteristics");
      unload_dll = 1;
    }

  AvSetMmThreadPriority = (AvSetMmThreadPriority_t)
    GetProcAddress (avrt_dll, "AvSetMmThreadPriority");

  if (AvSetMmThreadPriority == NULL)
    {
      g_critical ("Unable to resolve AvSetMmThreadPriority");
      unload_dll = 1;
    }

  if (unload_dll)
    {
      g_warning (
        "MMCSS Unable to resolve necessary symbols, "
        "unloading avrt.dll");
      windows_mmcss_deinitialize ();
    }

  return 0;
}

int
windows_mmcss_deinitialize (void)
{
  if (avrt_dll == NULL)
    return 0;

  if (FreeLibrary (avrt_dll) == 0)
    {
      g_critical ("Unable to unload avrt.dll");
      return -1;
    }

  avrt_dll = NULL;

  AvSetMmThreadCharacteristicsA = NULL;
  AvRevertMmThreadCharacteristics = NULL;
  AvSetMmThreadPriority = NULL;

  return 0;
}

int
windows_mmcss_set_thread_characteristics (
  const char * task_name,
  HANDLE *     task_handle)
{
  if (AvSetMmThreadCharacteristicsA == NULL)
    return -1;

  DWORD task_index_dummy = 0;

  *task_handle = AvSetMmThreadCharacteristicsA (
    (char *) task_name, &task_index_dummy);

  if (*task_handle == 0)

    {
      g_critical (
        "Failed to set Thread Characteristics to %s",
        task_name);
      DWORD error = GetLastError ();

      switch (error)
        {
        case MMCSS_ERROR_INVALID_TASK_INDEX:
          g_critical ("MMCSS: Invalid Task Index");
          break;
        case MMCSS_ERROR_INVALID_TASK_NAME:
          g_critical ("MMCSS: Invalid Task Name");
          break;
        case ERROR_PRIVILEGE_NOT_HELD:
          g_critical ("MMCSS: Privilege not held");
          break;
        default:
          g_critical (
            "MMCSS: Unknown error setting thread "
            "characteristics");
          break;
        }
      return -1;
    }

  g_message (
    "MMCSS: Set thread characteristics to %s", task_name);

  return 0;
}

int
windows_mmcss_revert_thread_characteristics (
  HANDLE task_handle)
{
  if (AvRevertMmThreadCharacteristics == NULL)
    return -1;

  if (AvRevertMmThreadCharacteristics (task_handle) == 0)
    {
      g_critical (
        "MMCSS: Failed to set revert thread "
        "characteristics");
      return -1;
    }

  g_message ("MMCSS: Reverted thread characteristics");

  return 0;
}

int
windows_mmcss_set_thread_priority (
  HANDLE        task_handle,
  AVRT_PRIORITY priority)
{
  if (AvSetMmThreadPriority == NULL)
    return -1;

  if (AvSetMmThreadPriority (task_handle, priority) == 0)
    {
      g_critical (
        "Failed to set thread priority %i", priority);
      return -1;
    }

  g_message ("Set thread priority to %i", priority);

  return 0;
}

#endif // _WOE32
