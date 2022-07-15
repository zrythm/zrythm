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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
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
 */

#include "zrythm-config.h"

#ifdef _WOE32

#  ifndef __AUDIO_WINDOWS_MMCSS_H__
#    define __AUDIO_WINDOWS_MMCSS_H__

#    include <windows.h>

typedef enum AVRT_PRIORITY
{
  AVRT_PRIORITY_VERYLOW = -2,
  AVRT_PRIORITY_LOW,
  AVRT_PRIORITY_NORMAL,
  AVRT_PRIORITY_HIGH,
  AVRT_PRIORITY_CRITICAL
} AVRT_PRIORITY;

enum error_codes
{
  ERROR_INVALID_TASK_NAME = 1550,
  ERROR_INVALID_TASK_INDEX = 1551
};

int
windows_mmcss_initialize (void);

int
windows_mmcss_deinitialize (void);

int
windows_mmcss_set_thread_characteristics (
  const char * task_name,
  HANDLE *     task_handle);

int
windows_mmcss_revert_thread_characteristics (
  HANDLE task_handle);

int windows_mmcss_set_thread_priority (HANDLE, AVRT_PRIORITY);

#  endif // __AUDIO_WINDOWS_MMCSS_H__
#endif   // _WOE32
