// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/datetime.h"
#include "common/utils/io.h"
#include "common/utils/logger.h"
#include "gui/backend/backend/project_info.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#include <glib/gi18n.h>

ProjectInfo::ProjectInfo (const std::string &name, const std::string &filename)
    : name_ (name)
{
  if (filename.empty ())
    {
      filename_ = "-";
      modified_ = 0;
      modified_str_ = "-";
    }
  else
    {
      filename_ = filename;
      modified_ = io_file_get_last_modified_datetime (filename.c_str ());
      if (modified_ == -1)
        {
          modified_str_ = PROJECT_INFO_FILE_NOT_FOUND_STR;
          modified_ = G_MAXINT64;
        }
      else
        {
          modified_str_ = datetime_epoch_to_str (modified_).c_str ();
        }
      z_return_if_fail (!modified_str_.empty ());
    }
}

void
ProjectInfo::destroy_func (void * data)
{
  delete static_cast<ProjectInfo *> (data);
}