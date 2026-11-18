// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <system_error>

#include "structure/project/project_directory.h"
#include "structure/project/project_path_provider.h"
#include "utils/utf8_string.h"

#include <QDir>
#include <QFileInfo>

namespace zrythm::structure::project
{

DirectoryContent
classify_directory (const std::filesystem::path &dir)
{
  std::error_code ec;
  const auto      status = std::filesystem::status (dir, ec);
  // A missing path reports ENOENT both as an error code and as a
  // not-found status, so the status decides first
  if (!std::filesystem::exists (status))
    return DirectoryContent::NonExistent;
  if (ec)
    return DirectoryContent::Unknown;
  if (!std::filesystem::is_directory (status))
    return DirectoryContent::OtherContent;

  const QString dir_path = utils::Utf8String::from_path (dir).to_qstring ();
  if (!QFileInfo (dir_path).isReadable ())
    return DirectoryContent::Unknown;

  // The platform resolves the file name, so its own case rules apply
  const QFileInfo project_file (QDir (dir_path).filePath (
    utils::Utf8String::from_path (
      ProjectPathProvider::get_path (
        ProjectPathProvider::ProjectPath::ProjectFile))
      .to_qstring ()));
  if (project_file.isFile ())
    return DirectoryContent::ContainsProjectFile;
  return QDir (dir_path).isEmpty ()
           ? DirectoryContent::Empty
           : DirectoryContent::OtherContent;
}

} // namespace zrythm::structure::project
