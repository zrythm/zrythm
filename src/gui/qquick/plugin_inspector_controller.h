// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QtQmlIntegration>

namespace zrythm::plugins
{
class Plugin;
}

namespace zrythm::gui::qquick
{

class PluginInspectorController : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  explicit PluginInspectorController (QObject * parent = nullptr);

  Q_INVOKABLE void showInspector (plugins::Plugin * plugin);

Q_SIGNALS:
  void inspectorRequested (plugins::Plugin * plugin);
};

} // namespace zrythm::gui::qquick
