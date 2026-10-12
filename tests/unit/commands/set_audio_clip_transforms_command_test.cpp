// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/set_audio_clip_transforms_command.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/audio_source_object.h"
#include "utils/object_registry.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

namespace
{

constexpr dsp::ContentTick
ticks (double t)
{
  return dsp::ContentTick{ units::ticks (t) };
}

} // namespace

class SetAudioClipTransformsCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app = std::make_unique<test_helpers::ScopedQCoreApplication> ();
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (tempo_map);

    clip_ref = utils::create_object<structure::arrangement::AudioClip> (
      registry, *tempo_map_wrapper, registry);
    auto * clip = clip_ref.get_object_as<structure::arrangement::AudioClip> ();

    utils::audio::AudioBuffer frames{ 2, 8 };
    auto source_ref = utils::create_object<dsp::FileAudioSource> (
      registry, frames, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), units::bpm (120.0), u8"CommandTestSource");
    auto source_object_ref =
      utils::create_object<structure::arrangement::AudioSourceObject> (
        registry, *tempo_map_wrapper, registry, source_ref);
    clip->set_source (source_object_ref);
  }

  dsp::TempoMap tempo_map{ units::sample_rate (44100.0) };
  std::unique_ptr<dsp::TempoMapWrapper>                 tempo_map_wrapper;
  std::unique_ptr<test_helpers::ScopedQCoreApplication> app;
  utils::ObjectRegistry                                 registry;
  structure::arrangement::ArrangerObjectUuidReference   clip_ref{ registry };
};

TEST_F (SetAudioClipTransformsCommandTest, RedoAndUndoSwapEntryLists)
{
  auto * clip = clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  ASSERT_TRUE (clip->transform_entries ().empty ());

  QSignalSpy rendered_spy (
    clip, &structure::arrangement::AudioClip::renderedAudioChanged);

  std::vector<structure::arrangement::AudioFunction::Entry> after;
  after.push_back (
    structure::arrangement::AudioFunction::Invert{
      .start = ticks (0.), .end = ticks (1920.) });
  SetAudioClipTransformsCommand command (
    clip_ref, std::move (after), QStringLiteral ("Apply Invert Polarity"));

  command.redo ();
  EXPECT_EQ (clip->transform_entries ().size (), 1u);
  EXPECT_EQ (rendered_spy.count (), 1);
  const auto * invert =
    std::get_if<structure::arrangement::AudioFunction::Invert> (
      &clip->transform_entries ().front ());
  ASSERT_NE (invert, nullptr);
  EXPECT_DOUBLE_EQ (invert->end.asDouble (), 1920.);

  command.undo ();
  EXPECT_TRUE (clip->transform_entries ().empty ());
  EXPECT_EQ (rendered_spy.count (), 2);
}

} // namespace zrythm::commands
