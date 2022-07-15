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

#ifdef _WOE32

#  include "utils/windows_errors.h"

#  include <gtk/gtk.h>

#  if 0
void
windows_errors_print_mmresult (
  MMRESULT res)
{
  switch (res)
    {
    case MMSYSERR_ALLOCATED:
      g_critical (
        "MMSYSERR_ALLOCATED: The specified resource is already allocated");
      break;
    case MMSYSERR_BADDEVICEID:
      g_critical (
        "MMSYSERR_BADDEVICEID: The specified device identifier is out of range");
      break;
    case MMSYSERR_INVALFLAG:
      g_critical (
        "MMSYSERR_INVALFLAG: The flags specified by dwFlags are invalid");
      break;
    case MMSYSERR_INVALPARAM:
      g_critical (
        "MMSYSERR_INVALPARAM:  The specified pointer or structure is invalid");
      break;
    case MMSYSERR_NODRIVER:
      g_critical (
        "MMSYSERR_NODRIVER: The driver is not installed");
      break;
    case MMSYSERR_NOMEM:
      g_critical (
        "MMSYSERR_NOMEM: The system is unable to allocate or lock memory");
      break;
    default:
      g_warning ("Unknown error %u received", res);
      break;
    }
}
#  endif

void
windows_errors_get_last_error_str (char * str)
{
  DWORD error_id = GetLastError ();
  LPSTR buf = NULL;
  FormatMessageA (
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
      | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, error_id, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPSTR) &buf, 0, NULL);
  strcpy (str, buf);
  LocalFree (buf);
}

#endif // _WOE32
