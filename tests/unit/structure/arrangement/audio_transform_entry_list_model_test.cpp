// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/audio_function.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/audio_transform_entry_list_model.h"
#include "utils/object_registry.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{

class AudioTransformEntryListModelTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app = std::make_unique<test_helpers::ScopedQCoreApplication> ();
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (tempo_map);

    clip_ref = make_clip (u8"ModelTestSource");
  }

  static void attach_ramp_source (
    utils::ObjectRegistry &object_registry,
    dsp::TempoMapWrapper  &wrapper,
    AudioClip             &clip,
    const char8_t *        name)
  {
    utils::audio::AudioBuffer frames{ 2, 8 };
    auto source_ref = utils::create_object<dsp::FileAudioSource> (
      object_registry, frames, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), units::bpm (120.0), name);
    auto source_object_ref = utils::create_object<AudioSourceObject> (
      object_registry, wrapper, object_registry, source_ref);
    clip.set_source (source_object_ref);
  }

  ArrangerObjectUuidReference make_clip (const char8_t * name)
  {
    auto ref =
      utils::create_object<AudioClip> (registry, *tempo_map_wrapper, registry);
    attach_ramp_source (
      registry, *tempo_map_wrapper, *ref.get_object_as<AudioClip> (), name);
    return ref;
  }

  static AudioFunction::Entry
  invert_entry (double start_ticks, double end_ticks, bool enabled = true)
  {
    return AudioFunction::Entry{
      .start = dsp::ContentTick{ units::ticks (start_ticks) },
      .end = dsp::ContentTick{ units::ticks (end_ticks) },
      .enabled = enabled,
      .kind = AudioFunction::Invert{}
    };
  }

  dsp::TempoMap tempo_map{ units::sample_rate (44100.0) };
  std::unique_ptr<dsp::TempoMapWrapper>                 tempo_map_wrapper;
  std::unique_ptr<test_helpers::ScopedQCoreApplication> app;
  utils::ObjectRegistry                                 registry;
  ArrangerObjectUuidReference                           clip_ref{ registry };
};

TEST_F (AudioTransformEntryListModelTest, ExposesClipEntriesAsRoleData)
{
  auto * clip = clip_ref.get_object_as<AudioClip> ();
  clip->set_transform_entries ({ invert_entry (0., 1920.) });

  AudioTransformEntryListModel model;
  EXPECT_EQ (model.rowCount (), 0);

  model.setClip (clip);
  EXPECT_EQ (model.rowCount (), 1);

  const auto variant =
    model.data (model.index (0, 0), AudioTransformEntryListModel::EntryRole);
  ASSERT_TRUE (variant.isValid ());
  const auto entry = variant.value<AudioFunction::Entry> ();
  EXPECT_EQ (entry.kindId (), QStringLiteral ("invert-polarity"));
  EXPECT_DOUBLE_EQ (entry.end_ticks (), 1920.);
  EXPECT_TRUE (entry.is_enabled ());

  EXPECT_FALSE (model.data (model.index (0, 0), Qt::DisplayRole).isValid ());
}

TEST_F (AudioTransformEntryListModelTest, SameCountChangeUpdatesRowsInPlace)
{
  auto * clip = clip_ref.get_object_as<AudioClip> ();
  clip->set_transform_entries ({ invert_entry (0., 1920.) });

  AudioTransformEntryListModel model;
  model.setClip (clip);

  QSignalSpy data_changed (&model, &QAbstractItemModel::dataChanged);
  QSignalSpy reset (&model, &QAbstractItemModel::modelReset);

  clip->set_transform_entries ({ invert_entry (0., 960., false) });

  EXPECT_EQ (reset.count (), 0);
  ASSERT_EQ (data_changed.count (), 1);
  EXPECT_EQ (model.rowCount (), 1);

  const auto entry =
    model.data (model.index (0, 0), AudioTransformEntryListModel::EntryRole)
      .value<AudioFunction::Entry> ();
  EXPECT_DOUBLE_EQ (entry.end_ticks (), 960.);
  EXPECT_FALSE (entry.is_enabled ());
}

TEST_F (AudioTransformEntryListModelTest, CountChangeResetsList)
{
  auto * clip = clip_ref.get_object_as<AudioClip> ();
  clip->set_transform_entries ({ invert_entry (0., 1920.) });

  AudioTransformEntryListModel model;
  model.setClip (clip);

  QSignalSpy data_changed (&model, &QAbstractItemModel::dataChanged);
  QSignalSpy reset (&model, &QAbstractItemModel::modelReset);

  clip->set_transform_entries (
    { invert_entry (0., 960.), invert_entry (960., 1920.) });
  EXPECT_EQ (reset.count (), 1);
  EXPECT_EQ (model.rowCount (), 2);

  clip->set_transform_entries ({});
  EXPECT_EQ (reset.count (), 2);
  EXPECT_EQ (model.rowCount (), 0);
  EXPECT_EQ (data_changed.count (), 0);
}

TEST_F (
  AudioTransformEntryListModelTest,
  DetachingClipClearsListAndStopsFollowing)
{
  auto * clip = clip_ref.get_object_as<AudioClip> ();
  clip->set_transform_entries ({ invert_entry (0., 1920.) });

  AudioTransformEntryListModel model;
  model.setClip (clip);
  EXPECT_EQ (model.rowCount (), 1);

  QSignalSpy reset (&model, &QAbstractItemModel::modelReset);
  model.setClip (nullptr);
  EXPECT_EQ (model.rowCount (), 0);
  ASSERT_EQ (reset.count (), 1);

  clip->set_transform_entries ({});
  EXPECT_EQ (reset.count (), 1);
}

TEST_F (
  AudioTransformEntryListModelTest,
  SwitchingClipsStopsFollowingTheDetachedClip)
{
  auto * clip = clip_ref.get_object_as<AudioClip> ();
  clip->set_transform_entries ({ invert_entry (0., 1920.) });
  auto   second_ref = make_clip (u8"ModelTestSecondSource");
  auto * second_clip = second_ref.get_object_as<AudioClip> ();

  AudioTransformEntryListModel model;
  model.setClip (clip);
  ASSERT_EQ (model.rowCount (), 1);
  model.setClip (second_clip);
  EXPECT_EQ (model.rowCount (), 0);

  QSignalSpy data_changed (&model, &QAbstractItemModel::dataChanged);
  QSignalSpy reset (&model, &QAbstractItemModel::modelReset);

  clip->set_transform_entries ({});
  EXPECT_EQ (data_changed.count (), 0);
  EXPECT_EQ (reset.count (), 0);

  second_clip->set_transform_entries ({ invert_entry (0., 960.) });
  EXPECT_EQ (reset.count (), 1);
  EXPECT_EQ (model.rowCount (), 1);
}

TEST_F (AudioTransformEntryListModelTest, CountTracksEntryListSize)
{
  auto * clip = clip_ref.get_object_as<AudioClip> ();
  clip->set_transform_entries ({ invert_entry (0., 1920.) });

  AudioTransformEntryListModel model;
  EXPECT_EQ (model.count (), 0);

  QSignalSpy count_spy (&model, &AudioTransformEntryListModel::countChanged);
  model.setClip (clip);
  EXPECT_EQ (model.count (), 1);
  EXPECT_EQ (count_spy.count (), 1);

  // Same-count changes leave the count signal untouched
  clip->set_transform_entries ({ invert_entry (0., 960., false) });
  EXPECT_EQ (count_spy.count (), 1);

  clip->set_transform_entries (
    { invert_entry (0., 960.), invert_entry (960., 1920.) });
  EXPECT_EQ (model.count (), 2);
  EXPECT_EQ (count_spy.count (), 2);

  model.setClip (nullptr);
  EXPECT_EQ (model.count (), 0);
  EXPECT_EQ (count_spy.count (), 3);
}

} // namespace zrythm::structure::arrangement
