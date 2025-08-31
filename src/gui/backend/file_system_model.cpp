// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#include "gui/backend/file_system_model.h"

namespace zrythm::gui
{
FileSystemModel::FileSystemModel (QObject * parent) : QFileSystemModel (parent)
{
  setRootPath (QDir::homePath ());
}

void
FileSystemModel::setRootIndex (const QModelIndex &index)
{
  if (index == root_index_)
    return;

  root_index_ = index;
  Q_EMIT rootIndexChanged ();
}

QMimeType
FileSystemModel::getFileMimeType (const QString &filePath) const
{
  if (filePath.isEmpty ())
    return {};

  QFileInfo nfo (filePath);
  return mime_db_.mimeTypeForFile (nfo);
}

QString
FileSystemModel::getFileInfoAsString (const QString &filePath) const
{
  if (filePath.isEmpty ())
    return {};

  const auto size_string = [] (const QFile &file) -> std::string {
    const auto size_in_bytes = file.size ();
    if (size_in_bytes < 1'000)
      return fmt::format ("{} bytes", size_in_bytes);
    if (size_in_bytes < 1'000'000)
      return fmt::format ("{} KB", size_in_bytes / 1'000);
    if (size_in_bytes < 1'000'000'000)
      return fmt::format ("{} MB", size_in_bytes / 1'000'000);
    return fmt::format ("{} GB", size_in_bytes / 1'000'000'000);
  };

  QFile     file (filePath);
  QFileInfo nfo (filePath);

  const QMimeType mime = mime_db_.mimeTypeForFile (nfo);

  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}\n{} | {}", filePath, mime.comment (), size_string (file)));
}

int
FileSystemModel::columnCount (const QModelIndex &parent) const
{
  return 1;
}

QVariant
FileSystemModel::data (const QModelIndex &index, int role) const
{
  switch (role)
    {
    default:
      return QFileSystemModel::data (index, role);
    }
}

QHash<int, QByteArray>
FileSystemModel::roleNames () const
{
  auto result = QFileSystemModel::roleNames ();
  return result;
}
}
