// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "project_info.h"

using namespace zrythm::gui;

ProjectInfo::ProjectInfo (QObject * parent) : QObject (parent) { }

ProjectInfo::ProjectInfo (const fs::path &path, QObject * parent)
    : QObject (parent), path_ (path)
{
  if (exists ())
    {
      name_ = QString::fromStdString (path.filename ().string ());
      const auto time_since_epoch =
        fs::last_write_time (path).time_since_epoch ();
      const auto secs_since_epoch =
        std::chrono::duration_cast<std::chrono::seconds> (time_since_epoch)
          .count ();
      last_saved_at_ = QDateTime::fromSecsSinceEpoch (secs_since_epoch);
    }
}

bool
ProjectInfo::exists () const
{
  return fs::exists (path_) && !fs::is_directory (path_);
}

QString
ProjectInfo::getName () const
{
  return name_;
}

QString
ProjectInfo::getPath () const
{
  return QString::fromStdString (path_.string ());
}

QDateTime
ProjectInfo::getLastSavedAt () const
{
  return last_saved_at_;
}

void
ProjectInfo::setPath (const QString &name)
{
  auto converted_path = utils::io::to_fs_path (name);
  if (converted_path == path_)
    return;

  path_ = utils::io::to_fs_path (name);
  Q_EMIT pathChanged ();
}

void
ProjectInfo::setName (const QString &name)
{
  if (name_ == name)
    return;
  name_ = name;
  Q_EMIT nameChanged ();
}