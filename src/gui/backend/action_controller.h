// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/alert_manager.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/theme_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/project_manager.h"

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