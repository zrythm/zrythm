// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::gui::backend
{
class TrackSelectionManager
    : public QObject,
      public utils::UuidIdentifiableObjectSelectionManager<
        structure::tracks::TrackRegistry>
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  DEFINE_UUID_IDENTIFIABLE_OBJECT_SELECTION_MANAGER_QML_PROPERTIES (
    TrackSelectionManager,
    zrythm::structure::tracks::Track)

public:
  TrackSelectionManager (const RegistryType &registry, QObject * parent = nullptr)
      : QObject (parent), UuidIdentifiableObjectSelectionManager (registry)
  {
  }
};
}
