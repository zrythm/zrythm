// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_WINDOWS_ERRORS_H__
#define __UTILS_WINDOWS_ERRORS_H__

#ifdef G_OS_WIN32

#  include <windows.h>

void
windows_errors_get_last_error_str (char * str);

#endif // G_OS_WIN32

#endif
