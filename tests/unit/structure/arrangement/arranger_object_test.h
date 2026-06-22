// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/arranger_object.h"

namespace zrythm
{
// Mock ArrangerObject with public constructor
class MockArrangerObject : public structure::arrangement::ArrangerObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  using ArrangerObjectFeatures =
    structure::arrangement::ArrangerObject::ArrangerObjectFeatures;

  MockArrangerObject (
    Type                        type,
    const dsp::TempoMapWrapper &tempo_map_wrapper,
    ArrangerObjectFeatures      features = ArrangerObjectFeatures::Bounds,
    QObject *                   parent = nullptr)
      : ArrangerObject (type, tempo_map_wrapper, features, parent)
  {
  }
};
}
