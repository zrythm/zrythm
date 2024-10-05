/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifdef _WIN32

#  include "common/utils/windows_errors.h"

#  include <gtk/gtk.h>

#  if 0
void
windows_errors_print_mmresult (
  MMRESULT res)
{
  switch (res)
    {
    case MMSYSERR_ALLOCATED:
      z_error (
        "MMSYSERR_ALLOCATED: The specified resource is already allocated");
      break;
    case MMSYSERR_BADDEVICEID:
      z_error (
        "MMSYSERR_BADDEVICEID: The specified device identifier is out of range");
      break;
    case MMSYSERR_INVALFLAG:
      z_error (
        "MMSYSERR_INVALFLAG: The flags specified by dwFlags are invalid");
      break;
    case MMSYSERR_INVALPARAM:
      z_error (
        "MMSYSERR_INVALPARAM:  The specified pointer or structure is invalid");
      break;
    case MMSYSERR_NODRIVER:
      z_error (
        "MMSYSERR_NODRIVER: The driver is not installed");
      break;
    case MMSYSERR_NOMEM:
      z_error (
        "MMSYSERR_NOMEM: The system is unable to allocate or lock memory");
      break;
    default:
      z_warning("Unknown error {} received", res);
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
    nullptr, error_id, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &buf,
    0, nullptr);
  strcpy (str, buf);
  LocalFree (buf);
}

#endif // _WIN32
