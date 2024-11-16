// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * ---
 *
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ---
 *
 * Copyright (C) 2016-2021 VCV.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ---
 */

#ifdef _WIN32

#  include <shlobj.h>

#  include "common/utils/logger.h"
#  include "common/utils/windows.h"

char *
windows_get_special_path (WindowsSpecialPath path_type)
{
  int csidl;
  switch (path_type)
    {
    case WINDOWS_SPECIAL_PATH_APPDATA:
      csidl = CSIDL_APPDATA;
      break;
    case WINDOWS_SPECIAL_PATH_COMMON_PROGRAM_FILES:
      csidl = CSIDL_PROGRAM_FILES_COMMON;
      break;
    default:
      z_return_val_if_reached (nullptr);
    }

  WCHAR path[MAX_PATH + 256];

  if (SHGetSpecialFolderPathW (nullptr, path, csidl, FALSE))
    {
      char * ret = g_utf16_to_utf8 (
        (const gunichar2 *) path, -1, nullptr, nullptr, nullptr);
      z_return_val_if_fail (ret, nullptr);
      return ret;
    }

  z_return_val_if_reached (nullptr);
}

#endif // _WIN32
