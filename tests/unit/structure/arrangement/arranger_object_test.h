// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object.h"

// Mock ArrangerObject with public constructor
class MockArrangerObject : public zrythm::structure::arrangement::ArrangerObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  MockArrangerObject (
    Type                         type,
    const zrythm::dsp::TempoMap &tempo_map,
    QObject *                    parent = nullptr)
      : ArrangerObject (type, tempo_map, parent)
  {
  }
};
