// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/midi_note.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class MockArrangerObjectOwner : public QObject, public ArrangerObjectOwner<MidiNote>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    midiNotes,
    MidiNote)
  QML_ELEMENT

public:
  MockArrangerObjectOwner (
    ArrangerObjectRegistry       &registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    QObject *                     parent = nullptr)
      : QObject (parent),
        ArrangerObjectOwner (registry, file_audio_source_registry, *this)
  {
  }

  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const MidiNote * obj),
    (const override));
};

} // namespace zrythm::structure::arrangement
