// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/cpp/backend/actions/tracklist_selections.h"
#include "gui/cpp/backend/actions/undo_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

#include "common/plugins/plugin_manager.h"

#ifdef HAVE_HELM
static void
_test (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  /* 1. add helm */
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  /* select it */
  auto helm_track = TRACKLIST->get_last_track<ChannelTrack> ();
  helm_track->select (true, true, false);

  /* 2. delete track */
  UNDO_MANAGER->perform (std::make_unique<DeleteTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR));

  /* 3. undo track deletion */
  UNDO_MANAGER->undo ();

  /* let the engine run */
  g_usleep (1000000);

  /* 4. reload project */
  test_project_save_and_reload ();

  /* 5. redo track deletion */
  UNDO_MANAGER->redo ();

  /* 6. undo track deletion */
  UNDO_MANAGER->undo ();

  /* let the engine run */
  g_usleep (1000000);
}
#endif

TEST_F (ZrythmFixture, UndoRedoHelmTrackCreation)
{
#ifdef HAVE_HELM
  _test (HELM_BUNDLE, HELM_URI, true, false);
#endif
}