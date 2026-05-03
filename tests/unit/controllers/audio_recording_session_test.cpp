// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <thread>

#include "controllers/audio_recording_session.h"

#include <gtest/gtest.h>

namespace zrythm::controllers
{

class AudioRecordingSessionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    session_ = std::make_unique<AudioRecordingSession> (units::samples (256));
  }

  std::unique_ptr<AudioRecordingSession> session_;
};

TEST_F (AudioRecordingSessionTest, InitialStateIsArmed)
{
  EXPECT_EQ (session_->state (), AudioRecordingSession::State::Armed);
}

TEST_F (AudioRecordingSessionTest, WriteSamplesTransitionsToCapturing)
{
  std::vector<float> l (256, 0.5f);
  std::vector<float> r (256, 0.3f);
  session_->write_samples (units::samples (0), l, r);
  EXPECT_EQ (session_->state (), AudioRecordingSession::State::Capturing);
}

TEST_F (AudioRecordingSessionTest, DrainPendingReadsWrittenData)
{
  std::vector<float> l (256, 0.5f);
  std::vector<float> r (256, 0.3f);
  session_->write_samples (units::samples (0), l, r);

  auto packets = session_->drain_pending ();
  ASSERT_EQ (packets.size (), 1);
  EXPECT_EQ (packets[0].timeline_position, units::samples (0));
  EXPECT_EQ (packets[0].nframes, units::samples (256));
}

TEST_F (AudioRecordingSessionTest, DrainPendingContainsCorrectAudioData)
{
  std::vector<float> l (256);
  std::vector<float> r (256);
  for (size_t i = 0; i < 256; ++i)
    {
      l[i] = static_cast<float> (i) * 0.01f;
      r[i] = static_cast<float> (i) * -0.01f;
    }
  session_->write_samples (units::samples (0), l, r);

  auto packets = session_->drain_pending ();
  ASSERT_EQ (packets.size (), 1);
  ASSERT_EQ (packets[0].l_frames.size (), 256);
  ASSERT_EQ (packets[0].r_frames.size (), 256);
  EXPECT_FLOAT_EQ (packets[0].l_frames[0], 0.f);
  EXPECT_FLOAT_EQ (packets[0].l_frames[100], 1.f);
  EXPECT_FLOAT_EQ (packets[0].r_frames[100], -1.f);
}

TEST_F (AudioRecordingSessionTest, MultipleWritesBeforeDrain)
{
  std::vector<float> l (128, 0.1f);
  std::vector<float> r (128, 0.2f);

  session_->write_samples (units::samples (0), l, r);
  session_->write_samples (units::samples (128), l, r);

  auto packets = session_->drain_pending ();
  EXPECT_EQ (packets.size (), 2);
  EXPECT_EQ (packets[0].timeline_position, units::samples (0));
  EXPECT_EQ (packets[1].timeline_position, units::samples (128));
}

TEST_F (AudioRecordingSessionTest, DiscontinuityDetected)
{
  std::vector<float> l (128, 0.1f);
  std::vector<float> r (128, 0.2f);

  session_->write_samples (units::samples (0), l, r);
  session_->write_samples (units::samples (256), l, r);

  auto packets = session_->drain_pending ();
  EXPECT_EQ (packets.size (), 2);
  EXPECT_GT (
    packets[1].timeline_position,
    packets[0].timeline_position + packets[0].nframes);
}

TEST_F (AudioRecordingSessionTest, RegionTrackingStartsEmpty)
{
  EXPECT_TRUE (session_->recorded_regions ().empty ());
  EXPECT_EQ (session_->current_region (), nullptr);
}

TEST_F (AudioRecordingSessionTest, FinalizeTransitionsState)
{
  session_->finalize ();
  EXPECT_EQ (session_->state (), AudioRecordingSession::State::Finalizing);
}

TEST_F (AudioRecordingSessionTest, FinalizeRejectsFurtherWrites)
{
  std::vector<float> l (256, 0.5f);
  std::vector<float> r (256, 0.3f);

  session_->write_samples (units::samples (0), l, r);
  session_->finalize ();

  // Write after finalize should be rejected
  session_->write_samples (units::samples (256), l, r);

  auto packets = session_->drain_pending ();
  // Only the first write should have produced a packet
  EXPECT_EQ (packets.size (), 1);
}

TEST_F (AudioRecordingSessionTest, ResetClearsStateAndCounters)
{
  std::vector<float> l (128, 0.1f);
  std::vector<float> r (128, 0.2f);

  session_->write_samples (units::samples (0), l, r);
  session_->write_samples (units::samples (128), l, r);
  EXPECT_EQ (session_->state (), AudioRecordingSession::State::Capturing);

  session_->reset ();
  EXPECT_EQ (session_->state (), AudioRecordingSession::State::Armed);
  EXPECT_EQ (session_->dropped_packets (), 0u);
  EXPECT_TRUE (session_->recorded_regions ().empty ());
  EXPECT_EQ (session_->current_region (), nullptr);

  auto packets = session_->drain_pending ();
  EXPECT_EQ (packets.size (), 0u);
}

TEST_F (AudioRecordingSessionTest, ResetAllowsReuse)
{
  std::vector<float> l (128, 0.1f);
  std::vector<float> r (128, 0.2f);

  session_->write_samples (units::samples (0), l, r);
  session_->drain_pending ();
  session_->reset ();

  session_->write_samples (units::samples (1000), l, r);
  auto packets = session_->drain_pending ();
  ASSERT_EQ (packets.size (), 1u);
  EXPECT_EQ (packets[0].timeline_position, units::samples (1000));
}

TEST_F (AudioRecordingSessionTest, OverflowReusesSameSlot)
{
  const auto         capacity = AudioRecordingSession::kFifoCapacity;
  std::vector<float> l (128, 0.1f);
  std::vector<float> r (128, 0.2f);

  for (size_t i = 0; i < capacity; ++i)
    {
      session_->write_samples (
        units::samples (static_cast<int64_t> (i) * 128), l, r);
    }
  EXPECT_EQ (session_->dropped_packets (), 0u);

  session_->write_samples (units::samples (999'999), l, r);
  EXPECT_EQ (session_->dropped_packets (), 1u);

  auto packets = session_->drain_pending ();
  ASSERT_EQ (packets.size (), capacity);
  EXPECT_EQ (packets[0].timeline_position, units::samples (0));

  session_->write_samples (units::samples (1'234'567), l, r);
  EXPECT_EQ (session_->dropped_packets (), 1u);

  auto remaining = session_->drain_pending ();
  ASSERT_EQ (remaining.size (), 1u);
  EXPECT_EQ (remaining[0].timeline_position, units::samples (1'234'567));
}

TEST_F (AudioRecordingSessionTest, ConcurrentWriteAndDrain)
{
  static constexpr int num_writes = 500;
  std::vector<float>   l (256, 0.5f);
  std::vector<float>   r (256, 0.3f);

  std::atomic<int> writes_done{ 0 };

  std::vector<RecordingAudioPacket> all_packets;
  {
    std::jthread writer ([&] (std::stop_token) {
      for (int i = 0; i < num_writes; ++i)
        {
          session_->write_samples (
            units::samples (static_cast<int64_t> (i) * 256), l, r);
          writes_done.fetch_add (1, std::memory_order_relaxed);
        }
    });

    std::jthread reader ([&] (std::stop_token) {
      while (writes_done.load (std::memory_order_relaxed) < num_writes)
        {
          auto packets = session_->drain_pending ();
          all_packets.insert (
            all_packets.end (), std::make_move_iterator (packets.begin ()),
            std::make_move_iterator (packets.end ()));
          std::this_thread::sleep_for (std::chrono::microseconds (100));
        }
      auto remaining = session_->drain_pending ();
      all_packets.insert (
        all_packets.end (), std::make_move_iterator (remaining.begin ()),
        std::make_move_iterator (remaining.end ()));
    });
  }

  EXPECT_GT (all_packets.size (), 0u);
  EXPECT_EQ (
    all_packets.size () + session_->dropped_packets (),
    static_cast<size_t> (num_writes));

  for (size_t i = 1; i < all_packets.size (); ++i)
    {
      EXPECT_GE (
        all_packets[i].timeline_position, all_packets[i - 1].timeline_position)
        << "Packet " << i << " is out of order";
    }
}

TEST_F (AudioRecordingSessionTest, ConcurrentWriteAndDrainWithOverflow)
{
  static constexpr int num_writes = 500;
  std::vector<float>   l (256, 0.5f);
  std::vector<float>   r (256, 0.3f);
  std::atomic<int>     writes_done{ 0 };

  std::vector<RecordingAudioPacket> all_packets;
  {
    std::jthread writer ([&] (std::stop_token) {
      for (int i = 0; i < num_writes; ++i)
        {
          session_->write_samples (
            units::samples (static_cast<int64_t> (i) * 256), l, r);
          writes_done.fetch_add (1, std::memory_order_relaxed);
        }
    });

    std::jthread reader ([&] (std::stop_token) {
      while (writes_done.load (std::memory_order_relaxed) < num_writes)
        {
          auto packets = session_->drain_pending ();
          all_packets.insert (
            all_packets.end (), std::make_move_iterator (packets.begin ()),
            std::make_move_iterator (packets.end ()));
          std::this_thread::sleep_for (std::chrono::milliseconds (5));
        }
      auto remaining = session_->drain_pending ();
      all_packets.insert (
        all_packets.end (), std::make_move_iterator (remaining.begin ()),
        std::make_move_iterator (remaining.end ()));
    });
  }

  EXPECT_GT (all_packets.size (), 0u);
  EXPECT_GT (session_->dropped_packets (), 0u);
  EXPECT_EQ (
    all_packets.size () + session_->dropped_packets (),
    static_cast<size_t> (num_writes));

  for (size_t i = 1; i < all_packets.size (); ++i)
    {
      EXPECT_GE (
        all_packets[i].timeline_position, all_packets[i - 1].timeline_position)
        << "Packet " << i << " is out of order";
    }
}

} // namespace zrythm::controllers
