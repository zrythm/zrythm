// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/recording_coordinator.h"
#include "structure/tracks/track_fwd.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::controllers
{

using TrackUuid = structure::tracks::TrackUuid;

class RecordingCoordinatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<zrythm::test_helpers::ScopedQCoreApplication> ();
    coordinator_ = std::make_unique<RecordingCoordinator> ();
  }

  void TearDown () override
  {
    coordinator_.reset ();
    app_.reset ();
  }

  std::unique_ptr<zrythm::test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<RecordingCoordinator>                         coordinator_;
};

TEST_F (RecordingCoordinatorTest, ArmTrackCreatesSession)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256));
  EXPECT_TRUE (coordinator_->has_session (track_uuid));
}

TEST_F (RecordingCoordinatorTest, DisarmTrackRemovesSession)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256));
  coordinator_->disarm_track (track_uuid);
  EXPECT_FALSE (coordinator_->has_session (track_uuid));
}

TEST_F (RecordingCoordinatorTest, ProcessPendingDrainsAndDeletesPending)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256));
  coordinator_->disarm_track (track_uuid);
  coordinator_->process_pending ();
}

TEST_F (RecordingCoordinatorTest, DoubleArmDoesNothing)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256));
  coordinator_->arm_track (track_uuid, units::samples (256));
  EXPECT_TRUE (coordinator_->has_session (track_uuid));
}

TEST_F (RecordingCoordinatorTest, DisarmNonExistentDoesNothing)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->disarm_track (track_uuid);
}

TEST_F (RecordingCoordinatorTest, GetSessionReturnsNullForUnknown)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  EXPECT_EQ (coordinator_->session_for_track (track_uuid), nullptr);
}

TEST_F (RecordingCoordinatorTest, GetSessionReturnsValidPointer)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256));
  auto * session = coordinator_->session_for_track (track_uuid);
  ASSERT_NE (session, nullptr);
  EXPECT_EQ (session->state (), AudioRecordingSession::State::Armed);
}

TEST_F (RecordingCoordinatorTest, SessionAcceptsWritesAfterArm)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256));
  auto * session = coordinator_->session_for_track (track_uuid);

  std::vector<float> l (256, 0.5f);
  std::vector<float> r (256, 0.3f);
  session->write_samples (units::samples (0), l, r);
  EXPECT_EQ (session->state (), AudioRecordingSession::State::Capturing);
}

}
