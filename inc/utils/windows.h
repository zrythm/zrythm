// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_WINDOWS_H__
#define __UTILS_WINDOWS_H__

#ifdef G_OS_WIN32

typedef enum WindowsSpecialPath
{
  WINDOWS_SPECIAL_PATH_APPDATA,
  WINDOWS_SPECIAL_PATH_COMMON_PROGRAM_FILES,
} WindowsSpecialPath;

char *
windows_get_special_path (WindowsSpecialPath path_type);

#endif // G_OS_WIN32

#endif
