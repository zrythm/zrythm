// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/clipboard.h"
#include "gui/backend/project_session.h"
#include "utils/qt.h"

#include "helpers/project_fixture.h"

#include "operations/operation.h"
#include "operations/operation_registry.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::gui
{

class ProjectSessionOperationsTest : public test_helpers::ProjectTestFixture
{
};

namespace
{

operations::OperationReport
dispatch (ProjectSession &session, std::string_view name, nlohmann::json args)
{
  return session.operation_registry ()->dispatch (
    name, args, *session.operation_environment ());
}

} // namespace

// The session constructs the environment and registry from its own
// services, so every frontend dispatches against the same project state
// and the same undo stack
TEST_F (ProjectSessionOperationsTest, SessionDispatchesOperations)
{
  auto clipboard = utils::make_qobject_unique<controllers::Clipboard> ();
  auto project = create_minimal_project_without_recording_callback ();
  utils::QObjectUniquePtr<structure::project::Project> project_ptr (
    project.release ());
  auto session = utils::make_qobject_unique<ProjectSession> (
    *app_settings_, *clipboard, std::move (project_ptr));

  // tracks are created after the session installed its recording callback
  session->project ()->add_default_tracks ();

  auto report = dispatch (*session, "track.list", nlohmann::json::object ());
  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_GT (report.result.at ("total").get<int> (), 0);

  // an edit flows through the session's undo stack
  const auto uuid =
    report.result.at ("tracks").at (0).at ("uuid").get<std::string> ();
  auto applied = dispatch (
    *session, "track.set_name",
    {
      { "track",    uuid              },
      { "new_name", "Session Renamed" }
  });
  ASSERT_TRUE (applied.applied ()) << applied.message;
  EXPECT_GT (applied.revision_after, applied.revision_before);
  EXPECT_EQ (session->undoStack ()->index (), 1);

  session->undoStack ()->undo ();
  EXPECT_EQ (session->undoStack ()->index (), 0);
}

} // namespace zrythm::gui
