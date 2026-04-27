// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QtQmlIntegration/QtQmlIntegration>

/**
 * @brief Controller for actions.
 */
class ActionController : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  // Q_INVOKABLE void createEmptyTrack (int type);

public:
  ActionController (QObject * parent = nullptr);
};
