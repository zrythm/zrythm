// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_PROJECT_INFO_H__
#define __GUI_BACKEND_PROJECT_INFO_H__

#include "utils/types.h"

/**
 * @addtogroup gui_backend
 * @{
 */

/**
 * Project file information.
 */
struct ProjectInfo
{
  ProjectInfo (utils::Utf8String name, const fs::path &filename);

  static auto get_project_info_file_not_found_string ()
  {
    return QObject::tr ("<File not found>");
  }

  utils::Utf8String name_;
  /** Full path. */
  fs::path          filename_;
  RtTimePoint       modified_ = 0;
  utils::Utf8String modified_str_;
};

/**
 * @}
 */

#endif // __GUI_BACKEND_PROJECT_INFO_H__
