// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object.h"

// Mock ArrangerObject with public constructor
class MockArrangerObject
    : public QObject,
      public zrythm::structure::arrangement::ArrangerObject
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (MockArrangerObject)
  QML_ELEMENT

public:
  MockArrangerObject (
    Type                         type,
    const zrythm::dsp::TempoMap &tempo_map,
    QObject                     &parent)
      : ArrangerObject (type, tempo_map, parent)
  {
  }
};
