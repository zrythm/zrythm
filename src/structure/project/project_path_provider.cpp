// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project_path_provider.h"
#include "utils/utf8_string.h"

namespace zrythm::structure::project
{

std::filesystem::path
ProjectPathProvider::get_path (ProjectPath path)
{
  switch (path)
    {
    case ProjectPath::BackupsDir:
      return PROJECT_BACKUPS_DIR;
    case ProjectPath::ExportsDir:
      return PROJECT_EXPORTS_DIR;
    case ProjectPath::ExportStemsDir:
      return utils::Utf8String::from_utf8_encoded_string (PROJECT_EXPORTS_DIR)
               .to_path ()
             / PROJECT_STEMS_DIR;
    case ProjectPath::AudioFilePoolDir:
      return PROJECT_POOL_DIR;
    case ProjectPath::ProjectFile:
      return PROJECT_FILE;
    }
  throw std::runtime_error ("Invalid path type.");
}
}
