// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/cv_port.h"
#include "dsp/file_audio_source.h"
#include "dsp/midi_port.h"
#include "dsp/parameter.h"
#include "dsp/port.h"
#include "structure/project/project_registry.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

class ProjectRegistryTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override { registry_ = std::make_unique<ProjectRegistry> (); }

  void TearDown () override { registry_.reset (); }

  std::unique_ptr<ProjectRegistry> registry_;
};

// ============================================================================
// Registration tests
// ============================================================================

TEST_F (ProjectRegistryTest, RegisterAudioPort)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"test", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto uuid = port->raw_uuid ();
  registry_->register_object (*port);
  port.release ();

  EXPECT_TRUE (registry_->contains (uuid));
  EXPECT_EQ (registry_->count_matching<dsp::Port> (), 1u);
  EXPECT_NE (registry_->find_by_raw_uuid (uuid), nullptr);
}

TEST_F (ProjectRegistryTest, RegisterMidiPort)
{
  auto port = std::make_unique<dsp::MidiPort> (u8"midi", dsp::PortFlow::Input);
  auto uuid = port->raw_uuid ();
  registry_->register_object (*port);
  port.release ();

  EXPECT_TRUE (registry_->contains (uuid));
  EXPECT_EQ (registry_->count_matching<dsp::Port> (), 1u);
}

TEST_F (ProjectRegistryTest, RegisterCVPort)
{
  auto port = std::make_unique<dsp::CVPort> (u8"cv", dsp::PortFlow::Output);
  auto uuid = port->raw_uuid ();
  registry_->register_object (*port);
  port.release ();

  EXPECT_TRUE (registry_->contains (uuid));
  EXPECT_EQ (registry_->count_matching<dsp::Port> (), 1u);
}

TEST_F (ProjectRegistryTest, RegisterProcessorParameter)
{
  auto param = std::make_unique<dsp::ProcessorParameter> (
    *registry_, dsp::ProcessorParameter::UniqueId{},
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.01f),
    u8"gain");
  auto uuid = param->raw_uuid ();
  registry_->register_object (*param);
  param.release ();

  EXPECT_TRUE (registry_->contains (uuid));
  EXPECT_EQ (registry_->count_matching<dsp::ProcessorParameter> (), 1u);
  EXPECT_EQ (registry_->count_matching<dsp::Port> (), 1u);
}

TEST_F (ProjectRegistryTest, RegisterFileAudioSource)
{
  auto fas = std::make_unique<dsp::FileAudioSource> ();
  auto uuid = fas->raw_uuid ();
  registry_->register_object (*fas);
  fas.release ();

  EXPECT_TRUE (registry_->contains (uuid));
  EXPECT_EQ (registry_->count_matching<dsp::FileAudioSource> (), 1u);
}

// ============================================================================
// Multi-category registration
// ============================================================================

TEST_F (ProjectRegistryTest, RegisterMultipleCategories)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"port", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto port_uuid = port->raw_uuid ();
  registry_->register_object (*port);
  port.release ();

  auto param = std::make_unique<dsp::ProcessorParameter> (
    *registry_, dsp::ProcessorParameter::UniqueId{},
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.01f),
    u8"param");
  auto param_uuid = param->raw_uuid ();
  registry_->register_object (*param);
  param.release ();

  auto fas = std::make_unique<dsp::FileAudioSource> ();
  auto fas_uuid = fas->raw_uuid ();
  registry_->register_object (*fas);
  fas.release ();

  EXPECT_EQ (
    registry_->count_matching<dsp::Port> ()
      + registry_->count_matching<dsp::ProcessorParameter> ()
      + registry_->count_matching<dsp::FileAudioSource> (),
    4u);
  EXPECT_EQ (registry_->count_matching<dsp::Port> (), 2u);
  EXPECT_EQ (registry_->count_matching<dsp::ProcessorParameter> (), 1u);
  EXPECT_EQ (registry_->count_matching<dsp::FileAudioSource> (), 1u);

  EXPECT_TRUE (registry_->contains (port_uuid));
  EXPECT_TRUE (registry_->contains (param_uuid));
  EXPECT_TRUE (registry_->contains (fas_uuid));
}

TEST_F (ProjectRegistryTest, DuplicateRegistrationThrows)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"port", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  registry_->register_object (*port);
  EXPECT_THROW (registry_->register_object (*port), std::runtime_error);
  port.release ();
}

// ============================================================================
// Lookup tests
// ============================================================================

TEST_F (ProjectRegistryTest, FindByRawUuidReturnsCorrectObject)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"port", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto uuid = port->raw_uuid ();
  registry_->register_object (*port);
  port.release ();

  auto * found = registry_->find_by_raw_uuid (uuid);
  ASSERT_NE (found, nullptr);
  EXPECT_EQ (found->raw_uuid (), uuid);
}

TEST_F (ProjectRegistryTest, FindByRawUuidReturnsNullForUnknown)
{
  auto * found = registry_->find_by_raw_uuid (QUuid::createUuid ());
  EXPECT_EQ (found, nullptr);
}

TEST_F (ProjectRegistryTest, ContainsUnknownReturnsFalse)
{
  EXPECT_FALSE (registry_->contains (QUuid::createUuid ()));
}

TEST_F (ProjectRegistryTest, EmptyRegistrySizeIsZero)
{
  EXPECT_EQ (
    registry_->count_matching<dsp::Port> ()
      + registry_->count_matching<dsp::ProcessorParameter> ()
      + registry_->count_matching<dsp::FileAudioSource> (),
    0u);
}

// ============================================================================
// Reference counting
// ============================================================================

TEST_F (ProjectRegistryTest, ReleaseReferenceDeletesObject)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"port", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto uuid = port->raw_uuid ();
  registry_->register_object (*port);
  port.release ();

  registry_->acquire_reference (uuid);

  registry_->release_reference (uuid);
  EXPECT_FALSE (registry_->contains (uuid));
  EXPECT_EQ (registry_->find_by_raw_uuid (uuid), nullptr);
}

TEST_F (ProjectRegistryTest, AcquireReferenceKeepsObjectAlive)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"port", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto uuid = port->raw_uuid ();
  registry_->register_object (*port);
  port.release ();

  registry_->acquire_reference (uuid);
  registry_->acquire_reference (uuid);

  registry_->release_reference (uuid);
  EXPECT_TRUE (registry_->contains (uuid));

  registry_->release_reference (uuid);
  EXPECT_FALSE (registry_->contains (uuid));
}

// ============================================================================
// Typed iteration
// ============================================================================

TEST_F (ProjectRegistryTest, ForEachPortIteratesOnlyPorts)
{
  auto port1 = std::make_unique<dsp::AudioPort> (
    u8"port1", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto port2 = std::make_unique<dsp::MidiPort> (u8"port2", dsp::PortFlow::Input);
  auto param = std::make_unique<dsp::ProcessorParameter> (
    *registry_, dsp::ProcessorParameter::UniqueId{},
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.01f),
    u8"param");
  registry_->register_object (*port1);
  port1.release ();
  registry_->register_object (*port2);
  port2.release ();
  registry_->register_object (*param);
  param.release ();

  int count = 0;
  registry_->for_each_matching<dsp::Port> ([&] (dsp::Port &) { count++; });
  EXPECT_EQ (count, 3);
}

TEST_F (ProjectRegistryTest, ForEachParamIteratesOnlyParams)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"port", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto param = std::make_unique<dsp::ProcessorParameter> (
    *registry_, dsp::ProcessorParameter::UniqueId{},
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.01f),
    u8"param");
  registry_->register_object (*port);
  port.release ();
  registry_->register_object (*param);
  param.release ();

  int count = 0;
  registry_->for_each_matching<dsp::ProcessorParameter> (
    [&] (dsp::ProcessorParameter &p) {
      EXPECT_EQ (p.label ().toStdString (), "param");
      count++;
    });
  EXPECT_EQ (count, 1);
}

TEST_F (ProjectRegistryTest, ForEachEmptyBucketIsNoOp)
{
  int count = 0;
  registry_->for_each_matching<dsp::Port> ([&] (dsp::Port &) { count++; });
  EXPECT_EQ (count, 0);
}

// ============================================================================
// Total object count across categories
// ============================================================================

TEST_F (ProjectRegistryTest, CountAcrossAllCategories)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"port", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto param = std::make_unique<dsp::ProcessorParameter> (
    *registry_, dsp::ProcessorParameter::UniqueId{},
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.01f),
    u8"param");
  auto fas = std::make_unique<dsp::FileAudioSource> ();
  registry_->register_object (*port);
  port.release ();
  registry_->register_object (*param);
  param.release ();
  registry_->register_object (*fas);
  fas.release ();

  const auto total =
    registry_->count_matching<dsp::Port> ()
    + registry_->count_matching<dsp::ProcessorParameter> ()
    + registry_->count_matching<dsp::FileAudioSource> ();
  EXPECT_EQ (total, 4u);
}

// ============================================================================
// Serialization
// ============================================================================

TEST_F (ProjectRegistryTest, ToJsonProducesValidJson)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"port", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  registry_->register_object (*port);
  port.release ();

  nlohmann::json j;
  to_json (j, *registry_);

  EXPECT_TRUE (j.contains ("ports"));
  EXPECT_TRUE (j["ports"].is_array ());
  EXPECT_EQ (j["ports"].size (), 1u);
  EXPECT_TRUE (j["ports"][0].contains ("id"));
}

TEST_F (ProjectRegistryTest, ToJsonEmptyRegistry)
{
  nlohmann::json j;
  to_json (j, *registry_);

  EXPECT_TRUE (j.contains ("ports"));
  EXPECT_TRUE (j["ports"].is_array ());
  EXPECT_EQ (j["ports"].size (), 0u);
}

// ============================================================================
// QObject ownership
// ============================================================================

TEST_F (ProjectRegistryTest, RegisteredObjectParentIsRegistry)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"port", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto * raw = port.get ();
  registry_->register_object (*port);
  port.release ();

  EXPECT_EQ (raw->parent (), registry_.get ());
}

// ============================================================================
// Derived type queries
// ============================================================================

TEST_F (ProjectRegistryTest, CountMatchingDerivedType)
{
  auto audio_port = std::make_unique<dsp::AudioPort> (
    u8"audio", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  registry_->register_object (*audio_port);
  audio_port.release ();

  auto midi_port =
    std::make_unique<dsp::MidiPort> (u8"midi", dsp::PortFlow::Input);
  registry_->register_object (*midi_port);
  midi_port.release ();

  auto cv_port = std::make_unique<dsp::CVPort> (u8"cv", dsp::PortFlow::Output);
  registry_->register_object (*cv_port);
  cv_port.release ();

  EXPECT_EQ (registry_->count_matching<dsp::Port> (), 3u);
  EXPECT_EQ (registry_->count_matching<dsp::AudioPort> (), 1u);
  EXPECT_EQ (registry_->count_matching<dsp::MidiPort> (), 1u);
  EXPECT_EQ (registry_->count_matching<dsp::CVPort> (), 1u);
}

TEST_F (ProjectRegistryTest, ForEachMatchingDerivedType)
{
  auto audio_port = std::make_unique<dsp::AudioPort> (
    u8"audio", dsp::PortFlow::Input, dsp::AudioPort::BusLayout{}, 2,
    dsp::AudioPort::Purpose::Main);
  auto * audio_raw = audio_port.get ();
  registry_->register_object (*audio_port);
  audio_port.release ();

  auto midi_port =
    std::make_unique<dsp::MidiPort> (u8"midi", dsp::PortFlow::Input);
  registry_->register_object (*midi_port);
  midi_port.release ();

  std::vector<dsp::AudioPort *> found;
  registry_->for_each_matching<dsp::AudioPort> ([&] (dsp::AudioPort &p) {
    found.push_back (&p);
  });
  ASSERT_EQ (found.size (), 1u);
  EXPECT_EQ (found[0], audio_raw);

  std::vector<dsp::Port *> all_ports;
  registry_->for_each_matching<dsp::Port> ([&] (dsp::Port &p) {
    all_ports.push_back (&p);
  });
  EXPECT_EQ (all_ports.size (), 2u);
}

TEST_F (ProjectRegistryTest, ForEachMatchingEmptyIsNoOp)
{
  int count = 0;
  registry_->for_each_matching<dsp::AudioPort> ([&] (dsp::AudioPort &) {
    count++;
  });
  EXPECT_EQ (count, 0);

  EXPECT_EQ (registry_->count_matching<dsp::AudioPort> (), 0u);
}

TEST_F (ProjectRegistryTest, ToJsonRoundTripPreservesPort)
{
  auto port = std::make_unique<dsp::AudioPort> (
    u8"roundtrip", dsp::PortFlow::Output, dsp::AudioPort::BusLayout::Stereo, 2,
    dsp::AudioPort::Purpose::Main);
  auto uuid = port->raw_uuid ();
  registry_->register_object (*port);
  port.release ();

  nlohmann::json j;
  to_json (j, *registry_);
  EXPECT_TRUE (j.contains ("ports"));
  EXPECT_EQ (j["ports"].size (), 1u);
  EXPECT_EQ (j["ports"][0]["id"].get<QUuid> (), uuid);
}

TEST_F (ProjectRegistryTest, ToJsonRoundTripEmptyRegistry)
{
  nlohmann::json j;
  to_json (j, *registry_);
  EXPECT_TRUE (j.contains ("ports"));
  EXPECT_TRUE (j.contains ("parameters"));
  EXPECT_TRUE (j.contains ("plugins"));
  EXPECT_TRUE (j.contains ("tracks"));
  EXPECT_TRUE (j.contains ("arrangerObjects"));
  EXPECT_TRUE (j.contains ("fileAudioSources"));
}

} // namespace zrythm::structure::project
