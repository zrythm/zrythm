// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <optional>

#include "commands/transform_arranger_objects_command.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/in_memory_settings_backend.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

/// Content-tick literal for state construction and expectations.
constexpr dsp::ContentTick
ticks (double t)
{
  return dsp::ContentTick{ units::ticks (t) };
}

class TransformArrangerObjectsCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    app_settings = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    factory = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_wrapper,
        .registry_ = object_registry,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });

    note_ref =
      factory->get_builder<structure::arrangement::MidiNote> ()
        .build_in_registry ();
    auto * note = note_ref->get_object_as<structure::arrangement::MidiNote> ();
    note->position ()->set_ticks (ticks (1920));
    note->length ()->set_ticks (ticks (480));
    note->setPitch (60);
    note->setVelocity (40);
  }

  auto note () -> structure::arrangement::MidiNote *
  {
    return note_ref->get_object_as<structure::arrangement::MidiNote> ();
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 object_registry;
  std::unique_ptr<utils::AppSettings>   app_settings;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory>     factory;
  std::optional<structure::arrangement::ArrangerObjectUuidReference> note_ref;
};

TEST_F (TransformArrangerObjectsCommandTest, RedoWritesAfterStates)
{
  TransformableObjectState after;
  after.position_ticks_ = ticks (3840);
  after.length_ticks_ = ticks (96);
  after.pitch_ = 72;
  after.velocity_ = 111;

  TransformArrangerObjectsCommand command ({ *note_ref }, { after }, "Test");
  command.redo ();

  EXPECT_EQ (note ()->position ()->asTick (), ticks (3840));
  EXPECT_EQ (note ()->length ()->asTick (), ticks (96));
  EXPECT_EQ (note ()->pitch (), 72);
  EXPECT_EQ (note ()->velocity (), 111);
}

TEST_F (TransformArrangerObjectsCommandTest, UndoRestoresBeforeStates)
{
  TransformableObjectState after;
  after.length_ticks_ = ticks (96);
  after.velocity_ = 111;

  TransformArrangerObjectsCommand command ({ *note_ref }, { after }, "Test");
  command.redo ();
  command.undo ();

  EXPECT_EQ (note ()->position ()->asTick (), ticks (1920));
  EXPECT_EQ (note ()->length ()->asTick (), ticks (480));
  EXPECT_EQ (note ()->pitch (), 60);
  EXPECT_EQ (note ()->velocity (), 40);
}

TEST_F (TransformArrangerObjectsCommandTest, RepeatedCyclesAreIdempotent)
{
  TransformableObjectState after;
  after.position_ticks_ = ticks (0);
  after.length_ticks_ = ticks (48);
  after.pitch_ = 127;
  after.velocity_ = 127;

  TransformArrangerObjectsCommand command ({ *note_ref }, { after }, "Test");
  for (int i = 0; i < 3; ++i)
    {
      command.redo ();
      EXPECT_EQ (note ()->position ()->asTick (), ticks (0));
      EXPECT_EQ (note ()->length ()->asTick (), ticks (48));
      EXPECT_EQ (note ()->pitch (), 127);
      EXPECT_EQ (note ()->velocity (), 127);
      command.undo ();
      EXPECT_EQ (note ()->position ()->asTick (), ticks (1920));
      EXPECT_EQ (note ()->length ()->asTick (), ticks (480));
      EXPECT_EQ (note ()->pitch (), 60);
      EXPECT_EQ (note ()->velocity (), 40);
    }
}

TEST_F (TransformArrangerObjectsCommandTest, VelocityClampsLikeSetter)
{
  TransformableObjectState after;
  after.velocity_ = 300;

  TransformArrangerObjectsCommand command ({ *note_ref }, { after }, "Test");
  command.redo ();
  EXPECT_EQ (note ()->velocity (), 127);
}

TEST_F (TransformArrangerObjectsCommandTest, CommandText)
{
  TransformArrangerObjectsCommand command (
    {}, {}, QObject::tr ("Apply Crescendo"));
  EXPECT_EQ (command.text (), QString ("Apply Crescendo"));
}

} // namespace zrythm::commands
