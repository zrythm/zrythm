// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/graph_builder.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"

#include <QObject>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class TestProcessor : public ProcessorBase
{
public:
  TestProcessor (
    PortRegistry               &port_registry,
    ProcessorParameterRegistry &param_registry)
      : ProcessorBase (
          ProcessorBase::ProcessorBaseDependencies{
            .port_registry_ = port_registry,
            .param_registry_ = param_registry },
          u8"TestProcessor"),
        input_port_ (port_registry.create_object<dsp::AudioPort> (
          u8"Input",
          PortFlow::Input,
          AudioPort::BusLayout::Mono,
          1)),
        output_port_ (port_registry.create_object<dsp::AudioPort> (
          u8"Output",
          PortFlow::Output,
          AudioPort::BusLayout::Mono,
          1))
  {
    add_input_port (input_port_);
    add_output_port (output_port_);
  }

  MOCK_METHOD (
    void,
    custom_process_block,
    (EngineProcessTimeInfo, const dsp::ITransport &),
    (noexcept, override));

private:
  PortUuidReference input_port_;
  PortUuidReference output_port_;
};

class ProcessorBaseTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<PortRegistry> ();
    param_registry_ =
      std::make_unique<ProcessorParameterRegistry> (*port_registry_);

    processor_ =
      std::make_unique<TestProcessor> (*port_registry_, *param_registry_);

    // Set up mock transport
    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
  }

  void expect_passthrough_at_half_volume ()
  {
    EXPECT_CALL (*processor_, custom_process_block (::testing::_, ::testing::_))
      .WillRepeatedly ([&] (EngineProcessTimeInfo time_nfo, auto &) {
        // Simple passthrough
        auto &in =
          processor_->get_input_ports ()
            .front ()
            .get_object_as<dsp::AudioPort> ()
            ->buffers ();
        auto &out =
          processor_->get_output_ports ()
            .front ()
            .get_object_as<dsp::AudioPort> ()
            ->buffers ();

        for (size_t i = 0; i < time_nfo.nframes_; i++)
          {
            out->setSample (
              0, static_cast<int> (i),
              in->getSample (0, static_cast<int> (i)) * 0.5f); // Process at
                                                               // half volume
          }
      });
  }

  void TearDown () override
  {
    if (processor_)
      {
        processor_->release_resources ();
      }
  }

  std::unique_ptr<PortRegistry>               port_registry_;
  std::unique_ptr<ProcessorParameterRegistry> param_registry_;
  units::sample_rate_t           sample_rate_{ units::sample_rate (48000) };
  nframes_t                      max_block_length_{ 1024 };
  std::unique_ptr<TestProcessor> processor_;
  std::unique_ptr<graph_test::MockTransport> mock_transport_;
};

TEST_F (ProcessorBaseTest, ConstructionAndBasicProperties)
{
  EXPECT_EQ (processor_->get_node_name (), u8"TestProcessor");
  EXPECT_EQ (processor_->get_input_ports ().size (), 1);
  EXPECT_EQ (processor_->get_output_ports ().size (), 1);
  EXPECT_EQ (processor_->get_parameters ().size (), 0);
}

TEST_F (ProcessorBaseTest, PrepareAndReleaseResources)
{
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Verify ports are prepared
  auto input_port =
    processor_->get_input_ports ()[0].get_object_as<dsp::AudioPort> ();
  EXPECT_GE (input_port->buffers ()->getNumSamples (), max_block_length_);

  processor_->release_resources ();
  EXPECT_EQ (input_port->buffers ().get (), nullptr);
}

TEST_F (ProcessorBaseTest, Processing)
{
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  auto input_port =
    processor_->get_input_ports ()[0].get_object_as<dsp::AudioPort> ();
  auto output_port =
    processor_->get_output_ports ()[0].get_object_as<dsp::AudioPort> ();

  // Fill input buffer
  for (int i = 0; i < 512; i++)
    {
      input_port->buffers ()->setSample (0, i, static_cast<float> (i) * 0.01f);
    }

  // Set expectations
  expect_passthrough_at_half_volume ();

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };
  processor_->process_block (time_nfo, *mock_transport_);

  // Verify processing (half volume)
  for (int i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (
        output_port->buffers ()->getSample (0, i),
        static_cast<float> (i) * 0.005f);
    }
}

TEST_F (ProcessorBaseTest, InputBufferClearedBetweenProcessCalls)
{
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  auto input_port =
    processor_->get_input_ports ()[0].get_object_as<dsp::AudioPort> ();
  auto output_port =
    processor_->get_output_ports ()[0].get_object_as<dsp::AudioPort> ();

  // Fill input buffer with test data
  for (int i = 0; i < 512; i++)
    {
      input_port->buffers ()->setSample (0, i, 1.0f);
    }

  // Set expectations
  expect_passthrough_at_half_volume ();

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // First process call
  processor_->process_block (time_nfo, *mock_transport_);

  // Verify first processing (should be 0.5 due to half volume)
  for (int i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (output_port->buffers ()->getSample (0, i), 0.5f);
    }

  // Check that input buffer is cleared after first process call
  // This is the key test - input should be zeroed after processing
  for (int i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (input_port->buffers ()->getSample (0, i), 0.0f)
        << "Input buffer not cleared at index " << i
        << " after first process_block() call";
    }

  // Reset output buffer to verify second processing
  for (int i = 0; i < 512; i++)
    {
      output_port->buffers ()->setSample (0, i, 0.0f);
    }

  // Second process call - should produce zeros since input was cleared
  processor_->process_block (time_nfo, *mock_transport_);

  // Verify second processing produces zeros (no accumulation)
  for (int i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (output_port->buffers ()->getSample (0, i), 0.0f)
        << "Output not zero at index " << i
        << " during second process_block() call - indicates buffer accumulation";
    }

  // Test with new input data to ensure it still processes correctly
  for (int i = 0; i < 512; i++)
    {
      input_port->buffers ()->setSample (0, i, 0.8f);
    }

  // Third process call
  processor_->process_block (time_nfo, *mock_transport_);

  // Verify third processing (should be 0.4 due to half volume)
  for (int i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (output_port->buffers ()->getSample (0, i), 0.4f);
    }
}

TEST_F (ProcessorBaseTest, JsonSerializationRoundtrip)
{
  processor_->add_parameter (
    param_registry_->create_object<dsp::ProcessorParameter> (
      *port_registry_, dsp::ProcessorParameter::UniqueId (u8"unique-id"),
      dsp::ParameterRange::make_toggle (true), u8"my-toggle"));
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Serialize
  nlohmann::json j = *processor_;

  // Create new processor from JSON
  TestProcessor deserialized (*port_registry_, *param_registry_);
  from_json (j, deserialized);

  // Reinitialize after deserialization
  deserialized.prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Verify ports && params
  EXPECT_EQ (
    processor_->get_parameters ().front ().id (),
    deserialized.get_parameters ().front ().id ());
  EXPECT_EQ (
    processor_->get_input_ports ()[0].id (),
    deserialized.get_input_ports ()[0].id ());
  EXPECT_EQ (
    processor_->get_output_ports ()[0].id (),
    deserialized.get_output_ports ()[0].id ());
}

TEST_F (ProcessorBaseTest, GraphBuilderIntegration)
{
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  graph::Graph graph;

  // Add processor to graph
  ProcessorGraphBuilder::add_nodes (graph, *processor_);
  ProcessorGraphBuilder::add_connections (graph, *processor_);
  graph.finalize_nodes ();

  // Verify graph structure
  EXPECT_EQ (graph.get_nodes ().graph_nodes_.size (), 3);
  EXPECT_TRUE (graph.is_valid ());
}

TEST_F (ProcessorBaseTest, EdgeCases)
{
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  auto * input_port =
    processor_->get_input_ports ()[0].get_object_as<dsp::AudioPort> ();

  // Fill input buffer with test data
  for (nframes_t i = 0; i < max_block_length_; i++)
    {
      input_port->buffers ()->setSample (0, static_cast<int> (i), 1.0f);
    }

  // Test Case 1: nframes exceeds max_block_length_
  {
    EngineProcessTimeInfo time_nfo{
      .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = max_block_length_ + 100 // Exceeds max_block_length_
    };

    EXPECT_CALL (*processor_, custom_process_block (::testing::_, ::testing::_))
      .WillOnce ([&] (EngineProcessTimeInfo corrected_time_nfo, auto &) {
        // Verify correction was applied
        EXPECT_EQ (corrected_time_nfo.nframes_, max_block_length_);
        EXPECT_EQ (corrected_time_nfo.local_offset_, 0);
        EXPECT_EQ (corrected_time_nfo.g_start_frame_w_offset_, 0);
      });

    processor_->process_block (time_nfo, *mock_transport_);
  }

  // Test Case 2: local_offset + nframes exceeds max_block_length_
  {
    EngineProcessTimeInfo time_nfo{
      .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 500,
      .nframes_ = 600 // 500 + 600 = 1100 > max_block_length_ (1024)
    };

    EXPECT_CALL (*processor_, custom_process_block (::testing::_, ::testing::_))
      .WillOnce ([&] (EngineProcessTimeInfo corrected_time_nfo, auto &) {
        // Verify correction was applied
        EXPECT_EQ (corrected_time_nfo.nframes_, max_block_length_ - 500);
        EXPECT_EQ (corrected_time_nfo.local_offset_, 500);
        EXPECT_EQ (corrected_time_nfo.g_start_frame_w_offset_, 500);
      });

    processor_->process_block (time_nfo, *mock_transport_);
  }

  // Test Case 3: local_offset >= max_block_length_ (should result in 0 frames)
  {
    EngineProcessTimeInfo time_nfo{
      .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = max_block_length_ + 50, // Exceeds max_block_length_
      .nframes_ = 100
    };

    EXPECT_CALL (*processor_, custom_process_block (::testing::_, ::testing::_))
      .WillOnce ([&] (EngineProcessTimeInfo corrected_time_nfo, auto &) {
        // Verify correction was applied - should process 0 frames
        EXPECT_EQ (corrected_time_nfo.nframes_, 0);
        EXPECT_LT (corrected_time_nfo.local_offset_, max_block_length_);
        EXPECT_LT (
          corrected_time_nfo.g_start_frame_w_offset_, max_block_length_);
      });

    processor_->process_block (time_nfo, *mock_transport_);
  }

  // Test Case 4: Edge case where local_offset + nframes equals
  // max_block_length_ (no correction needed)
  {
    EngineProcessTimeInfo time_nfo{
      .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = max_block_length_ // Exactly matches max_block_length_
    };

    EXPECT_CALL (*processor_, custom_process_block (::testing::_, ::testing::_))
      .WillOnce ([&] (EngineProcessTimeInfo corrected_time_nfo, auto &) {
        // Verify no correction was applied
        EXPECT_EQ (corrected_time_nfo.nframes_, max_block_length_);
        EXPECT_EQ (corrected_time_nfo.local_offset_, 0);
        EXPECT_EQ (corrected_time_nfo.g_start_frame_w_offset_, 0);
      });

    processor_->process_block (time_nfo, *mock_transport_);
  }

  // Test Case 5: Small offset with nframes exceeding max_block_length_
  {
    EngineProcessTimeInfo time_nfo{
      .g_start_frame_ = 1000,
      .g_start_frame_w_offset_ = 1000,
      .local_offset_ = 100,
      .nframes_ = max_block_length_ // 100 + 1024 > max_block_length_ (1024)
    };

    EXPECT_CALL (*processor_, custom_process_block (::testing::_, ::testing::_))
      .WillOnce ([&] (EngineProcessTimeInfo corrected_time_nfo, auto &) {
        // Verify correction was applied
        EXPECT_EQ (corrected_time_nfo.nframes_, max_block_length_ - 100);
        EXPECT_EQ (corrected_time_nfo.local_offset_, 100);
        EXPECT_EQ (corrected_time_nfo.g_start_frame_w_offset_, 1100);
      });

    processor_->process_block (time_nfo, *mock_transport_);
  }
}

} // namespace zrythm::dsp
