// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class MockArrangerObjectOwner
    : public QObject,
      public ArrangerObjectOwner<MidiNote>,
      public ArrangerObjectOwner<Marker>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    midiNotes,
    MidiNote)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    markers,
    Marker)

public:
  MockArrangerObjectOwner (
    ArrangerObjectRegistry       &registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    QObject *                     parent = nullptr)
      : QObject (parent),
        ArrangerObjectOwner<MidiNote> (registry, file_audio_source_registry, *this),
        ArrangerObjectOwner<Marker> (registry, file_audio_source_registry, *this)
  {
  }

  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const MidiNote * obj),
    (const override));
  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const Marker * obj),
    (const override));
};

} // namespace zrythm::structure::arrangement
