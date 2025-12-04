// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/qquick/qfuture_qml_wrapper.h"

class Project;

class ProjectExporter : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  Q_INVOKABLE static gui::qquick::QFutureQmlWrapper *
  exportAudio (Project * project);
};
