// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration>

namespace zrythm::gui
{

class AlertManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  explicit AlertManager (QObject * parent = nullptr);

  Q_INVOKABLE void showAlert (const QString &title, const QString &message);

  Q_SIGNAL void alertRequested (const QString &title, const QString &message);
};

} // namespace zrythm::gui
