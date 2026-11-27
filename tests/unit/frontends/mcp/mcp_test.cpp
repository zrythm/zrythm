// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <ranges>
#include <utility>

#include "dsp/transport.h"
#include "frontends/mcp/mcp_service.h"
#include "frontends/mcp/mcp_tool_catalog.h"
#include "structure/project/port_connections_manager.h"
#include "structure/tracks/channel_send_routing.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"
#include "utils/composite_object_registry.h"
#include "utils/object_registry.h"
#include "utils/utf8_string.h"

#include <QSignalSpy>

#include "helpers/mcp_http_test_client.h"
#include "helpers/scoped_env_var.h"
#include "helpers/scoped_qcoreapplication.h"

#include "operations/builtin_operations.h"
#include "operations/operation_dispatcher.h"
#include "operations/operation_environment.h"
#include "operations/operation_registry.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::frontends::mcp
{

namespace
{
namespace http_test = zrythm::mcp::test;

/** The JSON report in the first text block of a tool call result. */
nlohmann::json
report_of (const nlohmann::json &result)
{
  const auto text = result.at ("content").at (0).at ("text").get<std::string> ();
  const auto report = nlohmann::json::parse (text, nullptr, false);
  EXPECT_FALSE (report.is_discarded ()) << text;
  return report;
}

/** Whether @p text is made of lowercase hex characters only. */
bool
is_lowercase_hex (const QString &text)
{
  return std::ranges::all_of (text, [] (QChar c) {
    return (c >= u'0' && c <= u'9') || (c >= u'a' && c <= u'f');
  });
}
} // namespace

class McpFrontendTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    singleton_tracks_ = std::make_unique<structure::tracks::SingletonTracks> ();
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (registry_);
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (registry_);
    channel_send_routing_ = std::make_unique<
      structure::tracks::ChannelSendRouting> (*track_collection_);

    structure::tracks::FinalTrackDependencies factory_deps = {
      tempo_map_wrapper_,
      registry_,
      soloed_tracks_exist_getter_,
      {},
    };
    track_factory_ = std::make_unique<structure::tracks::TrackFactory> (
      [factory_deps] () -> structure::tracks::FinalTrackDependencies {
        return factory_deps;
      });

    undo_stack_ = std::make_unique<undo::UndoStack> (
      [] (utils::InplaceCallback callback, bool) { callback (); });

    for (
      const auto type :
      { structure::tracks::Track::Type::Master,
        structure::tracks::Track::Type::Chord,
        structure::tracks::Track::Type::Marker })
      {
        auto ref = track_factory_->create_empty_track (type);
        switch (type)
          {
          case structure::tracks::Track::Type::Master:
            singleton_tracks_->setMasterTrack (
              ref.get_object_as<structure::tracks::MasterTrack> ());
            break;
          case structure::tracks::Track::Type::Chord:
            singleton_tracks_->setChordTrack (
              ref.get_object_as<structure::tracks::ChordTrack> ());
            break;
          case structure::tracks::Track::Type::Marker:
            singleton_tracks_->setMarkerTrack (
              ref.get_object_as<structure::tracks::MarkerTrack> ());
            break;
          default:
            break;
          }
        track_collection_->add_track (ref);
      }

    transport_config_ = {
      .return_to_cue_on_pause_ = [] { return false; },
      .metronome_countin_bars_ = [] { return 0; },
      .recording_preroll_bars_ = [] { return 0; },
    };
    transport_ =
      std::make_unique<dsp::Transport> (tempo_map_wrapper_, transport_config_);

    port_connections_manager_ =
      std::make_unique<structure::project::PortConnectionsManager> ();
    environment_ = std::make_unique<operations::OperationEnvironment> (
      composite_registry_, registry_, *undo_stack_, *track_collection_,
      *track_factory_, *track_routing_, *channel_send_routing_,
      *singleton_tracks_, *port_connections_manager_, transport_.get ());

    register_builtin_operations (operation_registry_);
    dispatcher_ = std::make_unique<operations::OperationDispatcher> (
      operation_registry_, *environment_);
  }

  /** Starts the endpoint with the given tokens and waits for it. */
  void start_service (const QList<McpToken> &tokens, quint16 port)
  {
    McpServiceConfig config;
    config.enabled = true;
    config.port = int (port);
    config.tokens = tokens;
    service_ = std::make_unique<McpService> (
      operation_registry_, *dispatcher_, *undo_stack_);
    QSignalSpy started (service_.get (), &McpService::started);
    service_->apply_config (config);
    ASSERT_TRUE (started.wait (10000)) << "the endpoint did not start";
  }

  structure::tracks::Track * add_audio_track ()
  {
    auto ref =
      track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
    auto * track = ref.get ();
    track_collection_->add_track (ref);
    return track;
  }

  /**
   * Sends a tools/call in the stateless revision and returns the result
   * that the server sent.
   */
  nlohmann::json stateless_call (
    http_test::McpHttpClient &client,
    const QByteArray         &token,
    int                       id,
    const std::string_view    tool,
    nlohmann::json            arguments = nlohmann::json::object ())
  {
    const auto response = client.send (
      http_test::McpHttpClient::Request{
    }
        .json_body (
          http_test::rpc_request (
            id, "tools/call",
            http_test::stateless_params (
              { { "name", std::string (tool) },
                { "arguments", std::move (arguments) } })))
        .stateless ("tools/call", tool)
        .bearer (token));
    EXPECT_EQ (response.status, 200) << response.body.toStdString ();
    return response.json ().value ("result", nlohmann::json ());
  }

  std::string uuid_of (const structure::tracks::Track &track) const
  {
    return std::string (
      utils::Utf8String::from_qstring (
        type_safe::get (track.get_uuid ()).toString (QUuid::WithoutBraces))
        .view ());
  }

  // Explicit constructors: direct brace-init is the only valid form.
  dsp::TempoMap                  tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper           tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry          registry_;
  utils::ObjectRegistry          app_registry_;
  utils::CompositeObjectRegistry composite_registry_ = {
    registry_, app_registry_
  };
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_ = { [] {
    return false;
  } };

  std::unique_ptr<structure::tracks::SingletonTracks>    singleton_tracks_;
  std::unique_ptr<structure::tracks::TrackCollection>    track_collection_;
  std::unique_ptr<structure::tracks::TrackRouting>       track_routing_;
  std::unique_ptr<structure::tracks::ChannelSendRouting> channel_send_routing_;
  std::unique_ptr<structure::tracks::TrackFactory>       track_factory_;
  std::unique_ptr<structure::project::PortConnectionsManager>
                                                    port_connections_manager_;
  std::unique_ptr<undo::UndoStack>                  undo_stack_;
  dsp::Transport::ConfigProvider                    transport_config_;
  std::unique_ptr<dsp::Transport>                   transport_;
  std::unique_ptr<operations::OperationEnvironment> environment_;
  operations::OperationRegistry                     operation_registry_;
  std::unique_ptr<operations::OperationDispatcher>  dispatcher_;

  /** Declared last so the endpoint stops before the state it touches. */
  std::unique_ptr<McpService> service_;
};

// ============================================================================
// Configuration parsing
// ============================================================================

TEST_F (McpFrontendTest, TokenEntryParsing)
{
  const auto full_token = McpToken::from_secret (
    QStringLiteral ("studio"), McpGrant::Full, QStringLiteral ("hunter2"));
  const auto query_token = McpToken::from_secret (
    QStringLiteral ("read-only"), McpGrant::Query, QStringLiteral ("hunter3"));
  const auto entries = QStringList{
    QStringLiteral ("studio|full|%1").arg (full_token.sha256_hex),
    QStringLiteral ("read-only|query|%1").arg (query_token.sha256_hex),
    QStringLiteral ("bad-entry"),
    QStringLiteral ("studio|full|nothex"),
    QStringLiteral ("studio|full|%1").arg (QString (64, u'x')),
    // The same secret as the studio entry: the duplicate is dropped.
    QStringLiteral ("clone|query|%1").arg (full_token.sha256_hex),
  };
  const auto parsed = parse_token_entries (entries);
  ASSERT_EQ (parsed.size (), 2);
  EXPECT_EQ (parsed.at (0).name, QStringLiteral ("studio"));
  EXPECT_EQ (parsed.at (0).grant, McpGrant::Full);
  EXPECT_EQ (parsed.at (1).name, QStringLiteral ("read-only"));
  EXPECT_EQ (parsed.at (1).grant, McpGrant::Query);
}

TEST_F (McpFrontendTest, EnvironmentConfig)
{
  // Unsets the variables on any exit, including assertion failures.
  struct EnvReset
  {
    ~EnvReset ()
    {
      qunsetenv ("ZRYTHM_MCP_TOKEN");
      qunsetenv ("ZRYTHM_MCP_PORT");
    }
  } env_reset;

  qputenv ("ZRYTHM_MCP_TOKEN", QByteArray ("ci:full:hunter2"));
  qputenv ("ZRYTHM_MCP_PORT", QByteArray ("8123"));
  const auto config = mcp_config_from_environment ();
  ASSERT_TRUE (config.has_value ());
  EXPECT_TRUE (config->enabled);
  EXPECT_EQ (config->port, 8123);
  ASSERT_EQ (config->tokens.size (), 1);
  EXPECT_EQ (config->tokens.at (0).name, QStringLiteral ("ci"));
  EXPECT_EQ (config->tokens.at (0).grant, McpGrant::Full);
  EXPECT_EQ (
    config->tokens.at (0).sha256_hex,
    McpToken::from_secret (
      QStringLiteral ("ci"), McpGrant::Full, QStringLiteral ("hunter2"))
      .sha256_hex);

  // A malformed specification is authoritative: it disables the
  // endpoint instead of falling back to the settings configuration.
  qputenv ("ZRYTHM_MCP_TOKEN", QByteArray ("ci:full:"));
  const auto empty_secret = mcp_config_from_environment ();
  ASSERT_TRUE (empty_secret.has_value ());
  EXPECT_FALSE (empty_secret->enabled);

  qputenv ("ZRYTHM_MCP_TOKEN", QByteArray ("ci:notagrant:hunter2"));
  const auto bad_grant = mcp_config_from_environment ();
  ASSERT_TRUE (bad_grant.has_value ());
  EXPECT_FALSE (bad_grant->enabled);

  qputenv ("ZRYTHM_MCP_TOKEN", QByteArray ("ci:full:hunter2"));
  qputenv ("ZRYTHM_MCP_PORT", QByteArray ("0"));
  const auto bad_port = mcp_config_from_environment ();
  ASSERT_TRUE (bad_port.has_value ());
  EXPECT_FALSE (bad_port->enabled);

  qunsetenv ("ZRYTHM_MCP_TOKEN");
  qunsetenv ("ZRYTHM_MCP_PORT");
  EXPECT_FALSE (mcp_config_from_environment ().has_value ());
}

TEST_F (McpFrontendTest, EnvironmentOverride)
{
  // Restores the variables on any exit, including assertion failures.
  test_helpers::ScopedEnvVar token_guard ("ZRYTHM_MCP_TOKEN");
  test_helpers::ScopedEnvVar port_guard ("ZRYTHM_MCP_PORT");

  qunsetenv ("ZRYTHM_MCP_TOKEN");
  qunsetenv ("ZRYTHM_MCP_PORT");
  EXPECT_EQ (mcp_environment_override (), McpEnvironmentOverride::None);

  qputenv ("ZRYTHM_MCP_TOKEN", QByteArray ("ci:full:hunter2"));
  EXPECT_EQ (mcp_environment_override (), McpEnvironmentOverride::Active);
  qputenv ("ZRYTHM_MCP_PORT", QByteArray ("8123"));
  EXPECT_EQ (mcp_environment_override (), McpEnvironmentOverride::Active);

  qputenv ("ZRYTHM_MCP_PORT", QByteArray ("70000"));
  EXPECT_EQ (mcp_environment_override (), McpEnvironmentOverride::Malformed);
  qunsetenv ("ZRYTHM_MCP_PORT");
  qputenv ("ZRYTHM_MCP_TOKEN", QByteArray ("ci:full:"));
  EXPECT_EQ (mcp_environment_override (), McpEnvironmentOverride::Malformed);
}

// ============================================================================
// Token creation and entry writing
// ============================================================================

TEST_F (McpFrontendTest, TokenEntriesRoundTrip)
{
  QList<McpToken> tokens;
  ASSERT_TRUE (
    add_token (tokens, QStringLiteral ("Studio Desk"), McpGrant::Full)
      .has_value ());
  ASSERT_TRUE (
    add_token (tokens, QStringLiteral ("Café"), McpGrant::Query).has_value ());
  tokens.append (
    McpToken::from_secret (
      QStringLiteral ("laptop:browser"), McpGrant::Control,
      QStringLiteral ("typed-secret")));

  const auto entries = write_token_entries (tokens);
  ASSERT_EQ (entries.size (), tokens.size ());
  EXPECT_EQ (parse_token_entries (entries), tokens);

  EXPECT_EQ (
    entries.front (),
    QStringLiteral ("Studio Desk|full|%1").arg (tokens.front ().sha256_hex));
  EXPECT_EQ (write_token_entries (parse_token_entries (entries)), entries);
}

TEST_F (McpFrontendTest, AddTokenRejectsInvalidNames)
{
  QList<McpToken> tokens;
  ASSERT_TRUE (
    add_token (tokens, QStringLiteral ("studio"), McpGrant::Full).has_value ());
  const auto before = tokens;

  const auto rejected = std::array{
    std::pair{ QString (),                 TokenRejection::EmptyName             },
    std::pair{ QStringLiteral ("   "),     TokenRejection::EmptyName             },
    std::pair{ QStringLiteral ("a|b"),     TokenRejection::ForbiddenCharacter    },
    std::pair{ QStringLiteral ("a:b"),     TokenRejection::ForbiddenCharacter    },
    std::pair{
              QStringLiteral (" studio"),  TokenRejection::SurroundingWhitespace },
    std::pair{
              QStringLiteral ("studio\t"), TokenRejection::SurroundingWhitespace },
  };
  for (const auto &[name, rejection] : rejected)
    {
      const auto result = add_token (tokens, name, McpGrant::Query);
      ASSERT_FALSE (result.has_value ());
      EXPECT_EQ (result.error (), rejection);
      EXPECT_EQ (tokens, before);
      EXPECT_EQ (validate_new_token_name (tokens, name).error (), rejection);
    }
}

TEST_F (McpFrontendTest, AddTokenRejectsDuplicateName)
{
  QList<McpToken> tokens;
  ASSERT_TRUE (
    add_token (tokens, QStringLiteral ("studio"), McpGrant::Full).has_value ());

  const auto duplicate =
    add_token (tokens, QStringLiteral ("studio"), McpGrant::Query);
  ASSERT_FALSE (duplicate.has_value ());
  EXPECT_EQ (duplicate.error (), TokenRejection::DuplicateName);
  ASSERT_EQ (tokens.size (), 1);
  EXPECT_EQ (tokens.front ().grant, McpGrant::Full);

  // Names compare case-sensitively.
  EXPECT_TRUE (
    add_token (tokens, QStringLiteral ("Studio"), McpGrant::Query).has_value ());
}

TEST_F (McpFrontendTest, AddTokenReturnsTheSecretItHashes)
{
  QList<McpToken> tokens;
  const auto secret = add_token (tokens, QStringLiteral ("one"), McpGrant::Full);
  const auto other = add_token (tokens, QStringLiteral ("two"), McpGrant::Query);
  ASSERT_TRUE (secret.has_value ());
  ASSERT_TRUE (other.has_value ());
  EXPECT_NE (*secret, *other);

  for (const auto &value : { *secret, *other })
    {
      EXPECT_EQ (value.size (), 64);
      EXPECT_TRUE (is_lowercase_hex (value));
    }
  ASSERT_EQ (tokens.size (), 2);
  EXPECT_EQ (
    McpToken::from_secret (QStringLiteral ("one"), McpGrant::Full, *secret),
    tokens.front ());
  EXPECT_EQ (
    McpToken::from_secret (QStringLiteral ("two"), McpGrant::Query, *other),
    tokens.back ());
}

TEST_F (McpFrontendTest, GeneratedSecretAuthenticates)
{
  QList<McpToken> tokens;
  const auto      secret =
    add_token (tokens, QStringLiteral ("generated"), McpGrant::Full);
  ASSERT_TRUE (secret.has_value ());

  const auto port = http_test::McpHttpClient::pick_free_port ();
  start_service (tokens, port);

  http_test::McpHttpClient client (port);
  const auto               response = client.send (
    http_test::McpHttpClient::Request{}
      .json_body (
        http_test::rpc_request (
          1, "tools/list",
          http_test::stateless_params (nlohmann::json::object ())))
      .stateless ("tools/list")
      .bearer (secret->toUtf8 ()));
  EXPECT_EQ (response.status, 200) << response.body.toStdString ();

  http_test::McpHttpClient anonymous (port);
  const auto               unauthenticated = anonymous.send (
    http_test::McpHttpClient::Request{}
      .json_body (
        http_test::rpc_request (
          2, "tools/list",
          http_test::stateless_params (nlohmann::json::object ())))
      .stateless ("tools/list"));
  EXPECT_EQ (unauthenticated.status, 401);
}

TEST_F (McpFrontendTest, McpPortRange)
{
  EXPECT_TRUE (is_valid_mcp_port (1));
  EXPECT_TRUE (is_valid_mcp_port (65535));
  EXPECT_FALSE (is_valid_mcp_port (0));
  EXPECT_FALSE (is_valid_mcp_port (65536));
  EXPECT_FALSE (is_valid_mcp_port (-1));
}

// ============================================================================
// Tool publication
// ============================================================================

TEST_F (McpFrontendTest, ToolCatalogMatchesRegistry)
{
  const auto catalog = make_tool_catalog (operation_registry_);
  ASSERT_TRUE (catalog.has_value ()) << catalog.error ();
  ASSERT_EQ (catalog->tools.size (), operation_registry_.operations ().size ());

  for (const auto &operation : operation_registry_.operations ())
    {
      const auto kind = catalog->kinds_by_name.find (operation.name ());
      ASSERT_NE (kind, catalog->kinds_by_name.end ())
        << "missing kind for " << operation.name ();
      EXPECT_EQ (kind->second, operation.kind ());

      const auto tool = std::ranges::find_if (
        catalog->tools, [&operation] (const zrythm::mcp::Tool &candidate) {
          return candidate.name () == operation.name ();
        });
      ASSERT_NE (tool, catalog->tools.end ());
      // The server publishes the definition of the registry unchanged.
      EXPECT_EQ (tool->to_json (), operation.tool_definition ())
        << operation.name ();
    }
}

// ============================================================================
// Worker lifecycle
// ============================================================================

TEST_F (McpFrontendTest, WorkerRefusesTokenlessStartAndKeepsJournalEntries)
{
  McpWorkerController worker ([] (const McpDispatchRequest &) { });
  QSignalSpy          errors (&worker, &McpWorkerController::server_error);
  QSignalSpy          started (&worker, &McpWorkerController::server_started);

  // An empty token map refuses to start instead of serving
  // unauthenticated callers.
  McpWorkerController::Config config;
  worker.start_server (config);
  EXPECT_EQ (errors.size (), 1);
  EXPECT_EQ (started.size (), 0);

  // Journal entries that arrive while no server exists are kept.
  worker.push_journal_entry (
    {
      { "revision", 1      },
      { "kind",     "push" }
  });
  worker.push_journal_entry (
    {
      { "revision", 2      },
      { "kind",     "undo" }
  });

  const auto port = http_test::McpHttpClient::pick_free_port ();
  config.port = port;
  config.token_hashes.insert (
    McpToken::from_secret (
      QStringLiteral ("full"), McpGrant::Full, QStringLiteral ("full-secret"))
      .sha256_hex,
    McpGrant::Full);
  config.catalog = std::move (*make_tool_catalog (operation_registry_));
  worker.start_server (std::move (config));
  EXPECT_EQ (started.size (), 1);

  // Both entries are served once the server runs.
  http_test::McpHttpClient client (port);
  const auto               response = client.send (
    http_test::McpHttpClient::Request{
  }
      .json_body (
        http_test::rpc_request (
          1, "resources/read",
          http_test::stateless_params ({ { "uri", "zrythm://journal" } })))
      .stateless ("resources/read", "zrythm://journal")
      .bearer ("full-secret"));
  ASSERT_EQ (response.status, 200) << response.body.toStdString ();
  const auto text =
    response.json ()
      .at ("result")
      .at ("contents")
      .at (0)
      .at ("text")
      .get<std::string> ();
  const auto entries = nlohmann::json::parse (text).at ("entries");
  ASSERT_EQ (entries.size (), 2);
  EXPECT_EQ (entries.at (0).at ("revision"), 1);
  EXPECT_EQ (entries.at (1).at ("kind"), "undo");
}

// ============================================================================
// Endpoint: stateless revision (2026-07-28)
// ============================================================================

TEST_F (McpFrontendTest, StatelessRevisionListsAndCallsTools)
{
  const auto port = http_test::McpHttpClient::pick_free_port ();
  start_service (
    { McpToken::from_secret (
        QStringLiteral ("query"), McpGrant::Query,
        QStringLiteral ("query-secret")),
      McpToken::from_secret (
        QStringLiteral ("full"), McpGrant::Full, QStringLiteral ("full-secret")) },
    port);

  http_test::McpHttpClient full_client (port);
  const QByteArray         full_token ("full-secret");

  // tools/list publishes the registry's operations.
  const auto list_response = full_client.send (
    http_test::McpHttpClient::Request{}
      .json_body (
        http_test::rpc_request (
          1, "tools/list",
          http_test::stateless_params (nlohmann::json::object ())))
      .stateless ("tools/list")
      .bearer (full_token));
  ASSERT_EQ (list_response.status, 200) << list_response.body.toStdString ();
  const auto tools = list_response.json ().at ("result").at ("tools");
  ASSERT_EQ (tools.size (), operation_registry_.operations ().size ());

  // A query tool call returns an applied report.
  auto *     track = add_audio_track ();
  const auto track_uuid = uuid_of (*track);
  const auto list_result =
    stateless_call (full_client, full_token, 2, "track.list");
  EXPECT_FALSE (list_result.value ("isError", false));
  EXPECT_EQ (report_of (list_result).at ("status"), "applied");

  // An edit with the matching expected revision is applied and lands in
  // the undo history.
  const auto revision = undo_stack_->revision ();
  const auto edit_result = stateless_call (
    full_client, full_token, 3, "track.set_name",
    {
      { "track",             track_uuid },
      { "new_name",          "Renamed"  },
      { "expected_revision", revision   }
  });
  EXPECT_FALSE (edit_result.value ("isError", false));
  const auto report = report_of (edit_result);
  EXPECT_EQ (report.at ("status"), "applied");
  EXPECT_GT (report.at ("revision_after").get<std::uint64_t> (), revision);
  EXPECT_EQ (undo_stack_->count (), 1);

  // The same edit with the stale revision is refused with the current
  // revision in the report.
  const auto stale_result = stateless_call (
    full_client, full_token, 4, "track.set_name",
    {
      { "track",             track_uuid },
      { "new_name",          "Stale"    },
      { "expected_revision", revision   }
  });
  EXPECT_TRUE (stale_result.at ("isError").get<bool> ());
  const auto stale_report = report_of (stale_result);
  EXPECT_EQ (stale_report.at ("status"), "refused");
  EXPECT_EQ (stale_report.at ("refusal_kind"), "stale_revision");
  EXPECT_EQ (stale_report.at ("revision_after"), report.at ("revision_after"));
  EXPECT_EQ (undo_stack_->count (), 1);

  // An expected revision that is not a non-negative integer is refused
  // instead of being narrowed to one.
  for (
    const auto &invalid :
    { nlohmann::json (-1), nlohmann::json (1.5), nlohmann::json (1e300),
      nlohmann::json ("1") })
    {
      const auto result = stateless_call (
        full_client, full_token, 7, "track.set_name",
        {
          { "track",             track_uuid },
          { "new_name",          "Invalid"  },
          { "expected_revision", invalid    }
      });
      EXPECT_TRUE (result.at ("isError").get<bool> ()) << invalid;
    }
  EXPECT_EQ (undo_stack_->count (), 1);

  // A query grant cannot edit.
  http_test::McpHttpClient query_client (port);
  const auto               refused_result = stateless_call (
    query_client, QByteArray ("query-secret"), 5, "track.set_name",
    {
      { "track",    track_uuid },
      { "new_name", "Nope"     }
  });
  EXPECT_TRUE (refused_result.at ("isError").get<bool> ());
  EXPECT_EQ (undo_stack_->count (), 1);

  // A request without a token is rejected by the transport.
  http_test::McpHttpClient anonymous_client (port);
  const auto               unauthenticated = anonymous_client.send (
    http_test::McpHttpClient::Request{}
      .json_body (
        http_test::rpc_request (
          6, "tools/list",
          http_test::stateless_params (nlohmann::json::object ())))
      .stateless ("tools/list"));
  EXPECT_EQ (unauthenticated.status, 401);
}

// ============================================================================
// Endpoint: legacy revision, journal notifications
// ============================================================================

TEST_F (McpFrontendTest, LegacyRevisionForwardsJournalEntries)
{
  const auto port = http_test::McpHttpClient::pick_free_port ();
  start_service (
    { McpToken::from_secret (
      QStringLiteral ("full"), McpGrant::Full, QStringLiteral ("full-secret")) },
    port);

  http_test::McpHttpClient client (port);
  const QByteArray         token ("full-secret");

  // Legacy initialize handshake.
  const auto initialize = client.send (
    http_test::McpHttpClient::Request{
  }
      .json_body (
        http_test::rpc_request (
          1, "initialize",
          nlohmann::json{
            { "protocolVersion", "2025-11-25" },
            { "capabilities", nlohmann::json::object () },
            { "clientInfo", { { "name", "test" }, { "version", "1" } } } }))
      .bearer (token));
  ASSERT_EQ (initialize.status, 200) << initialize.body.toStdString ();
  const auto session_id = initialize.header ("Mcp-Session-Id");
  ASSERT_FALSE (session_id.isEmpty ());

  const auto initialized = client.send (
    http_test::McpHttpClient::Request{}
      .json_body (http_test::rpc_notification ("notifications/initialized"))
      .session (session_id)
      .bearer (token));
  EXPECT_EQ (initialized.status, 202);

  // Subscribe to the journal resource on its own stream.
  auto stream = client.open_stream (
    http_test::McpHttpClient::Request{ .method = "GET" }
      .accept ("text/event-stream")
      .session (session_id)
      .bearer (token));
  ASSERT_EQ (stream->response ().status, 200);
  const auto subscribe = client.send (
    http_test::McpHttpClient::Request{
  }
      .json_body (
        http_test::rpc_request (
          2, "resources/subscribe",
          nlohmann::json{ { "uri", "zrythm://journal" } }))
      .session (session_id)
      .bearer (token));
  ASSERT_EQ (subscribe.status, 200) << subscribe.body.toStdString ();

  // A real edit through the operation registry pushes onto the real undo
  // stack; its journal entry must reach the subscribed client.
  auto *     track = add_audio_track ();
  const auto report = dispatcher_->dispatch (
    "track.set_name",
    nlohmann::json{
      { "track",    uuid_of (*track) },
      { "new_name", "Journal Test"   },
  });
  ASSERT_TRUE (report.applied ());
  ASSERT_EQ (undo_stack_->count (), 1);

  const auto event = stream->next_event ();
  ASSERT_TRUE (event.has_value ()) << "no journal notification arrived";
  EXPECT_EQ (event->json ().at ("method"), "notifications/resources/updated");
  EXPECT_EQ (event->json ().at ("params").at ("uri"), "zrythm://journal");

  // The resource carries the entry of the edit.
  const auto resource_response = client.send (
    http_test::McpHttpClient::Request{
  }
      .json_body (
        http_test::rpc_request (
          3, "resources/read", nlohmann::json{ { "uri", "zrythm://journal" } }))
      .session (session_id)
      .bearer (token));
  ASSERT_EQ (resource_response.status, 200)
    << resource_response.body.toStdString ();
  const auto text =
    resource_response.json ()
      .at ("result")
      .at ("contents")
      .at (0)
      .at ("text")
      .get<std::string> ();
  const auto entries = nlohmann::json::parse (text).at ("entries");
  ASSERT_EQ (entries.size (), 1);
  EXPECT_EQ (entries.at (0).at ("kind"), "push");
  EXPECT_EQ (
    entries.at (0).at ("revision").get<std::uint64_t> (), report.revision_after);
}

} // namespace zrythm::frontends::mcp
