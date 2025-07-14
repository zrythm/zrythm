// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/graph_builder.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"

#include <QObject>

#include "./graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class TestProcessor : public ProcessorBase
{
public:
  TestProcessor (
    PortRegistry               &port_registry,
    ProcessorParameterRegistry &param_registry)
      : ProcessorBase (port_registry, param_registry, u8"TestProcessor"),
        input_port_ (port_registry.create_object<dsp::AudioPort> (
          u8"Input",
          PortFlow::Input)),
        output_port_ (port_registry.create_object<dsp::AudioPort> (
          u8"Output",
          PortFlow::Output))
  {
    add_input_port (input_port_);
    add_output_port (output_port_);
  }

  void custom_process_block (EngineProcessTimeInfo time_nfo) override
  {
    // Simple passthrough
    auto &in = input_port_.get_object_as<dsp::AudioPort> ()->buf_;
    auto &out = output_port_.get_object_as<dsp::AudioPort> ()->buf_;

    for (size_t i = 0; i < time_nfo.nframes_; i++)
      {
        out[i] = in[i] * 0.5f; // Process at half volume
      }
  }

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
  sample_rate_t                               sample_rate_{ 48000 };
  nframes_t                                   max_block_length_{ 1024 };
  std::unique_ptr<TestProcessor>              processor_;
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
  processor_->prepare_for_processing (sample_rate_, max_block_length_);

  // Verify ports are prepared
  auto input_port =
    processor_->get_input_ports ()[0].get_object_as<dsp::AudioPort> ();
  EXPECT_GE (input_port->buf_.size (), max_block_length_);

  processor_->release_resources ();
  EXPECT_EQ (input_port->buf_.size (), 0);
}

TEST_F (ProcessorBaseTest, Processing)
{
  processor_->prepare_for_processing (sample_rate_, max_block_length_);

  auto input_port =
    processor_->get_input_ports ()[0].get_object_as<dsp::AudioPort> ();
  auto output_port =
    processor_->get_output_ports ()[0].get_object_as<dsp::AudioPort> ();

  // Fill input buffer
  for (int i = 0; i < 512; i++)
    {
      input_port->buf_[i] = static_cast<float> (i) * 0.01f;
    }

  EngineProcessTimeInfo time_nfo{ 0, 0, 0, 512 };
  processor_->process_block (time_nfo);

  // Verify processing (half volume)
  for (int i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (output_port->buf_[i], i * 0.005f);
    }
}

TEST_F (ProcessorBaseTest, JsonSerializationRoundtrip)
{
  processor_->add_parameter (
    param_registry_->create_object<dsp::ProcessorParameter> (
      *port_registry_, dsp::ProcessorParameter::UniqueId (u8"unique-id"),
      dsp::ParameterRange::make_toggle (true), u8"my-toggle"));
  processor_->prepare_for_processing (sample_rate_, max_block_length_);

  // Serialize
  nlohmann::json j = *processor_;

  // Create new processor from JSON
  TestProcessor deserialized (*port_registry_, *param_registry_);
  from_json (j, deserialized);

  // Reinitialize after deserialization
  deserialized.prepare_for_processing (sample_rate_, max_block_length_);

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
  processor_->prepare_for_processing (sample_rate_, max_block_length_);

  // Create graph and transport
  graph_test::MockTransport transport;
  graph::Graph              graph;

  // Add processor to graph
  ProcessorGraphBuilder::add_nodes (graph, transport, *processor_);
  ProcessorGraphBuilder::add_connections (graph, *processor_);
  graph.finalize_nodes ();

  // Verify graph structure
  EXPECT_EQ (graph.get_nodes ().graph_nodes_.size (), 3);
  EXPECT_TRUE (graph.is_valid ());
}

} // namespace zrythm::dsp
