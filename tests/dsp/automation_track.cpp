// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/automation_track.h"
#include "gui/dsp/master_track.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_F (ZrythmFixture, SetAutomationTrackIndex)
{
  auto master = P_MASTER_TRACK;
  master->set_automation_visible (true);
  auto &atl = master->get_automation_tracklist ();
  auto  first_vis_at = atl.visible_ats_.front ();

  /* create a region and set it as clip editor region */
  Position start, end;
  start.set_to_bar (2);
  end.set_to_bar (4);
  auto region = std::make_shared<AutomationRegion> (
    start, end, master->get_name_hash (), first_vis_at->index_, 0);
  master->add_region (region, first_vis_at, -1, true, false);
  region->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));

  CLIP_EDITOR->set_region (region.get (), false);

  auto first_invisible_at = atl.get_first_invisible_at ();
  atl.set_at_index (*first_invisible_at, first_vis_at->index_, false);

  /* check that clip editor region can be found */
  CLIP_EDITOR->get_region ();
}

/**
 * There was a case where get_muted() assumed
 * that the region was an audio/MIDI region and
 * threw errors.
 *
 * This replicates the issue and tests that this does not happen.
 */
TEST_F (ZrythmFixture, GetMutedOnRegionIn2ndAutomationTrack)
{
  auto master = P_MASTER_TRACK;
  master->set_automation_visible (true);
  auto &atl = master->get_automation_tracklist ();
  auto  first_vis_at = atl.visible_ats_.front ();

  /* create a new automation track */
  auto new_at = atl.get_first_invisible_at ();
  if (!new_at->created_)
    new_at->created_ = 1;
  atl.set_at_visible (*new_at, true);

  /* move it after the clicked automation track */
  atl.set_at_index (*new_at, first_vis_at->index_ + 1, true);

  /* create a region and set it as clip editor
   * region */
  Position start, end;
  start.set_to_bar (2);
  end.set_to_bar (4);
  auto region = std::make_shared<AutomationRegion> (
    start, end, master->get_name_hash (), new_at->index_, 0);
  master->add_region (region, new_at, -1, true, false);
  region->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));

  CLIP_EDITOR->set_region (region.get (), false);

  AUDIO_ENGINE->wait_n_cycles (3);

  /* assert not muted */
  ASSERT_FALSE (region->get_muted (true));
}

TEST_F (ZrythmFixture, CurveValue)
{
  /* stop engine to run manually */
  test_project_stop_dummy_engine ();

  auto master = P_MASTER_TRACK;
  master->set_automation_visible (true);
  /*AutomationTracklist * atl =*/
  /*track_get_automation_tracklist (master);*/
  auto fader_at = master->channel_->get_automation_track (
    PortIdentifier::Flags::ChannelFader);
  ASSERT_NONNULL (fader_at);
  if (!fader_at->created_)
    fader_at->created_ = true;
  auto atl = fader_at->get_automation_tracklist ();
  atl->set_at_visible (*fader_at, true);
  auto port = Port::find_from_identifier<ControlPort> (fader_at->port_id_);

  /* create region */
  Position start, end;
  start.set_to_bar (1);
  end.set_to_bar (5);
  auto region = std::make_shared<AutomationRegion> (
    start, end, master->get_name_hash (), fader_at->index_, 0);
  master->add_region (region, fader_at, -1, true, false);
  region->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));

  /* create a triangle curve and test the value at various
   * points */
  Position pos;
  pos.set_to_bar (1);
  auto ap = std::make_shared<AutomationPoint> (0.0f, 0.0f, pos);
  region->append_object (ap);
  ap->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*AUTOMATION_SELECTIONS));
  pos.set_to_bar (2);
  ap = std::make_shared<AutomationPoint> (2.0f, 1.0f, pos);
  region->append_object (ap);
  ap->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*AUTOMATION_SELECTIONS));
  pos.set_to_bar (3);
  ap = std::make_shared<AutomationPoint> (0.0f, 0.0f, pos);
  region->append_object (ap);
  ap->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*AUTOMATION_SELECTIONS));
  ASSERT_SIZE_EQ (region->aps_, 3);

  TRANSPORT->requestRoll (true);
  AUDIO_ENGINE->process (40);

  ASSERT_NEAR (port->control_, 2.32830644e-10, 0.0001f);

  pos.set_to_bar (3);
  pos.add_frames (-80);
  TRANSPORT->set_playhead_pos_rt_safe (pos);

  AUDIO_ENGINE->process (40);

  ASSERT_NEAR (port->control_, 2.32830644e-10, 0.0001f);
}
