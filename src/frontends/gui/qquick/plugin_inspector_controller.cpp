// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "frontends/gui/qquick/plugin_inspector_controller.h"
#include "plugins/plugin.h"

namespace zrythm::frontends::gui::qquick
{

PluginInspectorController::PluginInspectorController (QObject * parent)
    : QObject (parent)
{
}

void
PluginInspectorController::showInspector (plugins::Plugin * plugin)
{
  Q_EMIT inspectorRequested (plugin);
}

} // namespace zrythm::frontends::gui::qquick
