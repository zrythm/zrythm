// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/types.h"

#include <QObject>
#include <QtQmlIntegration>

namespace zrythm::gui
{

class ProjectInfo : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY (QString name READ getName WRITE setName NOTIFY nameChanged FINAL)
  Q_PROPERTY (QString path READ getPath WRITE setPath NOTIFY pathChanged FINAL)
  Q_PROPERTY (QDateTime lastSavedAt READ getLastSavedAt CONSTANT FINAL)

public:
  explicit ProjectInfo (QObject * parent = nullptr);

  /**
   * @brief Construct a new Project Info object from a path.
   *
   * @param path
   * @param parent
   */
  ProjectInfo (const fs::path &path, QObject * parent = nullptr);

  QString   getName () const;
  QString   getPath () const;
  QDateTime getLastSavedAt () const;

  void setPath (const QString &path);
  void setName (const QString &name);

  /**
   * @brief Whether the project exists on disk.
   */
  Q_INVOKABLE bool exists () const;

Q_SIGNALS:
  void nameChanged ();
  void pathChanged ();

private:
  QString   name_;
  fs::path  path_;
  QDateTime last_saved_at_;
};

} // namespace zrythm::gui
