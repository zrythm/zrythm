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
  ProjectInfo (const std::string &name, const std::string &filename);

  static void destroy_func (void * data);

  std::string name_;
  /** Full path. */
  std::string filename_;
  RtTimePoint modified_ = 0;
  std::string modified_str_;
};

#define PROJECT_INFO_FILE_NOT_FOUND_STR QObject::tr ("<File not found>")

/**
 * @}
 */

#endif // __GUI_BACKEND_PROJECT_INFO_H__
