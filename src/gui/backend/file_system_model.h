// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#pragma once

#include <QFileSystemModel>
#include <QtQmlIntegration>

namespace zrythm::gui
{
class FileSystemModel : public QFileSystemModel
{
  Q_OBJECT
  Q_PROPERTY (
    QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY
      rootIndexChanged)
  QML_ELEMENT

public:
  explicit FileSystemModel (QObject * parent = nullptr);

  enum Roles
  {
    FileSizeRole = Qt::UserRole + 1
  };

  Q_INVOKABLE static bool isDir (const QFileInfo * fileInfo)
  {
    return fileInfo->isDir ();
  }

  Q_INVOKABLE QMimeType getFileMimeType (const QString &filePath) const;
  Q_INVOKABLE QString   getFileInfoAsString (const QString &filePath) const;

  QModelIndex   rootIndex () const { return root_index_; }
  void          setRootIndex (const QModelIndex &index);
  Q_SIGNAL void rootIndexChanged ();

  int      columnCount (const QModelIndex &parent) const override;
  QVariant data (const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames () const override;

private:
  QModelIndex         root_index_;
  const QMimeDatabase mime_db_;
};
}
