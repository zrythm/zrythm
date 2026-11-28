// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/clipboard.h"
#include "dsp/parameter.h"
#include "frontends/gui/backend/project_session.h"
#include "frontends/gui/qquick/qfuture_qml_wrapper.h"
#include "frontends/mcp/mcp_service.h"
#include "utils/object_registry.h"
#include "utils/qt.h"
#include "utils/registry_utils.h"

#include <QSignalSpy>

#include "helpers/project_fixture.h"
#include "helpers/qt_helpers.h"
#include "helpers/scoped_env_var.h"

#include "operations/operation.h"
#include "operations/operation_registry.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::frontends::gui
{

class ProjectSessionOperationsTest : public test_helpers::ProjectTestFixture
{
};

namespace
{

/**
 * Dispatches @p id with the current revision of the session's undo
 * stack.
 */
operations::OperationReport
dispatch (ProjectSession &session, operations::OperationId id, nlohmann::json args)
{
  const operations::DispatchOptions options{
    .expected_revision = session.undoStack ()->revision ()
  };
  return session.operation_registry ()->dispatch (
    id, args, *session.operation_environment (), options);
}

} // namespace

// Application-scope parameters resolve through the session's composite
// registry, so control-room state accepts live writes from any frontend
TEST_F (ProjectSessionOperationsTest, SessionLiveWritesApplicationParameter)
{
  auto clipboard = utils::make_qobject_unique<controllers::Clipboard> ();
  auto project = create_minimal_project_without_recording_callback ();
  utils::QObjectUniquePtr<structure::project::Project> project_ptr (
    project.release ());
  utils::ObjectRegistry app_registry;
  auto app_param_ref = utils::create_object<dsp::ProcessorParameter> (
    app_registry, app_registry,
    dsp::ProcessorParameter::UniqueId (u8"monitor_level"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
    utils::Utf8String::from_utf8_encoded_string ("Monitor Level"));
  auto * app_param = app_param_ref.get ();
  ASSERT_NE (app_param, nullptr);
  app_param->setBaseValue (0.5f);

  auto session = utils::make_qobject_unique<ProjectSession> (
    *app_settings_, *clipboard, app_registry, std::move (project_ptr));

  const auto param_uuid =
    type_safe::get (app_param->get_uuid ())
      .toString (QUuid::WithoutBraces)
      .toStdString ();
  const auto report = dispatch (
    *session,
    {
      "parameter", "set_value_live"
  },
    { { "parameter", param_uuid }, { "value", 0.9 } });
  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_FLOAT_EQ (app_param->baseValue (), 0.9f);
  EXPECT_EQ (session->undoStack ()->index (), 0);

  // the undoable variant refuses application state: the undo history is
  // project-scoped
  const auto edit_report = dispatch (
    *session,
    {
      "parameter", "set_value"
  },
    { { "parameter", param_uuid }, { "value", 0.2 } });
  EXPECT_TRUE (edit_report.refused ());
  EXPECT_FLOAT_EQ (app_param->baseValue (), 0.9f);
  EXPECT_EQ (session->undoStack ()->index (), 0);
}

// The session constructs the environment and registry from its own
// services, so every frontend dispatches against the same project state
// and the same undo stack
TEST_F (ProjectSessionOperationsTest, SessionDispatchesOperations)
{
  auto clipboard = utils::make_qobject_unique<controllers::Clipboard> ();
  auto project = create_minimal_project_without_recording_callback ();
  utils::QObjectUniquePtr<structure::project::Project> project_ptr (
    project.release ());
  utils::ObjectRegistry app_registry;
  auto                  session = utils::make_qobject_unique<ProjectSession> (
    *app_settings_, *clipboard, app_registry, std::move (project_ptr));

  // tracks are created after the session installed its recording callback
  session->project ()->add_default_tracks ();

  auto report =
    dispatch (*session, { "track", "list" }, nlohmann::json::object ());
  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_GT (report.result.at ("total_count").get<int> (), 0);

  // an edit flows through the session's undo stack
  const auto uuid =
    report.result.at ("items").at (0).at ("uuid").get<std::string> ();
  auto applied = dispatch (
    *session,
    {
      "track", "set_name"
  },
    { { "track", uuid }, { "new_name", "Session Renamed" } });
  ASSERT_TRUE (applied.applied ()) << applied.message;
  EXPECT_GT (applied.revision_after, applied.revision_before);
  EXPECT_EQ (session->undoStack ()->index (), 1);

  session->undoStack ()->undo ();
  EXPECT_EQ (session->undoStack ()->index (), 0);
}

// hasUnsavedChanges follows the undo history position, and a successful
// save records the position at the start of the save
TEST_F (ProjectSessionOperationsTest, SessionTracksUnsavedChanges)
{
  // The provider constructor never touches the OS clipboard, which this
  // test environment does not provide
  auto clipboard = utils::make_qobject_unique<controllers::Clipboard> (
    [] () -> QString { return {}; }, [] (const QString &) { });
  auto project = create_minimal_project_without_recording_callback ();
  utils::QObjectUniquePtr<structure::project::Project> project_ptr (
    project.release ());
  utils::ObjectRegistry app_registry;
  auto                  session = utils::make_qobject_unique<ProjectSession> (
    *app_settings_, *clipboard, app_registry, std::move (project_ptr));
  session->project ()->add_default_tracks ();

  QSignalSpy changed (session.get (), &ProjectSession::hasUnsavedChangesChanged);
  EXPECT_FALSE (session->hasUnsavedChanges ());

  auto report =
    dispatch (*session, { "track", "list" }, nlohmann::json::object ());
  ASSERT_TRUE (report.applied ()) << report.message;
  const auto uuid =
    report.result.at ("items").at (0).at ("uuid").get<std::string> ();
  auto applied = dispatch (
    *session,
    {
      "track", "set_name"
  },
    { { "track", uuid }, { "new_name", "Unsaved Rename" } });
  ASSERT_TRUE (applied.applied ()) << applied.message;
  EXPECT_TRUE (session->hasUnsavedChanges ());
  EXPECT_EQ (changed.size (), 1);

  session->undoStack ()->undo ();
  EXPECT_FALSE (session->hasUnsavedChanges ());

  session->undoStack ()->redo ();
  EXPECT_TRUE (session->hasUnsavedChanges ());

  session->setProjectDirectory (
    utils::Utf8String::from_path (project_dir_).to_qstring ());
  const std::unique_ptr<qquick::QFutureQmlWrapper> wrapper (session->save ());
  QSignalSpy saved (wrapper.get (), &qquick::QFutureQmlWrapper::succeeded);
  ASSERT_TRUE (saved.wait (test_helpers::kFutureWaitTimeout));
  EXPECT_FALSE (session->hasUnsavedChanges ());
  EXPECT_EQ (changed.size (), 4);
}

class ProjectSessionMcpTest : public test_helpers::ProjectTestFixture
{
protected:
  void SetUp () override
  {
    ProjectTestFixture::SetUp ();
    // The settings decide whether the endpoint starts.
    qunsetenv ("ZRYTHM_MCP_TOKEN");
    qunsetenv ("ZRYTHM_MCP_PORT");
  }

  /** Creates the session. Its endpoint follows the MCP settings. */
  void make_session ()
  {
    // The provider constructor never touches the OS clipboard, which
    // this test environment does not provide
    clipboard_ = utils::make_qobject_unique<controllers::Clipboard> (
      [] () -> QString { return {}; }, [] (const QString &) { });
    auto project = create_minimal_project_without_recording_callback ();
    utils::QObjectUniquePtr<structure::project::Project> project_ptr (
      project.release ());
    session_ = utils::make_qobject_unique<ProjectSession> (
      *app_settings_, *clipboard_, app_registry_, std::move (project_ptr));
  }

  test_helpers::ScopedEnvVar token_guard_{ "ZRYTHM_MCP_TOKEN" };
  test_helpers::ScopedEnvVar port_guard_{ "ZRYTHM_MCP_PORT" };
  utils::ObjectRegistry      app_registry_;
  utils::QObjectUniquePtr<controllers::Clipboard> clipboard_;
  utils::QObjectUniquePtr<ProjectSession>         session_;
};

// The session exposes its endpoint, and the endpoint follows the MCP
// settings: enabling the setting without a token leaves it Failed
TEST_F (ProjectSessionMcpTest, SessionExposesAnEndpointThatFollowsTheSettings)
{
  app_settings_->set_mcpEnabled (true);

  make_session ();
  ASSERT_NE (session_->mcpEndpoint (), nullptr);
  EXPECT_EQ (
    session_->mcpEndpoint ()->state (), mcp::McpEndpoint::State::Failed);
  EXPECT_FALSE (session_->mcpEndpoint ()->errorText ().isEmpty ());
}

} // namespace zrythm::frontends::gui
