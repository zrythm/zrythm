// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "commands/bake_audio_clip_command.h"
#include "dsp/audio_block_store.h"
#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/audio_function.h"
#include "structure/arrangement/audio_source_object.h"
#include "utils/io_utils.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class BakeAudioClipCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app = std::make_unique<test_helpers::ScopedQCoreApplication> ();
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (tempo_map);
    temp_dir =
      utils::Utf8String::from_qstring (temp_dir_obj->path ()).to_path ();
    store = std::make_shared<dsp::AudioBlockStore> (temp_dir / "blocks");
  }

  /**
   * Creates a source reading a frame-index ramp on both channels.
   * At 120 BPM and 44.1 kHz, 192 ticks equal 4410 samples.
   */
  dsp::FileAudioSourceUuidReference
  make_ramp_source (const utils::Utf8String &name, int num_frames)
  {
    utils::audio::AudioBuffer frames (2, num_frames);
    for (const auto ch : std::views::iota (0, 2))
      {
        for (const auto i : std::views::iota (0, num_frames))
          {
            frames.setSample (ch, i, static_cast<float> (i));
          }
      }
    auto source_ref = utils::create_object<dsp::FileAudioSource> (
      registry, frames, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), units::bpm (120.0), name);
    source_ref.get_object_as<dsp::FileAudioSource> ()->set_store (store);
    return source_ref;
  }

  structure::arrangement::AudioClip &
  make_clip (dsp::FileAudioSourceUuidReference source_ref)
  {
    auto source_object_ref =
      utils::create_object<structure::arrangement::AudioSourceObject> (
        registry, *tempo_map_wrapper, registry, source_ref);
    auto clip_ref_ = utils::create_object<structure::arrangement::AudioClip> (
      registry, *tempo_map_wrapper, registry);
    auto * clip = clip_ref_.get_object_as<structure::arrangement::AudioClip> ();
    clip->set_source (source_object_ref);
    source_object_refs.push_back (std::move (source_object_ref));
    clip_refs.push_back (std::move (clip_ref_));
    return *clip;
  }

  /** Expects the source to read the ramp, with the first @p
   * inverted_frames frames negated. */
  static void expect_source_reads_ramp (
    const dsp::FileAudioSource &source,
    int64_t                     inverted_frames)
  {
    const auto                num = source.get_num_frames ();
    utils::audio::AudioBuffer result (
      2, static_cast<int> (num.in (units::samples)));
    ASSERT_NO_THROW (source.read_span (units::samples (0), num, result));
    for (const auto ch : std::views::iota (0, 2))
      {
        for (
          const auto i :
          std::views::iota (int64_t{ 0 }, num.in (units::samples)))
          {
            auto expected = static_cast<float> (i);
            if (i < inverted_frames)
              {
                expected = -expected;
              }
            EXPECT_EQ (result.getSample (ch, static_cast<int> (i)), expected)
              << "ch=" << ch << " i=" << i;
          }
      }
  }

  dsp::TempoMap tempo_map{ units::sample_rate (44100.0) };
  std::unique_ptr<dsp::TempoMapWrapper>                 tempo_map_wrapper;
  std::unique_ptr<test_helpers::ScopedQCoreApplication> app;
  utils::ObjectRegistry                                 registry;
  std::unique_ptr<QTemporaryDir> temp_dir_obj = utils::io::make_tmp_dir ();
  std::filesystem::path          temp_dir;
  std::shared_ptr<dsp::AudioBlockStore>                            store;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> clip_refs;
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
    source_object_refs;
};

TEST_F (BakeAudioClipCommandTest, RedoPointsClipAtBakedSourceAndClearsEntries)
{
  auto  source_ref = make_ramp_source (u8"BakeTestSource", 8820);
  auto &clip = make_clip (source_ref);
  clip.set_transform_entries (
    {
      structure::arrangement::AudioFunction::Invert{
                                                    .start = dsp::ContentTick{ units::ticks (0.0) },
                                                    .end = dsp::ContentTick{ units::ticks (384.0) } }
  });

  const auto baked_ref =
    structure::arrangement::AudioFunction::bake_audio_clip (clip, registry);
  BakeAudioClipCommand command (
    clip_refs.back (), baked_ref, QStringLiteral ("Bake Clip"));

  QSignalSpy rendered_spy (
    &clip, &structure::arrangement::AudioClip::renderedAudioChanged);
  command.redo ();
  // The re-point itself is signal-silent; clearing the entries must emit
  // the notification listeners use to invalidate caches.
  EXPECT_EQ (rendered_spy.count (), 1);

  const auto &new_source =
    clip.get_children_view ().front ()->file_audio_source ();
  EXPECT_TRUE (clip.transform_entries ().empty ());
  EXPECT_NE (
    clip.get_children_view ().front ()->audio_source_ref ().id (),
    source_ref.id ());
  expect_source_reads_ramp (new_source, 8820);
  expect_source_reads_ramp (
    *source_ref.get_object_as<dsp::FileAudioSource> (), 0);
}

TEST_F (BakeAudioClipCommandTest, BakeSharesBlocksOutsideEntryRanges)
{
  auto  source_ref = make_ramp_source (u8"BakeShareSource", 8820);
  auto &clip = make_clip (source_ref);
  clip.set_transform_entries (
    {
      structure::arrangement::AudioFunction::Invert{
                                                    .start = dsp::ContentTick{ units::ticks (0.0) },
                                                    .end = dsp::ContentTick{ units::ticks (192.0) } }
  });

  const auto baked_ref =
    structure::arrangement::AudioFunction::bake_audio_clip (clip, registry);
  // The bake flushes the original source, materializing its block.
  const auto original_spans =
    source_ref.get_object_as<dsp::FileAudioSource> ()->get_spans ();
  ASSERT_EQ (original_spans.size (), 1u);
  BakeAudioClipCommand command (
    clip_refs.back (), baked_ref, QStringLiteral ("Bake Clip"));
  command.redo ();

  const auto &new_source =
    clip.get_children_view ().front ()->file_audio_source ();
  expect_source_reads_ramp (new_source, 4410);

  // The untouched second half is a partial reference to the original
  // block.
  const auto new_spans = new_source.get_spans ();
  ASSERT_EQ (new_spans.size (), 2u);
  EXPECT_EQ (new_spans[1].block, original_spans[0].block);
  EXPECT_EQ (new_spans[1].src_start_frame, units::samples (4410));
  EXPECT_EQ (new_spans[1].num_frames, units::samples (4410));
}

TEST_F (BakeAudioClipCommandTest, UndoRestoresSourceAndEntries)
{
  auto  source_ref = make_ramp_source (u8"BakeUndoSource", 8820);
  auto &clip = make_clip (source_ref);
  clip.set_transform_entries (
    {
      structure::arrangement::AudioFunction::Invert{
                                                    .start = dsp::ContentTick{ units::ticks (0.0) },
                                                    .end = dsp::ContentTick{ units::ticks (384.0) } }
  });

  const auto baked_ref =
    structure::arrangement::AudioFunction::bake_audio_clip (clip, registry);
  BakeAudioClipCommand command (
    clip_refs.back (), baked_ref, QStringLiteral ("Bake Clip"));
  QSignalSpy rendered_spy (
    &clip, &structure::arrangement::AudioClip::renderedAudioChanged);
  command.redo ();
  command.undo ();
  EXPECT_EQ (rendered_spy.count (), 2);

  EXPECT_EQ (
    clip.get_children_view ().front ()->audio_source_ref ().id (),
    source_ref.id ());
  EXPECT_EQ (clip.transform_entries ().size (), 1u);
  expect_source_reads_ramp (
    clip.get_children_view ().front ()->file_audio_source (), 0);

  command.redo ();
  EXPECT_NE (
    clip.get_children_view ().front ()->audio_source_ref ().id (),
    source_ref.id ());
  EXPECT_TRUE (clip.transform_entries ().empty ());
  EXPECT_EQ (rendered_spy.count (), 3);
}

TEST_F (BakeAudioClipCommandTest, OtherClipsSharingTheSourceAreUnaffected)
{
  auto  source_ref = make_ramp_source (u8"BakeSharedSource", 8820);
  auto &editing_clip = make_clip (source_ref);
  auto &other_clip = make_clip (source_ref);
  editing_clip.set_transform_entries (
    {
      structure::arrangement::AudioFunction::Invert{
                                                    .start = dsp::ContentTick{ units::ticks (0.0) },
                                                    .end = dsp::ContentTick{ units::ticks (384.0) } }
  });

  const auto baked_ref = structure::arrangement::AudioFunction::bake_audio_clip (
    editing_clip, registry);
  BakeAudioClipCommand command (
    clip_refs.front (), baked_ref, QStringLiteral ("Bake Clip"));
  command.redo ();

  EXPECT_NE (
    editing_clip.get_children_view ().front ()->audio_source_ref ().id (),
    source_ref.id ());
  EXPECT_EQ (
    other_clip.get_children_view ().front ()->audio_source_ref ().id (),
    source_ref.id ());
  expect_source_reads_ramp (
    other_clip.get_children_view ().front ()->file_audio_source (), 0);
}

} // namespace zrythm::commands
