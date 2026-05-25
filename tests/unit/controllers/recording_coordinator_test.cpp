// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/recording_coordinator.h"
#include "structure/tracks/track_fwd.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::controllers
{

using TrackUuid = structure::tracks::TrackUuid;
using SessionType = RecordingCoordinator::SessionType;

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
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  EXPECT_TRUE (coordinator_->has_session (track_uuid));
}

TEST_F (RecordingCoordinatorTest, DisarmTrackRemovesSession)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  coordinator_->disarm_track (track_uuid);
  EXPECT_FALSE (coordinator_->has_session (track_uuid));
}

TEST_F (RecordingCoordinatorTest, ProcessPendingDrainsAndDeletesPending)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  EXPECT_TRUE (coordinator_->has_session (track_uuid));

  coordinator_->disarm_track (track_uuid);
  EXPECT_FALSE (coordinator_->has_session (track_uuid));

  coordinator_->process_pending ();
  EXPECT_FALSE (coordinator_->has_session (track_uuid));
}

TEST_F (RecordingCoordinatorTest, DoubleArmDoesNothing)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  EXPECT_TRUE (coordinator_->has_session (track_uuid));
}

TEST_F (RecordingCoordinatorTest, DisarmNonExistentDoesNothing)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->disarm_track (track_uuid);
}

TEST_F (RecordingCoordinatorTest, GetSessionReturnsMonostateForUnknown)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  auto session = coordinator_->session_for_track (track_uuid);
  EXPECT_TRUE (std::holds_alternative<std::monostate> (session));
}

TEST_F (RecordingCoordinatorTest, GetSessionReturnsValidPointer)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  auto session = coordinator_->session_for_track (track_uuid);
  ASSERT_TRUE (std::holds_alternative<AudioRecordingSession *> (session));
  auto * s = std::get<AudioRecordingSession *> (session);
  EXPECT_EQ (s->state (), AudioRecordingSession::State::Armed);
}

TEST_F (RecordingCoordinatorTest, SessionAcceptsWritesAfterArm)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  auto session = coordinator_->session_for_track (track_uuid);
  ASSERT_TRUE (std::holds_alternative<AudioRecordingSession *> (session));
  auto * s = std::get<AudioRecordingSession *> (session);

  std::vector<float> l (256, 0.5f);
  std::vector<float> r (256, 0.3f);
  s->write (units::samples (0), true, l, r);
  EXPECT_EQ (s->state (), AudioRecordingSession::State::Capturing);
}

TEST_F (RecordingCoordinatorTest, SessionForTrackReturnsNullBeforeAnyArm)
{
  auto random_uuid = TrackUuid (QUuid::createUuid ());
  EXPECT_TRUE (
    std::holds_alternative<std::monostate> (
      coordinator_->session_for_track (random_uuid)));
}

TEST_F (RecordingCoordinatorTest, SessionForTrackReturnsNullAfterDisarm)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  EXPECT_TRUE (
    std::holds_alternative<AudioRecordingSession *> (
      coordinator_->session_for_track (track_uuid)));

  coordinator_->disarm_track (track_uuid);
  EXPECT_TRUE (
    std::holds_alternative<std::monostate> (
      coordinator_->session_for_track (track_uuid)));
}

TEST_F (RecordingCoordinatorTest, MultipleTracksArmedSimultaneously)
{
  auto track_a = TrackUuid (QUuid::createUuid ());
  auto track_b = TrackUuid (QUuid::createUuid ());

  coordinator_->arm_track (track_a, units::samples (256), SessionType::Audio);
  coordinator_->arm_track (track_b, units::samples (256), SessionType::Audio);

  EXPECT_TRUE (coordinator_->has_session (track_a));
  EXPECT_TRUE (coordinator_->has_session (track_b));
  EXPECT_TRUE (
    std::holds_alternative<AudioRecordingSession *> (
      coordinator_->session_for_track (track_a)));
  EXPECT_TRUE (
    std::holds_alternative<AudioRecordingSession *> (
      coordinator_->session_for_track (track_b)));

  auto session_a = coordinator_->session_for_track (track_a);
  auto session_b = coordinator_->session_for_track (track_b);
  EXPECT_NE (
    std::get<AudioRecordingSession *> (session_a),
    std::get<AudioRecordingSession *> (session_b));
}

TEST_F (RecordingCoordinatorTest, ProcessPendingClearsPendingDeletion)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  coordinator_->disarm_track (track_uuid);

  EXPECT_FALSE (coordinator_->has_session (track_uuid));
  EXPECT_TRUE (
    std::holds_alternative<std::monostate> (
      coordinator_->session_for_track (track_uuid)));

  coordinator_->process_pending ();

  EXPECT_FALSE (coordinator_->has_session (track_uuid));
  EXPECT_TRUE (
    std::holds_alternative<std::monostate> (
      coordinator_->session_for_track (track_uuid)));
}

TEST_F (RecordingCoordinatorTest, DisarmTrackUpdatesSnapshotImmediately)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  EXPECT_TRUE (
    std::holds_alternative<AudioRecordingSession *> (
      coordinator_->session_for_track (track_uuid)));

  coordinator_->disarm_track (track_uuid);
  EXPECT_FALSE (coordinator_->has_session (track_uuid));
  EXPECT_TRUE (
    std::holds_alternative<std::monostate> (
      coordinator_->session_for_track (track_uuid)));
}

TEST_F (RecordingCoordinatorTest, ArmDisarmReArm)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());

  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  EXPECT_TRUE (
    std::holds_alternative<AudioRecordingSession *> (
      coordinator_->session_for_track (track_uuid)));

  coordinator_->disarm_track (track_uuid);
  EXPECT_TRUE (
    std::holds_alternative<std::monostate> (
      coordinator_->session_for_track (track_uuid)));

  coordinator_->process_pending ();

  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  EXPECT_TRUE (
    std::holds_alternative<AudioRecordingSession *> (
      coordinator_->session_for_track (track_uuid)));
  EXPECT_TRUE (coordinator_->has_session (track_uuid));

  auto session = coordinator_->session_for_track (track_uuid);
  ASSERT_TRUE (std::holds_alternative<AudioRecordingSession *> (session));
  EXPECT_EQ (
    std::get<AudioRecordingSession *> (session)->state (),
    AudioRecordingSession::State::Armed);
}

TEST_F (
  RecordingCoordinatorTest,
  RecordingSessionEndedEmittedAfterDisarmLastTrack)
{
  auto       track_uuid = TrackUuid (QUuid::createUuid ());
  QSignalSpy spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (spy.isValid ());

  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  coordinator_->disarm_track (track_uuid);
  coordinator_->process_pending ();

  EXPECT_EQ (spy.count (), 1);
}

TEST_F (
  RecordingCoordinatorTest,
  RecordingSessionEndedNotEmittedWhenOtherTrackStillActive)
{
  auto       track_a = TrackUuid (QUuid::createUuid ());
  auto       track_b = TrackUuid (QUuid::createUuid ());
  QSignalSpy spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (spy.isValid ());

  coordinator_->arm_track (track_a, units::samples (256), SessionType::Audio);
  coordinator_->arm_track (track_b, units::samples (256), SessionType::Audio);

  coordinator_->disarm_track (track_a);
  coordinator_->process_pending ();

  EXPECT_EQ (spy.count (), 0);
}

TEST_F (RecordingCoordinatorTest, RecordingSessionEndedEmittedAfterAllDisarmed)
{
  auto       track_a = TrackUuid (QUuid::createUuid ());
  auto       track_b = TrackUuid (QUuid::createUuid ());
  QSignalSpy spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (spy.isValid ());

  coordinator_->arm_track (track_a, units::samples (256), SessionType::Audio);
  coordinator_->arm_track (track_b, units::samples (256), SessionType::Audio);

  coordinator_->disarm_track (track_a);
  coordinator_->process_pending ();
  EXPECT_EQ (spy.count (), 0);

  coordinator_->disarm_track (track_b);
  coordinator_->process_pending ();
  EXPECT_EQ (spy.count (), 1);
}

TEST_F (RecordingCoordinatorTest, NoSessionEndedWhenNoDisarm)
{
  QSignalSpy spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (spy.isValid ());

  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);
  coordinator_->process_pending ();

  EXPECT_EQ (spy.count (), 0);
}

TEST_F (RecordingCoordinatorTest, EndAllSessionsFinalizesAllSessions)
{
  auto       track_a = TrackUuid (QUuid::createUuid ());
  auto       track_b = TrackUuid (QUuid::createUuid ());
  QSignalSpy spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (spy.isValid ());

  coordinator_->arm_track (track_a, units::samples (256), SessionType::Audio);
  coordinator_->arm_track (track_b, units::samples (256), SessionType::Audio);

  coordinator_->finalizeAllSessions ();

  EXPECT_TRUE (coordinator_->has_session (track_a));
  EXPECT_TRUE (coordinator_->has_session (track_b));
  EXPECT_EQ (spy.count (), 1);
}

TEST_F (RecordingCoordinatorTest, EndAllSessionsWithNoSessionsDoesNotEmit)
{
  QSignalSpy ended_spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (ended_spy.isValid ());

  coordinator_->finalizeAllSessions ();

  EXPECT_EQ (ended_spy.count (), 0);
}

TEST_F (
  RecordingCoordinatorTest,
  EndAllSessionsDrainsRemainingDataBeforeSessionEnded)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);

  auto session = coordinator_->session_for_track (track_uuid);
  ASSERT_TRUE (std::holds_alternative<AudioRecordingSession *> (session));
  auto * s = std::get<AudioRecordingSession *> (session);

  std::vector<float> l (256, 0.5f);
  std::vector<float> r (256, 0.3f);
  s->write (units::samples (0), true, l, r);

  QSignalSpy data_spy (
    coordinator_.get (), &RecordingCoordinator::audioDataReady);
  QSignalSpy ended_spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (data_spy.isValid ());
  ASSERT_TRUE (ended_spy.isValid ());

  coordinator_->finalizeAllSessions ();

  EXPECT_EQ (data_spy.count (), 1)
    << "Pending data should be drained before session ended";
  EXPECT_EQ (ended_spy.count (), 1);
}

TEST_F (RecordingCoordinatorTest, EndAllSessionsResetsSessionForReuse)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);

  auto session = coordinator_->session_for_track (track_uuid);
  ASSERT_TRUE (std::holds_alternative<AudioRecordingSession *> (session));
  auto * s = std::get<AudioRecordingSession *> (session);

  std::vector<float> l (256, 0.5f);
  std::vector<float> r (256, 0.3f);
  s->write (units::samples (0), true, l, r);

  QSignalSpy ended_spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (ended_spy.isValid ());

  coordinator_->finalizeAllSessions ();

  EXPECT_TRUE (coordinator_->has_session (track_uuid))
    << "Session should survive finalizeAllSessions";
  EXPECT_EQ (ended_spy.count (), 1);
  EXPECT_EQ (s->state (), AudioRecordingSession::State::Armed)
    << "Session should be reset to Armed";

  s->write (units::samples (0), true, l, r);
  EXPECT_EQ (s->state (), AudioRecordingSession::State::Capturing)
    << "Reset session should accept writes again";
}

TEST_F (
  RecordingCoordinatorTest,
  ProcessPendingDoesNotEmitSessionEndedWithNoActivity)
{
  QSignalSpy ended_spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (ended_spy.isValid ());

  coordinator_->process_pending ();

  EXPECT_EQ (ended_spy.count (), 0)
    << "recordingSessionEnded should not be emitted when no recording "
       "activity occurred";
}

TEST_F (
  RecordingCoordinatorTest,
  FinalizeAllSessionsEmitsSessionEndedWhenAllTracksPreDisarmed)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Audio);

  auto session = coordinator_->session_for_track (track_uuid);
  ASSERT_TRUE (std::holds_alternative<AudioRecordingSession *> (session));
  auto * s = std::get<AudioRecordingSession *> (session);

  std::vector<float> l (256, 0.5f);
  std::vector<float> r (256, 0.3f);
  s->write (units::samples (0), true, l, r);

  QSignalSpy data_spy (
    coordinator_.get (), &RecordingCoordinator::audioDataReady);
  QSignalSpy ended_spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (data_spy.isValid ());
  ASSERT_TRUE (ended_spy.isValid ());

  coordinator_->disarm_track (track_uuid);

  coordinator_->finalizeAllSessions ();

  EXPECT_EQ (data_spy.count (), 1)
    << "Pending data from disarmed track should be drained";
  EXPECT_EQ (ended_spy.count (), 1)
    << "recordingSessionEnded should be emitted even when all tracks "
       "were disarmed before finalizeAllSessions";
}

// ============================================================================
// MIDI session tests
// ============================================================================

TEST_F (RecordingCoordinatorTest, ArmMidiTrackCreatesSession)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Midi);
  EXPECT_TRUE (coordinator_->has_session (track_uuid));
}

TEST_F (RecordingCoordinatorTest, MidiSessionReturnsCorrectVariant)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Midi);
  auto session = coordinator_->session_for_track (track_uuid);
  ASSERT_TRUE (std::holds_alternative<MidiRecordingSession *> (session));
  auto * s = std::get<MidiRecordingSession *> (session);
  EXPECT_EQ (s->state (), MidiRecordingSession::State::Armed);
}

TEST_F (RecordingCoordinatorTest, MidiSessionAcceptsWrites)
{
  auto track_uuid = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Midi);
  auto   session = coordinator_->session_for_track (track_uuid);
  auto * s = std::get<MidiRecordingSession *> (session);

  dsp::MidiEventVector events;
  events.add_note_on (1, 60, 100, units::samples (0u));
  s->write (units::samples (0), true, events, units::samples (256u));
  EXPECT_EQ (s->state (), MidiRecordingSession::State::Capturing);
}

TEST_F (RecordingCoordinatorTest, MidiDataReadyEmittedOnDrain)
{
  auto       track_uuid = TrackUuid (QUuid::createUuid ());
  QSignalSpy data_spy (
    coordinator_.get (), &RecordingCoordinator::midiDataReady);
  ASSERT_TRUE (data_spy.isValid ());

  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Midi);
  auto   session = coordinator_->session_for_track (track_uuid);
  auto * s = std::get<MidiRecordingSession *> (session);

  dsp::MidiEventVector events;
  events.add_note_on (1, 60, 100, units::samples (0u));
  s->write (units::samples (0), true, events, units::samples (256u));

  coordinator_->process_pending ();

  EXPECT_EQ (data_spy.count (), 1);
}

TEST_F (RecordingCoordinatorTest, MixedAudioAndMidiSessions)
{
  auto audio_uuid = TrackUuid (QUuid::createUuid ());
  auto midi_uuid = TrackUuid (QUuid::createUuid ());

  coordinator_->arm_track (audio_uuid, units::samples (256), SessionType::Audio);
  coordinator_->arm_track (midi_uuid, units::samples (256), SessionType::Midi);

  EXPECT_TRUE (
    std::holds_alternative<AudioRecordingSession *> (
      coordinator_->session_for_track (audio_uuid)));
  EXPECT_TRUE (
    std::holds_alternative<MidiRecordingSession *> (
      coordinator_->session_for_track (midi_uuid)));
}

TEST_F (RecordingCoordinatorTest, FinalizeAllSessionsHandlesMidi)
{
  auto       track_uuid = TrackUuid (QUuid::createUuid ());
  QSignalSpy ended_spy (
    coordinator_.get (), &RecordingCoordinator::recordingSessionEnded);
  ASSERT_TRUE (ended_spy.isValid ());

  coordinator_->arm_track (track_uuid, units::samples (256), SessionType::Midi);
  coordinator_->finalizeAllSessions ();

  EXPECT_TRUE (coordinator_->has_session (track_uuid));
  EXPECT_EQ (ended_spy.count (), 1);
}

}
