// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "utils/iobject_registry.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::actions
{
class MockArrangerObjectOwner
    : public QObject,
      public structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiNote>,
      public structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::Marker>,
      public structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>,
      public structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::AudioClip>,
      public structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::AutomationPoint>,
      public structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::TempoObject>,
      public structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::TimeSignatureObject>
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
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    midiClips,
    zrythm::structure::arrangement::MidiClip)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    audioClips,
    zrythm::structure::arrangement::AudioClip)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    automationPoints,
    zrythm::structure::arrangement::AutomationPoint)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    tempoObjects,
    zrythm::structure::arrangement::TempoObject)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MockArrangerObjectOwner,
    timeSignatureObjects,
    zrythm::structure::arrangement::TimeSignatureObject)

public:
  MockArrangerObjectOwner (
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr)
      : QObject (parent),
        ArrangerObjectOwner<structure::arrangement::MidiNote> (registry, *this),
        ArrangerObjectOwner<structure::arrangement::Marker> (registry, *this),
        ArrangerObjectOwner<structure::arrangement::MidiClip> (registry, *this),
        ArrangerObjectOwner<structure::arrangement::AudioClip> (registry, *this),
        ArrangerObjectOwner<
          structure::arrangement::AutomationPoint> (registry, *this),
        ArrangerObjectOwner<structure::arrangement::TempoObject> (registry, *this),
        ArrangerObjectOwner<
          structure::arrangement::TimeSignatureObject> (registry, *this)
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
  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const structure::arrangement::AudioClip * obj),
    (const override));
  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const structure::arrangement::MidiClip * obj),
    (const override));
  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const structure::arrangement::AutomationPoint * obj),
    (const override));
  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const structure::arrangement::TempoObject * obj),
    (const override));
  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const structure::arrangement::TimeSignatureObject * obj),
    (const override));
};

}
