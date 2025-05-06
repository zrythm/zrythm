// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project_info.h"
#include "utils/datetime.h"
#include "utils/io.h"
#include "utils/logger.h"

using namespace zrythm;

ProjectInfo::ProjectInfo (utils::Utf8String name, const fs::path &filename)
    : name_ (std::move (name))
{
  if (filename.empty ())
    {
      filename_ = u8"-";
      modified_ = 0;
      modified_str_ = u8"-";
    }
  else
    {
      filename_ = filename;
      modified_ = utils::io::file_get_last_modified_datetime (filename);
      if (modified_ == -1)
        {
          modified_str_ = utils::Utf8String::from_qstring (
            get_project_info_file_not_found_string ());
          modified_ = std::numeric_limits<int64_t>::max ();
        }
      else
        {
          modified_str_ = utils::datetime::epoch_to_str (modified_);
        }
      z_return_if_fail (!modified_str_.empty ());
    }
}
