// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_connection.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::dsp
{

class PortConnectionTest : public ::testing::Test
{
protected:
  const PortUuid kTestSrcId{ QUuid::createUuid () };
  const PortUuid kTestDestId{ QUuid::createUuid () };

  void SetUp () override
  {
    // Create default connection for tests
    default_connection_ = std::make_unique<PortConnection> (
      kTestSrcId, kTestDestId, 1.0f, false, true);
  }

  std::unique_ptr<PortConnection> default_connection_;
};

TEST_F (PortConnectionTest, DefaultConstruction)
{
  PortConnection conn;
  EXPECT_TRUE (conn.src_id_.is_null ());
  EXPECT_TRUE (conn.dest_id_.is_null ());
  EXPECT_FLOAT_EQ (conn.multiplier_, 1.0f);
  EXPECT_FALSE (conn.locked_);
  EXPECT_TRUE (conn.enabled_);
  EXPECT_FALSE (conn.bipolar_);
  EXPECT_FALSE (conn.source_ch_to_destination_ch_mapping_.has_value ());
}

TEST_F (PortConnectionTest, ParameterizedConstruction)
{
  const PortUuid src{ QUuid::createUuid () };
  const PortUuid dest{ QUuid::createUuid () };
  PortConnection conn (src, dest, 0.5f, true, false);

  EXPECT_EQ (conn.src_id_, src);
  EXPECT_EQ (conn.dest_id_, dest);
  EXPECT_FLOAT_EQ (conn.multiplier_, 0.5f);
  EXPECT_TRUE (conn.locked_);
  EXPECT_FALSE (conn.enabled_);
  EXPECT_FALSE (conn.bipolar_);
  EXPECT_FALSE (conn.source_ch_to_destination_ch_mapping_.has_value ());
}

TEST_F (PortConnectionTest, UpdateMethod)
{
  default_connection_->update (0.75f, true, false);

  EXPECT_FLOAT_EQ (default_connection_->multiplier_, 0.75f);
  EXPECT_TRUE (default_connection_->locked_);
  EXPECT_FALSE (default_connection_->enabled_);
}

TEST_F (PortConnectionTest, EqualityOperator)
{
  PortConnection same1 (kTestSrcId, kTestDestId, 1.0f, false, true);
  PortConnection same2 (kTestSrcId, kTestDestId, 1.0f, false, true);
  PortConnection different (
    kTestSrcId, PortUuid{ QUuid::createUuid () }, 0.5f, true, false);

  EXPECT_TRUE (same1 == same2);
  EXPECT_FALSE (same1 == different);
  EXPECT_FALSE (same2 == different);
}

TEST_F (PortConnectionTest, JsonSerialization)
{
  default_connection_->set_bipolar (true);

  // Serialize
  nlohmann::json j;
  to_json (j, *default_connection_);

  // Deserialize
  PortConnection deserialized;
  from_json (j, deserialized);

  // Verify deserialized object
  EXPECT_EQ (deserialized.src_id_, kTestSrcId);
  EXPECT_EQ (deserialized.dest_id_, kTestDestId);
  EXPECT_FLOAT_EQ (deserialized.multiplier_, 1.0f);
  EXPECT_FALSE (deserialized.locked_);
  EXPECT_TRUE (deserialized.enabled_);
  EXPECT_TRUE (deserialized.bipolar_);
  EXPECT_FALSE (deserialized.source_ch_to_destination_ch_mapping_.has_value ());
}

TEST_F (PortConnectionTest, ChannelMapping)
{
  // Test setting channel mapping
  default_connection_->set_channel_mapping (2, 1);
  EXPECT_TRUE (
    default_connection_->source_ch_to_destination_ch_mapping_.has_value ());
  EXPECT_EQ (
    default_connection_->source_ch_to_destination_ch_mapping_->first, 2);
  EXPECT_EQ (
    default_connection_->source_ch_to_destination_ch_mapping_->second, 1);

  // Test unsetting channel mapping
  default_connection_->unset_channel_mapping ();
  EXPECT_FALSE (
    default_connection_->source_ch_to_destination_ch_mapping_.has_value ());
}

TEST_F (PortConnectionTest, CloneBehavior)
{
  // Create a modified connection to clone
  PortConnection original (kTestSrcId, kTestDestId, 0.8f, true, false);

  // Clone it
  auto clone = utils::clone_unique (original);

  // Verify clone matches original
  EXPECT_EQ (clone->src_id_, original.src_id_);
  EXPECT_EQ (clone->dest_id_, original.dest_id_);
  EXPECT_FLOAT_EQ (clone->multiplier_, original.multiplier_);
  EXPECT_EQ (clone->locked_, original.locked_);
  EXPECT_EQ (clone->enabled_, original.enabled_);
  EXPECT_EQ (clone->bipolar_, original.bipolar_);
  EXPECT_EQ (
    clone->source_ch_to_destination_ch_mapping_,
    original.source_ch_to_destination_ch_mapping_);
}

} // namespace zrythm::dsp
