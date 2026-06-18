// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_preset.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

TEST (ChordPresetTest, DefaultConstruction)
{
  ChordPreset preset;
  EXPECT_TRUE (preset.name ().isEmpty ());
  EXPECT_TRUE (preset.category ().isEmpty ());
  EXPECT_FALSE (preset.isBuiltin ());
  EXPECT_TRUE (preset.descriptors ().empty ());
}

TEST (ChordPresetTest, NamedConstruction)
{
  ChordPreset preset (QString::fromUtf8 ("Test Preset"));
  EXPECT_EQ (preset.name (), QString::fromUtf8 ("Test Preset"));
}

TEST (ChordPresetTest, PropertySetters)
{
  ChordPreset preset;

  preset.setName (QString::fromUtf8 ("My Preset"));
  EXPECT_EQ (preset.name (), QString::fromUtf8 ("My Preset"));

  preset.setCategory (QString::fromUtf8 ("Euro Pop"));
  EXPECT_EQ (preset.category (), QString::fromUtf8 ("Euro Pop"));

  ChordPreset builtin (
    QString::fromUtf8 ("Builtin"), QString::fromUtf8 ("Pop"), true);
  EXPECT_TRUE (builtin.isBuiltin ());
}

TEST (ChordPresetTest, Equality)
{
  ChordPreset a;
  a.setName (QString::fromUtf8 ("Preset"));
  a.addDescriptor (
    zrythm::utils::make_qobject_unique<ChordDescriptor> (
      MusicalNote::C, ChordType::Major));

  ChordPreset b;
  b.setName (QString::fromUtf8 ("Preset"));
  b.addDescriptor (
    zrythm::utils::make_qobject_unique<ChordDescriptor> (
      MusicalNote::C, ChordType::Major));

  EXPECT_EQ (a, b);

  b.setName (QString::fromUtf8 ("Different"));
  EXPECT_NE (a, b);
}

TEST (ChordPresetTest, EqualityDifferentDescriptors)
{
  ChordPreset a;
  a.setName (QString::fromUtf8 ("Preset"));
  a.addDescriptor (
    zrythm::utils::make_qobject_unique<ChordDescriptor> (
      MusicalNote::C, ChordType::Major));

  ChordPreset b;
  b.setName (QString::fromUtf8 ("Preset"));
  b.addDescriptor (
    zrythm::utils::make_qobject_unique<ChordDescriptor> (
      MusicalNote::D, ChordType::Minor));

  EXPECT_NE (a, b);
}

TEST (ChordPresetTest, SerializationRoundTrip)
{
  ChordPreset original (
    QString::fromUtf8 ("Test Preset"), QString::fromUtf8 ("Rock"), true);
  original.addDescriptor (
    zrythm::utils::make_qobject_unique<ChordDescriptor> (
      MusicalNote::A, ChordType::Minor));
  original.addDescriptor (
    zrythm::utils::make_qobject_unique<ChordDescriptor> (
      MusicalNote::F, ChordType::Major, ChordAccent::Seventh));

  nlohmann::json j;
  to_json (j, original);

  ChordPreset loaded;
  from_json (j, loaded);

  EXPECT_EQ (loaded.name (), original.name ());
  EXPECT_EQ (loaded.category (), original.category ());
  EXPECT_EQ (loaded.descriptors ().size (), original.descriptors ().size ());
  EXPECT_EQ (loaded.descriptors ()[0]->rootNote (), MusicalNote::A);
  EXPECT_EQ (loaded.descriptors ()[0]->chordType (), ChordType::Minor);
  EXPECT_EQ (loaded.descriptors ()[1]->rootNote (), MusicalNote::F);
  EXPECT_EQ (loaded.descriptors ()[1]->chordType (), ChordType::Major);
  EXPECT_EQ (loaded.descriptors ()[1]->chordAccent (), ChordAccent::Seventh);
}

TEST (ChordPresetTest, SerializationNoCategory)
{
  nlohmann::json j;
  j["name"] = "Old Preset";
  j["descriptors"] = nlohmann::json::array ();

  ChordPreset loaded;
  from_json (j, loaded);

  EXPECT_EQ (loaded.name (), QString::fromUtf8 ("Old Preset"));
  EXPECT_TRUE (loaded.category ().isEmpty ());
}

TEST (ChordPresetTest, InitFrom)
{
  ChordPreset original (
    QString::fromUtf8 ("Original"), QString::fromUtf8 ("Euro Pop"), true);
  original.addDescriptor (
    zrythm::utils::make_qobject_unique<ChordDescriptor> (
      MusicalNote::G, ChordType::Major));
  original.addDescriptor (
    zrythm::utils::make_qobject_unique<ChordDescriptor> (
      MusicalNote::E, ChordType::Minor, ChordAccent::None, 1, MusicalNote::B));

  ChordPreset clone;
  init_from (clone, original, zrythm::utils::ObjectCloneType::NewIdentity);

  EXPECT_EQ (clone.name (), original.name ());
  EXPECT_EQ (clone.category (), original.category ());
  EXPECT_EQ (clone.isBuiltin (), original.isBuiltin ());
  EXPECT_EQ (clone.descriptors ().size (), 2u);
  EXPECT_EQ (clone.descriptors ()[0]->rootNote (), MusicalNote::G);
  EXPECT_EQ (clone.descriptors ()[0]->chordType (), ChordType::Major);
  EXPECT_EQ (clone.descriptors ()[1]->rootNote (), MusicalNote::E);
  EXPECT_EQ (clone.descriptors ()[1]->chordType (), ChordType::Minor);
  EXPECT_EQ (clone.descriptors ()[1]->inversion (), 1);
  EXPECT_TRUE (clone.descriptors ()[1]->hasBass ());
  EXPECT_EQ (clone.descriptors ()[1]->bassNote (), MusicalNote::B);
}

}
