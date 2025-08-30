// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"

namespace zrythm::gui::backend
{
class ArrangerObjectSelectionManager
    : public QObject,
      public utils::UuidIdentifiableObjectSelectionManager<
        structure::arrangement::ArrangerObjectRegistry>
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  DEFINE_UUID_IDENTIFIABLE_OBJECT_SELECTION_MANAGER_QML_PROPERTIES (
    ArrangerObjectSelectionManager,
    zrythm::structure::arrangement::ArrangerObject)

public:
  ArrangerObjectSelectionManager (
    const RegistryType &registry,
    QObject *           parent = nullptr)
      : QObject (parent), UuidIdentifiableObjectSelectionManager (registry)
  {
  }
};
}
