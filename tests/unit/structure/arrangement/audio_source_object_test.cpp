// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/file_audio_source.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/audio_source_object.h"
#include "utils/audio.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class AudioSourceObjectTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    // Create & register a dummy audio source
    source_ref = registry.create_object<dsp::FileAudioSource> (
      utils::audio::AudioBuffer (2, 512), utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), 120.0, u8"Test Audio Source");

    // Create audio source object
    source_object = std::make_unique<AudioSourceObject> (
      *tempo_map, registry, *source_ref, nullptr);
  }

  std::unique_ptr<dsp::TempoMap>                   tempo_map;
  dsp::FileAudioSourceRegistry                     registry;
  std::optional<dsp::FileAudioSourceUuidReference> source_ref;
  std::shared_ptr<dsp::FileAudioSource>            dummy_audio_source;
  std::unique_ptr<AudioSourceObject>               source_object;
};

TEST_F (AudioSourceObjectTest, InitialState)
{
  EXPECT_EQ (source_object->type (), ArrangerObject::Type::AudioSourceObject);
  EXPECT_NE (&source_object->get_audio_source (), nullptr);
}

TEST_F (AudioSourceObjectTest, GetAudioSource)
{
  auto &audio_source = source_object->get_audio_source ();
  EXPECT_EQ (audio_source.getTotalLength (), 512);
}

TEST_F (AudioSourceObjectTest, Serialization)
{
  // Serialize
  nlohmann::json j;
  to_json (j, *source_object);

  auto dummy_source_ref = registry.create_object<dsp::FileAudioSource> (
    utils::audio::AudioBuffer (2, 16), utils::audio::BitDepth::BIT_DEPTH_32,
    units::sample_rate (44100), 120.0, u8"Unused dummy Audio Source");

  // Create new object
  auto new_source_object = std::make_unique<AudioSourceObject> (
    *tempo_map, registry,
    dummy_source_ref, // Dummy, will be overwritten by deserialization
    nullptr);
  from_json (j, *new_source_object);

  // Verify deserialization
  EXPECT_EQ (new_source_object->get_uuid (), source_object->get_uuid ());
  auto &new_audio_source = new_source_object->get_audio_source ();
  EXPECT_EQ (new_audio_source.getTotalLength (), 512);
}

TEST_F (AudioSourceObjectTest, ObjectCloning)
{
  // Clone with new identity
  auto cloned_object = std::make_unique<AudioSourceObject> (
    *tempo_map, registry, *source_ref, nullptr);
  init_from (
    *cloned_object, *source_object, utils::ObjectCloneType::NewIdentity);

  // Verify cloning
  EXPECT_NE (cloned_object->get_uuid (), source_object->get_uuid ());
  auto &cloned_audio_source = cloned_object->get_audio_source ();
  EXPECT_EQ (cloned_audio_source.getTotalLength (), 512);
}

// When the buffer is cleared and repopulated, the internal audio source must be
// rebuilt to reflect the new buffer state.
TEST_F (AudioSourceObjectTest, AudioSourceReflectsRepopulatedBuffer)
{
  auto * file_source = std::get<dsp::FileAudioSource *> (
    registry.find_by_id_or_throw (source_ref->id ()));
  ASSERT_EQ (file_source->get_num_frames (), 512);
  ASSERT_EQ (source_object->get_audio_source ().getTotalLength (), 512);

  // Clear and repopulate the buffer with a different size, as happens when
  // the underlying audio data is reloaded from disk
  file_source->clear_frames ();
  ASSERT_EQ (file_source->get_num_frames (), 0);

  utils::audio::AudioBuffer new_buffer (2, 1024);
  file_source->expand_with_frames (new_buffer);
  ASSERT_EQ (file_source->get_num_frames (), 1024);

  EXPECT_EQ (source_object->get_audio_source ().getTotalLength (), 1024);
}

// When the underlying audio buffer changes, AudioSourceObject must emit
// propertiesChanged.
TEST_F (AudioSourceObjectTest, BufferChangeEmitsPropertiesChanged)
{
  test_helpers::ScopedQCoreApplication app;

  QSignalSpy spy (source_object.get (), &ArrangerObject::propertiesChanged);
  ASSERT_TRUE (spy.isValid ());

  auto * file_source = std::get<dsp::FileAudioSource *> (
    registry.find_by_id_or_throw (source_ref->id ()));

  // Simulate buffer change as happens during init_loaded()
  file_source->clear_frames ();
  utils::audio::AudioBuffer new_buffer (2, 256);
  file_source->expand_with_frames (new_buffer);

  EXPECT_GT (spy.count (), 0);
}

} // namespace zrythm::structure::arrangement
