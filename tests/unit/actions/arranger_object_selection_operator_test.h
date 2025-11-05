// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::actions
{
class MockArrangerObjectOwner
    : public QObject,
      public structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiNote>,
      public structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::Marker>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    midiNotes,
    zrythm::structure::arrangement::MidiNote)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    markers,
    zrythm::structure::arrangement::Marker)

public:
  MockArrangerObjectOwner (
    structure::arrangement::ArrangerObjectRegistry &registry,
    dsp::FileAudioSourceRegistry                   &file_audio_source_registry,
    QObject *                                       parent = nullptr)
      : QObject (parent),
        ArrangerObjectOwner<structure::arrangement::MidiNote> (
          registry,
          file_audio_source_registry,
          *this),
        ArrangerObjectOwner<structure::arrangement::Marker> (
          registry,
          file_audio_source_registry,
          *this)
  {
  }

  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const structure::arrangement::MidiNote * obj),
    (const override));
  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const structure::arrangement::Marker * obj),
    (const override));
};

}
