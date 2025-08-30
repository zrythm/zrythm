// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin.h"

namespace zrythm::gui::backend
{
class PluginSelectionManager
    : public QObject,
      public utils::UuidIdentifiableObjectSelectionManager<plugins::PluginRegistry>
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  DEFINE_UUID_IDENTIFIABLE_OBJECT_SELECTION_MANAGER_QML_PROPERTIES (
    PluginSelectionManager,
    zrythm::plugins::Plugin)

public:
  PluginSelectionManager (
    const RegistryType &registry,
    QObject *           parent = nullptr)
      : QObject (parent), UuidIdentifiableObjectSelectionManager (registry)
  {
  }
};
}
