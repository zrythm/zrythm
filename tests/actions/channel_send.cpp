// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "zrythm-test-config.h"

#include "engine/session/graph_dispatcher.h"
#include "gui/backend/backend/actions/channel_send_action.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/region.h"
#include "structure/project/project.h"
#include "structure/tracks/master_track.h"

#include "tests/helpers/zrythm_helper.h"

TEST_F (ZrythmFixture, RouteMasterSendToFx)
{
  /* create audio fx track */
  auto audio_fx = Track::create_empty_with_action<AudioBusTrack> ();

  /* route master to it */
  ASSERT_THROW (
    UNDO_MANAGER->perform (
      std::make_unique<ChannelSendConnectStereoAction> (
        *P_MASTER_TRACK->channel_->sends_[0], *audio_fx->processor_->stereo_in_,
        *PORT_CONNECTIONS_MGR));
    , ZrythmException);
}
