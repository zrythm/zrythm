// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/port_observation_manager.h"
#include "dsp/port_observer.h"
#include "gui/qquick/meter_processor.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::gui::qquick
{

class MeterProcessorTest : public ::testing::Test
{
protected:
  static constexpr units::sample_rate_t sample_rate_{
    units::sample_rate (48000)
  };
  static constexpr units::sample_u32_t block_length_{ units::samples (256u) };

  dsp::AudioPort * make_mono_port ()
  {
    port_refs_.push_back (
      utils::create_object<dsp::AudioPort> (
        registry_, u8"Test", dsp::PortFlow::Output,
        dsp::SpeakerArrangement::mono ()));
    return port_refs_.back ().get_object_as<dsp::AudioPort> ();
  }

  std::unique_ptr<MeterProcessor> make_meter (dsp::Port * port, int channel)
  {
    auto meter = std::make_unique<MeterProcessor> ();
    meter->setSampleRate (sample_rate_.in<int> (units::sample_rate));
    meter->setChannel (channel);
    meter->setPortObservationManager (&manager_);
    meter->setPort (port);
    return meter;
  }

  // Pushes @p value-filled samples into the observer ring for channel 0 and
  // drains them into all requester caches (mono data only).
  void push_mono_audio (dsp::Port * port, float value)
  {
    auto * observer = manager_.get_observer (*port);
    ASSERT_NE (observer, nullptr);
    observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

    std::array<float, block_length_.in<size_t> (units::samples)> samples{};
    samples.fill (value);
    observer->audio_ring (0).force_write_multiple (
      samples.data (), samples.size ());
    manager_.drain_all ();
  }

  test_helpers::ScopedQCoreApplication                   app_;
  utils::ObjectRegistry                                  registry_;
  dsp::PortObservationManager                            manager_{ registry_ };
  std::vector<utils::TypedUuidReference<dsp::AudioPort>> port_refs_;
};

// The observation cache only gains channels once the graph prepares the
// observer. Until then the meter must report silence.
TEST_F (MeterProcessorTest, UnpreparedObserverYieldsSilence)
{
  auto * port = make_mono_port ();

  // right-channel meter (this exact setup crashed out-of-bounds before)
  auto meter = make_meter (port, 1);

  meter->processValues ();

  EXPECT_FLOAT_EQ (meter->currentAmplitude (), 0.f);
  EXPECT_FLOAT_EQ (meter->peakAmplitude (), 0.f);
}

// A meter for a channel beyond what the drained cache contains must report
// silence.
TEST_F (MeterProcessorTest, OutOfRangeChannelYieldsSilence)
{
  auto * port = make_mono_port ();
  auto   meter = make_meter (port, 1);

  push_mono_audio (port, 1.0f);

  meter->processValues ();

  EXPECT_FLOAT_EQ (meter->currentAmplitude (), 0.f);
  EXPECT_FLOAT_EQ (meter->peakAmplitude (), 0.f);
}

TEST_F (MeterProcessorTest, ValidChannelReadsData)
{
  auto * port = make_mono_port ();
  auto   meter = make_meter (port, 0);

  push_mono_audio (port, 1.0f);

  meter->processValues ();

  EXPECT_GT (meter->currentAmplitude (), 0.5f);
}

}
