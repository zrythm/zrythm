// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/file_audio_source.h"
#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/audio_source_object.h"
#include "utils/audio.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
class AudioSourceObjectTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    // Create & register a dummy audio source
    source_ref = utils::create_object<dsp::FileAudioSource> (
      registry, utils::audio::AudioBuffer (2, 512),
      utils::audio::BitDepth::BIT_DEPTH_32, units::sample_rate (44100),
      units::bpm (120.0), u8"Test Audio Source");

    // Create audio source object
    source_object = std::make_unique<AudioSourceObject> (
      *tempo_map_wrapper, registry, *source_ref, nullptr);
  }

  std::unique_ptr<dsp::TempoMap>                   tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper>            tempo_map_wrapper;
  utils::ObjectRegistry                            registry;
  std::optional<dsp::FileAudioSourceUuidReference> source_ref;
  std::shared_ptr<dsp::FileAudioSource>            dummy_audio_source;
  std::unique_ptr<AudioSourceObject>               source_object;
};

TEST_F (AudioSourceObjectTest, InitialState)
{
  EXPECT_EQ (source_object->type (), ArrangerObject::Type::AudioSourceObject);
  EXPECT_EQ (&source_object->file_audio_source (), source_ref->get ());
}

TEST_F (AudioSourceObjectTest, Serialization)
{
  // Serialize
  nlohmann::json j;
  to_json (j, *source_object);

  auto dummy_source_ref = utils::create_object<dsp::FileAudioSource> (
    registry, utils::audio::AudioBuffer (2, 16),
    utils::audio::BitDepth::BIT_DEPTH_32, units::sample_rate (44100),
    units::bpm (120.0), u8"Unused dummy Audio Source");

  // Create new object
  auto new_source_object = std::make_unique<AudioSourceObject> (
    *tempo_map_wrapper, registry,
    dummy_source_ref, // Dummy, will be overwritten by deserialization
    nullptr);
  from_json (j, *new_source_object);

  // Verify deserialization
  EXPECT_EQ (new_source_object->get_uuid (), source_object->get_uuid ());
  EXPECT_EQ (&new_source_object->file_audio_source (), source_ref->get ());
}

TEST_F (AudioSourceObjectTest, ObjectCloning)
{
  // Clone with new identity
  auto cloned_object = std::make_unique<AudioSourceObject> (
    *tempo_map_wrapper, registry, *source_ref, nullptr);
  init_from (
    *cloned_object, *source_object, utils::ObjectCloneType::NewIdentity);

  // Verify cloning
  EXPECT_NE (cloned_object->get_uuid (), source_object->get_uuid ());
  EXPECT_EQ (&cloned_object->file_audio_source (), source_ref->get ());
}

} // namespace zrythm::structure::arrangement
