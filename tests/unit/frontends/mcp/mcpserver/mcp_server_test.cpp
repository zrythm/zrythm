// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <QAbstractSocket>
#include <QByteArray>
#include <QFuture>
#include <QHostAddress>
#include <QPromise>
#include <QTcpServer>
#include <QTest>

#include "helpers/mcp_http_test_client.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>
#include <mcpserver/json_rpc.h>
#include <mcpserver/mcp_server.h>
#include <mcpserver/resource_provider.h>
#include <nlohmann/json.hpp>

namespace zrythm::mcp
{

namespace
{

namespace http_test = zrythm::mcp::test;

using http_test::McpHttpClient;

/** The token that the server of the tests accepts. */
constexpr std::string_view kToken = "secret-token";

/** The identity of a client that authenticated with the token. */
constexpr std::string_view kIdentity = "token-holder";

/** The URI of the resource of the fake provider. */
constexpr std::string_view kNotesUri = "test://notes";

/** The instructions that the server of the tests publishes. */
constexpr std::string_view kInstructions = "The instructions of the test server";

/** The key of the protocol revision in the _meta of a request. */
constexpr std::string_view kProtocolVersionKey =
  "io.modelcontextprotocol/protocolVersion";

/** The key of the listen id in the _meta of a notification. */
constexpr std::string_view kSubscriptionIdKey =
  "io.modelcontextprotocol/subscriptionId";

/** The key of the server information in the _meta of a result. */
constexpr std::string_view kServerInfoKey = "io.modelcontextprotocol/serverInfo";

/** The identity of a client, as the authenticator of the tests returns it. */
struct TestIdentity
{
  std::string name;

  bool operator== (const TestIdentity &) const = default;
};

/** A provider of one text resource whose content the test controls. */
class FakeProvider final : public ResourceProvider
{
public:
  explicit FakeProvider (const std::string_view uri)
      : uri_ (*ResourceUri::parse (uri))
  {
  }

  auto resources () const -> std::vector<Resource> override
  {
    return {
      Resource{
               uri_, uri_.str (), "The resource " + uri_.str (), "text/plain",
               std::nullopt }
    };
  }

  auto read (const ResourceUri &uri) -> QFuture<ResourceContents> override
  {
    QPromise<ResourceContents> promise;
    promise.start ();
    if (uri == uri_)
      {
        promise.addResult (TextResourceContents{ uri_, text_, "text/plain" });
      }
    promise.finish ();
    return promise.future ();
  }

  /** Sets the text of the resource and reports the change. */
  auto set_text (std::string text) -> void
  {
    text_ = std::move (text);
    Q_EMIT resource_updated (uri_);
  }

  /** Reports that the list of the resources changed. */
  auto report_list_change () -> void { Q_EMIT resources_changed (); }

private:
  ResourceUri uri_;
  std::string text_{ "initial" };
};

/** A provider of one text resource whose resources() throws on demand. */
class ThrowingProvider final : public ResourceProvider
{
public:
  explicit ThrowingProvider (const std::string_view uri)
      : uri_ (*ResourceUri::parse (uri))
  {
  }

  auto resources () const -> std::vector<Resource> override
  {
    if (throws_)
      throw std::runtime_error ("the provider is broken");
    return {
      Resource{
               uri_, uri_.str (), "The resource " + uri_.str (), "text/plain",
               std::nullopt }
    };
  }

  auto read (const ResourceUri &) -> QFuture<ResourceContents> override
  {
    QPromise<ResourceContents> promise;
    promise.start ();
    promise.finish ();
    return promise.future ();
  }

  /** Makes resources() throw from now on. */
  auto break_resources () -> void { throws_ = true; }

private:
  ResourceUri  uri_;
  mutable bool throws_{};
};

/** A tool with an empty object schema. */
auto
make_tool (const std::string_view name) -> Tool
{
  const auto tool = Tool::from_json (
    nlohmann::json{
      { "name",        std::string (name)               },
      { "description", "The tool " + std::string (name) },
      { "inputSchema", { { "type", "object" } }         },
  });
  EXPECT_TRUE (tool.has_value ()) << tool.error ();
  return *tool;
}

/** The code of the JSON-RPC error of @p response, or 0 when there is none. */
auto
error_code (const McpHttpClient::Response &response) -> std::int32_t
{
  const auto json = response.json ();
  EXPECT_TRUE (json.contains ("error")) << response.body.toStdString ();
  return json.value ("error", nlohmann::json::object ()).value ("code", 0);
}

/** The error code @p code as a response carries it. */
constexpr auto
number_of (const JsonRpcErrorCode code) -> std::int32_t
{
  return static_cast<std::int32_t> (code);
}

/** The _meta key @p key as a string. */
auto
meta_key (const std::string_view key) -> std::string
{
  return std::string (key);
}

} // namespace

// ============================================================================
// Typed definitions
// ============================================================================

TEST (McpTypesTest, ToolDefinitionIsValidated)
{
  const nlohmann::json definition{
    { "name",        "echo"                   },
    { "inputSchema", { { "type", "object" } } },
  };
  const auto tool = Tool::from_json (definition);
  ASSERT_TRUE (tool.has_value ()) << tool.error ();
  // The definition is published unchanged.
  EXPECT_EQ (tool->to_json (), definition);

  // A tool needs a non-empty name and an object schema.
  EXPECT_FALSE (
    Tool::from_json (
      nlohmann::json{
        { "name",        ""                            },
        { "inputSchema", definition.at ("inputSchema") }
  })
      .has_value ());
  EXPECT_FALSE (
    Tool::from_json (
      nlohmann::json{
        { "name",        "echo"                   },
        { "inputSchema", { { "type", "string" } } }
  })
      .has_value ());
  EXPECT_FALSE (
    Tool::from_json (
      nlohmann::json{
        { "name", "echo" }
  })
      .has_value ());
  // Unknown members are refused.
  EXPECT_FALSE (
    Tool::from_json (
      nlohmann::json{
        { "name",        "echo"                        },
        { "colour",      "red"                         },
        { "inputSchema", definition.at ("inputSchema") }
  })
      .has_value ());
  // The x-mcp-header annotation is refused, also inside the properties.
  EXPECT_FALSE (
    Tool::from_json (
      nlohmann::json{
        { "name",        "echo"                                   },
        { "inputSchema",
         { { "type", "object" },
            { "properties",
              { { "Authorization",
                  { { "type", "string" },
                    { "x-mcp-header", "Authorization" } } } } } } }
  })
      .has_value ());
  // An annotation hint has to be a boolean.
  EXPECT_FALSE (
    Tool::from_json (
      nlohmann::json{
        { "name",        "echo"                        },
        { "inputSchema", definition.at ("inputSchema") },
        { "annotations", { { "readOnlyHint", "yes" } } }
  })
      .has_value ());
}

TEST (McpTypesTest, ResourceUriIsNeverEmpty)
{
  EXPECT_FALSE (ResourceUri::parse ("").has_value ());
  const auto uri = ResourceUri::parse ("test://notes");
  ASSERT_TRUE (uri.has_value ());
  EXPECT_EQ (uri->str (), "test://notes");
  EXPECT_EQ (*uri, *ResourceUri::parse ("test://notes"));
  EXPECT_NE (*uri, *ResourceUri::parse ("test://other"));
}

TEST (JsonRpcTest, MessageEnvelopeIsValidated)
{
  // A request has a string or a number as its id.
  const auto request =
    parse_json (R"({"jsonrpc":"2.0","id":7,"method":"tools/list"})");
  ASSERT_TRUE (request.has_value ());
  const auto message = parse_json_rpc_message (*request);
  ASSERT_TRUE (message.has_value ()) << message.error ().error.message;
  const auto * rpc = std::get_if<JsonRpcRequest> (&*message);
  ASSERT_NE (rpc, nullptr);
  EXPECT_EQ (rpc->method, "tools/list");
  EXPECT_FALSE (rpc->params.has_value ());

  // Batches, null ids, null params, a missing method and a value that is
  // not an object are invalid requests.
  for (
    const auto * const text : {
      R"([{"jsonrpc":"2.0","id":1,"method":"ping"}])",
      R"({"jsonrpc":"2.0","id":null,"method":"ping"})",
      R"({"jsonrpc":"2.0","id":1,"method":"ping","params":null})",
      R"({"jsonrpc":"2.0","id":1})",
      R"("hello")",
    })
    {
      const auto json = parse_json (text);
      ASSERT_TRUE (json.has_value ()) << text;
      const auto parsed = parse_json_rpc_message (*json);
      ASSERT_FALSE (parsed.has_value ()) << text;
      EXPECT_EQ (parsed.error ().error.code, JsonRpcErrorCode::InvalidRequest)
        << text;
    }
  // A notification has no id; text that is not JSON parses to nothing.
  const auto notification =
    parse_json (R"({"jsonrpc":"2.0","method":"notifications/initialized"})");
  ASSERT_TRUE (notification.has_value ());
  EXPECT_TRUE (
    std::holds_alternative<JsonRpcNotification> (
      *parse_json_rpc_message (*notification)));
  EXPECT_FALSE (parse_json ("{not json").has_value ());
}

// ============================================================================
// The server object
// ============================================================================

/** A server with an echo tool, a pending tool and one resource provider. */
class McpServerTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  /** What the server passed to the tool handler for one call. */
  struct Call
  {
    ToolCall         call;
    TestIdentity     identity;
    ProtocolRevision revision;
  };

  void SetUp () override { make_server ({}); }

  /** Builds the server with @p limits. A server that existed is dropped. */
  void make_server (ServerLimits limits)
  {
    provider_ = std::make_unique<FakeProvider> (kNotesUri);
    ServerOptions options;
    options.server = { "test-server", "1.0" };
    options.instructions = std::string (kInstructions);
    options.limits = limits;
    server_ = std::make_unique<Server> (std::move (options));

    server_->set_authenticator (
      [] (const HttpRequest &request) -> std::expected<TestIdentity, std::string> {
        if (
          request.header ("Authorization")
          == "Bearer " + http_test::to_bytes (kToken))
          {
            return TestIdentity{ std::string (kIdentity) };
          }
        return std::unexpected (std::string ("The token is not accepted"));
      });

    server_->set_call_tool_handler (
      [this] (const ToolCall &call, const Server::RequestContext &context)
        -> QFuture<CallToolResult> {
        calls_.push_back ({ call, context.identity, context.revision });
        if (call.name == "echo")
          {
            return QtFuture::makeReadyValueFuture (
              CallToolResult::text (
                call.arguments.has_value ()
                  ? call.arguments->dump ()
                  : std::string ("no arguments")));
          }
        // The test completes the call of the pending tool.
        pending_call_ = std::make_shared<QPromise<CallToolResult>> ();
        pending_call_->start ();
        return pending_call_->future ();
      });

    for (const auto &tool : { make_tool ("echo"), make_tool ("pending") })
      {
        const auto added = server_->add_tool (tool);
        ASSERT_TRUE (added.has_value ()) << added.error ();
      }
    server_->add_resource_provider (*provider_);
  }

  /** Starts the server on a free port. */
  void start ()
  {
    const auto listening = server_->listen (QHostAddress::LocalHost, 0);
    ASSERT_TRUE (listening.has_value ()) << listening.error ().message ();
    client_ = std::make_unique<McpHttpClient> (server_->listening_port ());
  }

  /** The POST of @p message, named as the stateless revision requires. */
  static auto stateless_post (
    const nlohmann::json  &message,
    const std::string_view method,
    const std::string_view name = {},
    const std::string_view version = http_test::kStatelessRevision)
    -> McpHttpClient::Request
  {
    return McpHttpClient::Request{}
      .json_body (message)
      .stateless (method, name, version)
      .bearer (http_test::to_bytes (kToken));
  }

  /**
   * The POST of a tools/call of @p tool.
   *
   * @p params holds the members of the params other than the name.
   */
  static auto call_post (
    const int              id,
    const std::string_view tool,
    nlohmann::json params = nlohmann::json::object ()) -> McpHttpClient::Request
  {
    params["name"] = std::string (tool);
    return stateless_post (
      http_test::rpc_request (
        id, "tools/call", http_test::stateless_params (params)),
      "tools/call", tool);
  }

  /** A listen request with the id @p id that asks for @p subscriptions. */
  static auto listen_request (
    const int                                      id,
    const std::optional<std::vector<std::string>> &subscriptions)
    -> McpHttpClient::Request
  {
    nlohmann::json notifications = nlohmann::json::object ();
    if (subscriptions.has_value ())
      {
        notifications["resourceSubscriptions"] = *subscriptions;
      }
    return stateless_post (
      http_test::rpc_request (
        id, "subscriptions/listen",
        http_test::stateless_params (
          {
            { "notifications", notifications }
    })),
      "subscriptions/listen");
  }

  /** A listen request with the id @p id that asks for the notes resource. */
  static auto listen_request (const int id) -> McpHttpClient::Request
  {
    return listen_request (id, std::vector{ std::string (kNotesUri) });
  }

  /** The initialize request of the legacy revision. */
  static auto legacy_initialize () -> McpHttpClient::Request
  {
    return McpHttpClient::Request{
    }
      .json_body (
        http_test::rpc_request (
          1, "initialize",
          nlohmann::json{
            { "protocolVersion", "2025-11-25" },
            { "capabilities", nlohmann::json::object () },
            { "clientInfo", { { "name", "test" }, { "version", "1" } } } }))
      .bearer (http_test::to_bytes (kToken));
  }

  /** A POST of @p message that belongs to the legacy session @p session_id. */
  static auto
  legacy_post (const nlohmann::json &message, const QByteArray &session_id)
    -> McpHttpClient::Request
  {
    return McpHttpClient::Request{}
      .json_body (message)
      .session (session_id)
      .bearer (http_test::to_bytes (kToken));
  }

  /** Starts a legacy session and returns its id. */
  auto start_legacy_session () -> QByteArray
  {
    const auto initialize = client_->send (legacy_initialize ());
    EXPECT_EQ (initialize.status, 200) << initialize.body.toStdString ();
    return initialize.header ("Mcp-Session-Id");
  }

  using Server = McpServer<TestIdentity>;

  std::unique_ptr<FakeProvider>  provider_;
  std::unique_ptr<Server>        server_;
  std::unique_ptr<McpHttpClient> client_;
  std::vector<Call>              calls_;

  /** The unfinished call of the pending tool. */
  std::shared_ptr<QPromise<CallToolResult>> pending_call_;
};

TEST_F (McpServerTest, ListenNeedsAnAuthenticator)
{
  ServerOptions options;
  options.server = { "test", "1.0" };
  McpServer<TestIdentity> server (std::move (options));
  const auto              refused = server.listen (QHostAddress::LocalHost, 0);
  ASSERT_FALSE (refused.has_value ());
  EXPECT_NE (
    refused.error ().message ().find ("authenticator"), std::string::npos);
  EXPECT_FALSE (server.is_listening ());
  EXPECT_EQ (server.listening_port (), 0);
}

TEST_F (McpServerTest, AddToolRefusesASecondToolWithTheSameName)
{
  const auto added = server_->add_tool (make_tool ("echo"));
  ASSERT_FALSE (added.has_value ());
  EXPECT_NE (added.error ().find ("echo"), std::string::npos);
  ASSERT_EQ (server_->tools ().size (), 2u);
  EXPECT_EQ (server_->tools ().front ().name (), "echo");
}

TEST_F (McpServerTest, ListenRefusesTwoProvidersOfTheSameResource)
{
  auto other = std::make_unique<FakeProvider> (kNotesUri);
  server_->add_resource_provider (*other);
  const auto refused = server_->listen (QHostAddress::LocalHost, 0);
  ASSERT_FALSE (refused.has_value ());
  EXPECT_NE (refused.error ().message ().find (kNotesUri), std::string::npos);
  EXPECT_FALSE (server_->is_listening ());
}

TEST_F (McpServerTest, ListenRefusesAConnectionLimitBelowTheStreams)
{
  // The other limits allow 16 listen streams and 16 sessions with 4 streams
  // each, and every stream holds a connection.
  make_server ({ .max_connections = 79 });
  const auto refused = server_->listen (QHostAddress::LocalHost, 0);
  ASSERT_FALSE (refused.has_value ());
  EXPECT_NE (
    refused.error ().message ().find ("max_connections"), std::string::npos);
  EXPECT_FALSE (server_->is_listening ());
}

TEST_F (McpServerTest, ListenReportsAPortThatIsInUse)
{
  // A second TCP server keeps the port busy for this test.
  QTcpServer holder;
  ASSERT_TRUE (holder.listen (QHostAddress::LocalHost, 0));
  const auto refused =
    server_->listen (QHostAddress::LocalHost, holder.serverPort ());
  ASSERT_FALSE (refused.has_value ());
  EXPECT_EQ (
    refused.error ().socket_error (), QAbstractSocket::AddressInUseError);
  EXPECT_FALSE (refused.error ().message ().empty ());
  EXPECT_FALSE (server_->is_listening ());
  EXPECT_EQ (server_->listening_port (), 0);
}

// ============================================================================
// Stateless revision: the transport
// ============================================================================

TEST_F (McpServerTest, RequestWithAnOriginIsRefused)
{
  start ();
  const auto response = client_->send (
    McpHttpClient::Request{}
      .json_body (
        http_test::rpc_request (
          1, "tools/list",
          http_test::stateless_params (nlohmann::json::object ())))
      .stateless ("tools/list")
      .header ("Origin", "https://example.com")
      .bearer (http_test::to_bytes (kToken)));
  EXPECT_EQ (response.status, 403);
}

TEST_F (McpServerTest, RequestWithoutATokenIsUnauthorized)
{
  start ();
  const auto response = client_->send (
    McpHttpClient::Request{}
      .json_body (
        http_test::rpc_request (
          1, "tools/list",
          http_test::stateless_params (nlohmann::json::object ())))
      .stateless ("tools/list"));
  EXPECT_EQ (response.status, 401);
  EXPECT_TRUE (response.header ("WWW-Authenticate").contains ("Bearer"))
    << response.raw_headers.toStdString ();
}

TEST_F (McpServerTest, AuthenticatorThatThrowsGetsAnInternalError)
{
  server_->set_authenticator (
    [] (const HttpRequest &) -> std::expected<TestIdentity, std::string> {
      throw std::runtime_error ("the authenticator is broken");
    });
  start ();
  const auto response = client_->send (stateless_post (
    http_test::rpc_request (
      1, "tools/list", http_test::stateless_params (nlohmann::json::object ())),
    "tools/list"));
  EXPECT_EQ (response.status, 500);
}

TEST_F (McpServerTest, RequestNeedsBothResponseMediaTypes)
{
  start ();
  const auto message = http_test::rpc_request (
    1, "tools/list", http_test::stateless_params (nlohmann::json::object ()));
  for (const auto * const accept : { "application/json", "text/event-stream" })
    {
      const auto response = client_->send (
        McpHttpClient::Request{}
          .raw_body (QByteArray::fromStdString (message.dump ()))
          .header ("Content-Type", "application/json")
          .header ("Accept", accept)
          .stateless ("tools/list")
          .bearer (http_test::to_bytes (kToken)));
      EXPECT_EQ (response.status, 406) << accept;
    }
  // The wildcard of a type stands for every subtype of the type.
  EXPECT_EQ (
    client_
      ->send (
        stateless_post (message, "tools/list").accept ("application/*, text/*"))
      .status,
    200);
}

TEST_F (McpServerTest, RequestNeedsAJsonContentType)
{
  start ();
  const auto message = http_test::rpc_request (
    1, "tools/list", http_test::stateless_params (nlohmann::json::object ()));
  const auto response = client_->send (
    McpHttpClient::Request{}
      .raw_body (QByteArray::fromStdString (message.dump ()))
      .header ("Content-Type", "text/plain")
      .header ("Accept", "application/json, text/event-stream")
      .stateless ("tools/list")
      .bearer (http_test::to_bytes (kToken)));
  EXPECT_EQ (response.status, 415);
}

TEST_F (McpServerTest, OversizedBodyIsRefused)
{
  make_server ({ .max_request_body_bytes = 64 });
  start ();
  const auto response = client_->send (stateless_post (
    http_test::rpc_request (
      1, "tools/list",
      http_test::stateless_params (
        {
          { "padding", std::string (200, 'x') }
  })),
    "tools/list"));
  EXPECT_EQ (response.status, 413);
}

TEST_F (McpServerTest, ConnectionOverTheLimitIsRefused)
{
  make_server (
    { .max_connections = 1,
      .max_listen_streams = 0,
      .max_legacy_sessions = 0,
      .max_streams_per_legacy_session = 0 });
  start ();
  // The first connection stays open until the tool call of its request ends.
  auto pending = client_->start_request (call_post (1, "pending"));
  ASSERT_TRUE (QTest::qWaitFor ([this] { return pending_call_ != nullptr; }))
    << "the server did not call the handler";
  // The HTTP server refuses the second connection.
  EXPECT_EQ (client_->send (call_post (2, "echo")).status, 429);
  pending_call_->addResult (CallToolResult::text ("done"));
  pending_call_->finish ();
  EXPECT_EQ (pending->wait ().status, 200);
}

TEST_F (McpServerTest, GetAndDeleteAreNotAllowed)
{
  start ();
  const auto version = http_test::to_bytes (http_test::kStatelessRevision);
  const auto get = client_->send (
    McpHttpClient::Request{ .method = "GET" }
      .header ("Accept", "text/event-stream")
      .header ("MCP-Protocol-Version", version)
      .bearer (http_test::to_bytes (kToken)));
  EXPECT_EQ (get.status, 405);
  EXPECT_TRUE (get.allows ("POST"));
  EXPECT_FALSE (get.allows ("GET"));
  const auto deleted = client_->send (
    McpHttpClient::Request{ .method = "DELETE" }
      .header ("MCP-Protocol-Version", version)
      .bearer (http_test::to_bytes (kToken)));
  EXPECT_EQ (deleted.status, 405);
}

TEST_F (McpServerTest, OtherPathIsNotFound)
{
  start ();
  auto request = stateless_post (
    http_test::rpc_request (
      1, "tools/list", http_test::stateless_params (nlohmann::json::object ())),
    "tools/list");
  request.path = "/other";
  EXPECT_EQ (client_->send (request).status, 404);
}

// ============================================================================
// Stateless revision: the protocol
// ============================================================================

TEST_F (McpServerTest, DiscoverListsTheRevisionsAndTheCapabilities)
{
  start ();
  const auto response = client_->send (stateless_post (
    http_test::rpc_request (
      1, "server/discover",
      http_test::stateless_params (nlohmann::json::object ())),
    "server/discover"));
  ASSERT_EQ (response.status, 200) << response.body.toStdString ();
  const auto json = response.json ();
  EXPECT_EQ (json.at ("id"), 1);
  const auto &result = json.at ("result");
  EXPECT_EQ (result.at ("resultType"), "complete");
  EXPECT_EQ (
    result.at ("supportedVersions"),
    (nlohmann::json{ "2026-07-28", "2025-11-25" }));
  EXPECT_EQ (result.at ("instructions"), kInstructions);
  EXPECT_EQ (
    result.at ("_meta").at (meta_key (kServerInfoKey)),
    (nlohmann::json{
      { "name",    "test-server" },
      { "version", "1.0"         }
  }));
  EXPECT_EQ (result.at ("ttlMs"), 0);
  EXPECT_EQ (result.at ("cacheScope"), "private");
  EXPECT_TRUE (result.at ("capabilities").contains ("tools"));
  EXPECT_FALSE (result.at ("capabilities").contains ("prompts"));
  // The stateless revision has no resources/subscribe.
  EXPECT_EQ (
    result.at ("capabilities").at ("resources").value ("subscribe", false),
    false);
}

TEST_F (McpServerTest, ToolsListAndCallServeTheTools)
{
  start ();
  const auto list = client_->send (stateless_post (
    http_test::rpc_request (
      1, "tools/list", http_test::stateless_params (nlohmann::json::object ())),
    "tools/list"));
  ASSERT_EQ (list.status, 200) << list.body.toStdString ();
  const auto tools = list.json ().at ("result").at ("tools");
  ASSERT_EQ (tools.size (), 2u);
  EXPECT_EQ (tools.at (0).at ("name"), "echo");
  EXPECT_EQ (tools.at (0).at ("inputSchema").at ("type"), "object");
  EXPECT_EQ (list.json ().at ("result").at ("ttlMs"), 0);

  const auto call = client_->send (call_post (
    2, "echo",
    {
      { "arguments", { { "text", "hello" } } }
  }));
  ASSERT_EQ (call.status, 200) << call.body.toStdString ();
  const auto  call_json = call.json ();
  const auto &result = call_json.at ("result");
  EXPECT_EQ (result.at ("content").at (0).at ("type"), "text");
  EXPECT_EQ (result.at ("content").at (0).at ("text"), R"({"text":"hello"})");
  EXPECT_FALSE (result.contains ("isError"));
  ASSERT_EQ (calls_.size (), 1u);
  EXPECT_EQ (calls_.front ().identity, TestIdentity{ std::string (kIdentity) });
  EXPECT_EQ (calls_.front ().revision, ProtocolRevision::V2026_07_28);
  ASSERT_TRUE (calls_.front ().call.arguments.has_value ());
  EXPECT_EQ (
    *calls_.front ().call.arguments,
    (nlohmann::json{
      { "text", "hello" }
  }));

  // Without arguments, the handler gets no arguments.
  const auto bare = client_->send (call_post (3, "echo"));
  ASSERT_EQ (bare.status, 200);
  EXPECT_EQ (
    bare.json ().at ("result").at ("content").at (0).at ("text"),
    "no arguments");
  ASSERT_EQ (calls_.size (), 2u);
  EXPECT_FALSE (calls_.back ().call.arguments.has_value ());
}

TEST_F (McpServerTest, CallToolArgumentsMustBeAnObject)
{
  start ();
  for (const auto &arguments : { nlohmann::json::array (), nlohmann::json () })
    {
      const auto response = client_->send (call_post (
        1, "echo",
        {
          { "arguments", arguments }
      }));
      EXPECT_EQ (response.status, 400);
      EXPECT_EQ (error_code (response), -32602);
    }
  EXPECT_TRUE (calls_.empty ());
}

TEST_F (McpServerTest, UnknownToolIsAnInvalidParamsError)
{
  start ();
  const auto response = client_->send (call_post (1, "no-such-tool"));
  EXPECT_EQ (response.status, 400);
  EXPECT_EQ (error_code (response), -32602);
  EXPECT_TRUE (calls_.empty ());
}

TEST_F (McpServerTest, UnknownMethodIsNotFound)
{
  start ();
  const auto response = client_->send (stateless_post (
    http_test::rpc_request (
      1, "tools/no-such-method",
      http_test::stateless_params (nlohmann::json::object ())),
    "tools/no-such-method"));
  EXPECT_EQ (response.status, 404);
  EXPECT_EQ (error_code (response), -32601);
}

TEST_F (McpServerTest, LegacyOnlyMethodsDoNotExistInTheStatelessRevision)
{
  start ();
  for (
    const auto * const method : { "initialize", "ping", "resources/subscribe" })
    {
      const auto response = client_->send (stateless_post (
        http_test::rpc_request (
          1, method, http_test::stateless_params (nlohmann::json::object ())),
        method));
      EXPECT_EQ (response.status, 404) << method;
      EXPECT_EQ (error_code (response), -32601) << method;
    }
}

TEST_F (McpServerTest, PromptsGetIsAnUnknownMethod)
{
  start ();
  // The server has no prompts, so the request gets the answer of an unknown
  // method without the Mcp-Name header.
  const auto response = client_->send (stateless_post (
    http_test::rpc_request (
      1, "prompts/get",
      http_test::stateless_params (
        {
          { "name", "x" }
  })),
    "prompts/get"));
  EXPECT_EQ (response.status, 404);
  EXPECT_EQ (error_code (response), -32601);
}

TEST_F (McpServerTest, MethodHeaderMustMatchTheBody)
{
  start ();
  const auto message = http_test::rpc_request (
    1, "tools/list", http_test::stateless_params (nlohmann::json::object ()));
  // The header is missing.
  const auto missing = client_->send (
    McpHttpClient::Request{}
      .json_body (message)
      .header (
        "MCP-Protocol-Version",
        http_test::to_bytes (http_test::kStatelessRevision))
      .bearer (http_test::to_bytes (kToken)));
  EXPECT_EQ (missing.status, 400);
  EXPECT_EQ (error_code (missing), -32020);
  // The header names another method.
  const auto other = client_->send (stateless_post (message, "tools/call"));
  EXPECT_EQ (other.status, 400);
  EXPECT_EQ (error_code (other), -32020);
  // The base64 sentinel format is for Mcp-Name only, so an encoded method
  // is a method that differs from the body.
  const auto encoded_method = client_->send (
    McpHttpClient::Request{}
      .json_body (message)
      .header (
        "MCP-Protocol-Version",
        http_test::to_bytes (http_test::kStatelessRevision))
      .header (
        "Mcp-Method",
        http_test::to_bytes (
          std::string ("=?base64?")
          + QByteArray ("tools/list").toBase64 ().toStdString () + "?="))
      .bearer (http_test::to_bytes (kToken)));
  EXPECT_EQ (error_code (encoded_method), -32020);
}

TEST_F (McpServerTest, NameHeaderMustMatchTheBody)
{
  start ();
  const auto call_with_name = [&] (const std::optional<std::string_view> name) {
    auto request =
      McpHttpClient::Request{
    }
        .json_body (
          http_test::rpc_request (
            1, "tools/call", http_test::stateless_params ({ { "name", "echo" } })))
        .stateless ("tools/call")
        .bearer (http_test::to_bytes (kToken));
    if (name.has_value ())
      {
        request.header ("Mcp-Name", http_test::to_bytes (*name));
      }
    return client_->send (request);
  };
  // A missing name and another name are refused.
  EXPECT_EQ (error_code (call_with_name (std::nullopt)), -32020);
  EXPECT_EQ (error_code (call_with_name ("other")), -32020);
  // A name with a byte outside the header range is refused.
  EXPECT_EQ (
    error_code (call_with_name (
      "ech\xc3"
      "\xb6"
      "o")),
    -32020);
  // A base64 encoded name is decoded before it is compared.
  const auto encoded =
    std::string ("=?base64?") + QByteArray ("echo").toBase64 ().toStdString ()
    + "?=";
  EXPECT_EQ (call_with_name (encoded).status, 200);
}

TEST_F (McpServerTest, MetaMustCarryTheRevisionAndTheClientCapabilities)
{
  start ();
  // The _meta names another revision than the header.
  auto other_revision = http_test::stateless_params (nlohmann::json::object ());
  other_revision["_meta"][meta_key (kProtocolVersionKey)] = "2025-11-25";
  const auto mismatch = client_->send (stateless_post (
    http_test::rpc_request (1, "tools/list", other_revision), "tools/list"));
  EXPECT_EQ (mismatch.status, 400);
  EXPECT_EQ (error_code (mismatch), -32020);

  // The _meta is missing, or lacks the client capabilities.
  for (
    const auto &params : {
      nlohmann::json::object (),
      nlohmann::json{
        { "_meta",
          { { meta_key (kProtocolVersionKey),
              std::string (http_test::kStatelessRevision) } } } },
    })
    {
      const auto response = client_->send (stateless_post (
        http_test::rpc_request (1, "tools/list", params), "tools/list"));
      EXPECT_EQ (response.status, 400);
      EXPECT_EQ (error_code (response), -32602);
    }
}

TEST_F (McpServerTest, UnsupportedRevisionIsRefused)
{
  start ();
  const auto response = client_->send (stateless_post (
    http_test::rpc_request (
      1, "tools/list", http_test::stateless_params (nlohmann::json::object ())),
    "tools/list", {}, "1999-01-01"));
  EXPECT_EQ (response.status, 400);
  EXPECT_EQ (error_code (response), -32022);
  const auto  error_json = response.json ();
  const auto &data = error_json.at ("error").at ("data");
  EXPECT_EQ (
    data.at ("supported"), (nlohmann::json{ "2026-07-28", "2025-11-25" }));
  EXPECT_EQ (data.at ("requested"), "1999-01-01");
  // The error carries the id of the request. A notification has no id.
  EXPECT_EQ (error_json.at ("id"), 1);
  const auto notification = client_->send (stateless_post (
    http_test::rpc_notification ("notifications/initialized"),
    "notifications/initialized", {}, "1999-01-01"));
  EXPECT_EQ (notification.status, 400);
  EXPECT_TRUE (notification.json ().at ("id").is_null ());
}

TEST_F (McpServerTest, MalformedMessageIsRefused)
{
  start ();
  const auto post_raw = [&] (const std::string &text) {
    return client_->send (
      McpHttpClient::Request{}
        .raw_body (QByteArray::fromStdString (text))
        .header ("Content-Type", "application/json")
        .header ("Accept", "application/json, text/event-stream")
        .stateless ("tools/list")
        .bearer (http_test::to_bytes (kToken)));
  };
  // Text that is not JSON is a parse error.
  const auto parse_error = post_raw ("{not json");
  EXPECT_EQ (parse_error.status, 400);
  EXPECT_EQ (error_code (parse_error), -32700);

  // A batch and null params are invalid requests.
  const auto message = http_test::rpc_request (
    1, "tools/list", http_test::stateless_params (nlohmann::json::object ()));
  auto null_params = http_test::rpc_request (1, "tools/list");
  null_params["params"] = nullptr;
  for (const auto &text : { "[" + message.dump () + "]", null_params.dump () })
    {
      const auto response = post_raw (text);
      EXPECT_EQ (response.status, 400) << text;
      EXPECT_EQ (error_code (response), -32600) << text;
    }
}

TEST_F (McpServerTest, NotificationIsAcceptedWithoutAResponseBody)
{
  start ();
  const auto response = client_->send (stateless_post (
    http_test::rpc_notification ("notifications/initialized"),
    "notifications/initialized"));
  EXPECT_EQ (response.status, 202);
  EXPECT_TRUE (response.body.isEmpty ());
}

TEST_F (McpServerTest, ClientResponseIsRefusedInTheStatelessRevision)
{
  start ();
  // The stateless revision sends no request, so a response has no meaning.
  const nlohmann::json client_response{
    { "jsonrpc", "2.0"                     },
    { "id",      1                         },
    { "result",  nlohmann::json::object () }
  };
  const auto stateless =
    client_->send (stateless_post (client_response, "none"));
  EXPECT_EQ (stateless.status, 400);
  EXPECT_EQ (
    error_code (stateless), number_of (JsonRpcErrorCode::InvalidRequest));
  EXPECT_TRUE (stateless.json ().at ("id").is_null ());
  // The session-based revision takes responses.
  const auto legacy = client_->send (
    McpHttpClient::Request{}
      .json_body (client_response)
      .bearer (http_test::to_bytes (kToken)));
  EXPECT_EQ (legacy.status, 202);
}

TEST_F (McpServerTest, ResourcesListMergesTheProviders)
{
  auto other = std::make_unique<FakeProvider> ("other://log");
  server_->add_resource_provider (*other);
  start ();
  const auto response = client_->send (stateless_post (
    http_test::rpc_request (
      1, "resources/list",
      http_test::stateless_params (nlohmann::json::object ())),
    "resources/list"));
  ASSERT_EQ (response.status, 200) << response.body.toStdString ();
  const auto resources = response.json ().at ("result").at ("resources");
  ASSERT_EQ (resources.size (), 2u);
  EXPECT_EQ (resources.at (0).at ("uri"), kNotesUri);
  EXPECT_EQ (resources.at (1).at ("uri"), "other://log");
}

TEST_F (McpServerTest, ResourceTemplatesListIsEmpty)
{
  start ();
  // The spec names the member resourceTemplates.
  const auto stateless = client_->send (stateless_post (
    http_test::rpc_request (
      1, "resources/templates/list",
      http_test::stateless_params (nlohmann::json::object ())),
    "resources/templates/list"));
  ASSERT_EQ (stateless.status, 200) << stateless.body.toStdString ();
  const auto  stateless_json = stateless.json ();
  const auto &stateless_result = stateless_json.at ("result");
  EXPECT_EQ (
    stateless_result.at ("resourceTemplates"), nlohmann::json::array ());
  EXPECT_FALSE (stateless_result.contains ("templates"));

  const auto session_id = start_legacy_session ();
  const auto legacy = client_->send (legacy_post (
    http_test::rpc_request (2, "resources/templates/list"), session_id));
  ASSERT_EQ (legacy.status, 200) << legacy.body.toStdString ();
  const auto  legacy_json = legacy.json ();
  const auto &legacy_result = legacy_json.at ("result");
  EXPECT_EQ (legacy_result.at ("resourceTemplates"), nlohmann::json::array ());
  EXPECT_FALSE (legacy_result.contains ("templates"));
}

TEST_F (McpServerTest, ProviderThatThrowsGetsAnInternalError)
{
  ThrowingProvider broken ("test://broken");
  server_->add_resource_provider (broken);
  start ();
  broken.break_resources ();
  const auto list = client_->send (stateless_post (
    http_test::rpc_request (
      1, "resources/list",
      http_test::stateless_params (nlohmann::json::object ())),
    "resources/list"));
  EXPECT_EQ (list.status, 500);
  EXPECT_EQ (error_code (list), number_of (JsonRpcErrorCode::InternalError));
  // A read of a URI that the first provider does not serve asks the broken
  // provider too.
  const auto read = client_->send (stateless_post (
    http_test::rpc_request (
      2, "resources/read",
      http_test::stateless_params (
        {
          { "uri", "test://no-such-resource" }
  })),
    "resources/read", "test://no-such-resource"));
  EXPECT_EQ (read.status, 500);
  EXPECT_EQ (error_code (read), number_of (JsonRpcErrorCode::InternalError));
}

TEST_F (McpServerTest, ResourceReadReturnsTheContentsOfTheProvider)
{
  start ();
  const auto response = client_->send (stateless_post (
    http_test::rpc_request (
      1, "resources/read",
      http_test::stateless_params (
        {
          { "uri", kNotesUri }
  })),
    "resources/read", kNotesUri));
  ASSERT_EQ (response.status, 200) << response.body.toStdString ();
  const auto contents = response.json ().at ("result").at ("contents");
  ASSERT_EQ (contents.size (), 1u);
  EXPECT_EQ (contents.at (0).at ("uri"), kNotesUri);
  EXPECT_EQ (contents.at (0).at ("text"), "initial");
  EXPECT_EQ (contents.at (0).at ("mimeType"), "text/plain");
}

TEST_F (McpServerTest, UnknownResourceIsNotFound)
{
  start ();
  const auto response = client_->send (stateless_post (
    http_test::rpc_request (
      1, "resources/read",
      http_test::stateless_params (
        {
          { "uri", "test://no-such-resource" }
  })),
    "resources/read", "test://no-such-resource"));
  EXPECT_EQ (response.status, 400);
  EXPECT_EQ (error_code (response), -32602);
}

TEST_F (McpServerTest, ToolCallWaitsForTheResultOfTheHandler)
{
  start ();
  auto pending = client_->start_request (call_post (1, "pending"));
  // The server answers once the future of the handler finishes.
  ASSERT_TRUE (QTest::qWaitFor ([this] { return pending_call_ != nullptr; }))
    << "the server did not call the handler";
  pending_call_->addResult (CallToolResult::text ("late"));
  pending_call_->finish ();
  const auto response = pending->wait ();
  EXPECT_EQ (response.status, 200) << response.body.toStdString ();
  EXPECT_EQ (
    response.json ().at ("result").at ("content").at (0).at ("text"), "late");
}

// ============================================================================
// Stateless revision: listen streams and connections
// ============================================================================

TEST_F (McpServerTest, ListenStreamReceivesTheResourcesItAskedFor)
{
  start ();
  McpHttpClient other_client (server_->listening_port ());
  // Two clients use the same request id; the server tells their streams
  // apart.
  auto subscribed = client_->open_stream (listen_request (5));
  auto uninterested =
    other_client.open_stream (listen_request (5, std::nullopt));
  ASSERT_EQ (subscribed->response ().status, 200);
  ASSERT_EQ (uninterested->response ().status, 200);

  const auto acknowledged = subscribed->next_event ();
  ASSERT_TRUE (acknowledged.has_value ());
  const auto acknowledged_json = acknowledged->json ();
  EXPECT_EQ (acknowledged->name, std::optional<QByteArray> ("message"));
  EXPECT_EQ (
    acknowledged_json.at ("method"), "notifications/subscriptions/acknowledged");
  const auto &params = acknowledged_json.at ("params");
  EXPECT_EQ (params.at ("_meta").at (meta_key (kSubscriptionIdKey)), 5);
  EXPECT_EQ (
    params.at ("notifications"),
    (nlohmann::json{
      { "resourceSubscriptions", { std::string (kNotesUri) } }
  }));
  const auto other_acknowledged = uninterested->next_event ();
  ASSERT_TRUE (other_acknowledged.has_value ());
  EXPECT_EQ (
    other_acknowledged->json ().at ("params").at ("notifications"),
    nlohmann::json::object ());

  provider_->set_text ("changed");

  const auto update = subscribed->next_event ();
  ASSERT_TRUE (update.has_value ()) << "the subscribed stream got no update";
  const auto update_json = update->json ();
  EXPECT_EQ (update_json.at ("method"), "notifications/resources/updated");
  EXPECT_EQ (update_json.at ("params").at ("uri"), kNotesUri);
  EXPECT_EQ (
    update_json.at ("params").at ("_meta").at (meta_key (kSubscriptionIdKey)),
    5);
  // The server sends no type that a client did not ask for.
  EXPECT_FALSE (
    uninterested->next_event (std::chrono::milliseconds{ 200 }).has_value ());
  EXPECT_FALSE (uninterested->ended ());
}

TEST_F (McpServerTest, ListenStreamGetsAListChangeOnlyWhenItAskedForIt)
{
  start ();
  auto listening = client_->open_stream (stateless_post (
    http_test::rpc_request (
      5, "subscriptions/listen",
      http_test::stateless_params (
        {
          { "notifications", { { "resourcesListChanged", true } } }
  })),
    "subscriptions/listen"));
  auto uninterested = client_->open_stream (listen_request (5, std::nullopt));
  ASSERT_EQ (listening->response ().status, 200);
  ASSERT_EQ (uninterested->response ().status, 200);
  ASSERT_TRUE (listening->next_event ().has_value ());
  ASSERT_TRUE (uninterested->next_event ().has_value ());

  provider_->report_list_change ();

  const auto changed = listening->next_event ();
  ASSERT_TRUE (changed.has_value ()) << "the stream got no list change";
  EXPECT_EQ (
    changed->json ().at ("method"), "notifications/resources/list_changed");
  EXPECT_EQ (
    changed->json ().at ("params").at ("_meta").at (
      meta_key (kSubscriptionIdKey)),
    5);
  // The server sends no type that a client did not ask for.
  EXPECT_FALSE (
    uninterested->next_event (std::chrono::milliseconds{ 200 }).has_value ());
}

TEST_F (McpServerTest, ClosedListenStreamIsReleased)
{
  make_server (
    {
      .max_listen_streams = 1,
      .stream_liveness_interval = std::chrono::milliseconds{ 20 },
    });
  start ();
  auto first = client_->open_stream (listen_request (1));
  ASSERT_EQ (first->response ().status, 200);
  // The limit is reached.
  const auto refused = client_->send (listen_request (1));
  EXPECT_EQ (refused.status, 429);
  EXPECT_EQ (error_code (refused), number_of (JsonRpcErrorCode::LimitReached));
  first.reset ();

  // The server notices the closed connection with its liveness check and
  // accepts a new stream.
  std::unique_ptr<McpHttpClient::Stream> second;
  ASSERT_TRUE (QTest::qWaitFor ([&] {
    second = client_->open_stream (listen_request (1));
    return second->response ().status == 200;
  }))
    << "the server kept the closed stream";
  const auto acknowledged = second->next_event ();
  ASSERT_TRUE (acknowledged.has_value ());
  provider_->set_text ("changed");
  const auto update = second->next_event ();
  ASSERT_TRUE (update.has_value ()) << "the new stream got no update";
  EXPECT_EQ (update->json ().at ("method"), "notifications/resources/updated");
}

TEST_F (McpServerTest, ListenStreamEndsWhenTheServerStops)
{
  start ();
  auto stream = client_->open_stream (listen_request (9));
  ASSERT_EQ (stream->response ().status, 200);
  ASSERT_TRUE (stream->next_event ().has_value ());

  server_->stop ();

  const auto last = stream->next_event ();
  ASSERT_TRUE (last.has_value ()) << "the stream ended without a final result";
  EXPECT_EQ (last->json ().at ("id"), 9);
  EXPECT_EQ (last->json ().at ("result").at ("resultType"), "complete");
  EXPECT_EQ (
    last->json ().at ("result").at ("_meta").at (meta_key (kSubscriptionIdKey)),
    9);
  EXPECT_FALSE (
    stream->next_event (std::chrono::milliseconds{ 200 }).has_value ());
  EXPECT_TRUE (stream->ended ());
  EXPECT_FALSE (server_->is_listening ());
}

TEST_F (McpServerTest, TwoRequestsOnOneConnectionKeepTheirIds)
{
  start ();
  const auto responses = client_->send_in_order (
    {
      call_post (1, "echo", { { "arguments", { { "text", "first" } } }  }
         ),
      call_post (2, "echo", { { "arguments", { { "text", "second" } } } }
         ),
  });
  ASSERT_EQ (responses.size (), 2u);
  EXPECT_EQ (responses.at (0).json ().at ("id"), 1);
  EXPECT_EQ (
    responses.at (0).json ().at ("result").at ("content").at (0).at ("text"),
    R"({"text":"first"})");
  EXPECT_EQ (responses.at (1).json ().at ("id"), 2);
  EXPECT_EQ (
    responses.at (1).json ().at ("result").at ("content").at (0).at ("text"),
    R"({"text":"second"})");
}

// ============================================================================
// Legacy revision
// ============================================================================

TEST_F (McpServerTest, LegacyInitializeStartsASession)
{
  start ();
  const auto initialize = client_->send (legacy_initialize ());
  ASSERT_EQ (initialize.status, 200) << initialize.body.toStdString ();
  const auto session_id = initialize.header ("Mcp-Session-Id");
  ASSERT_FALSE (session_id.isEmpty ());
  const auto  initialize_json = initialize.json ();
  const auto &result = initialize_json.at ("result");
  EXPECT_EQ (result.at ("protocolVersion"), "2025-11-25");
  EXPECT_EQ (
    result.at ("serverInfo"),
    (nlohmann::json{
      { "name",    "test-server" },
      { "version", "1.0"         }
  }));
  EXPECT_EQ (result.at ("instructions"), kInstructions);
  EXPECT_EQ (result.at ("capabilities").at ("resources").at ("subscribe"), true);

  // The server answers the revision that it supports when the client
  // names another one.
  const auto negotiated = client_->send (
    McpHttpClient::Request{
  }
      .json_body (
        http_test::rpc_request (
          1, "initialize",
          nlohmann::json{
            { "protocolVersion", "1999-01-01" },
            { "capabilities", nlohmann::json::object () },
            { "clientInfo", { { "name", "test" }, { "version", "1" } } } }))
      .bearer (http_test::to_bytes (kToken)));
  ASSERT_EQ (negotiated.status, 200) << negotiated.body.toStdString ();
  EXPECT_NE (negotiated.header ("Mcp-Session-Id"), session_id);
  EXPECT_EQ (
    negotiated.json ().at ("result").at ("protocolVersion"), "2025-11-25");
}

TEST_F (McpServerTest, LegacyRequestsNeedTheSession)
{
  start ();
  const auto message = http_test::rpc_request (1, "tools/list");
  const auto missing = client_->send (
    McpHttpClient::Request{}.json_body (message).bearer (
      http_test::to_bytes (kToken)));
  EXPECT_EQ (missing.status, 400);
  EXPECT_EQ (error_code (missing), -32600);
  const auto unknown = client_->send (legacy_post (message, "no-such-session"));
  EXPECT_EQ (unknown.status, 404);
  EXPECT_EQ (error_code (unknown), number_of (JsonRpcErrorCode::UnknownSession));
}

TEST_F (McpServerTest, LegacyStreamCarriesTheSubscribedUpdates)
{
  start ();
  const auto session_id = start_legacy_session ();
  ASSERT_FALSE (session_id.isEmpty ());

  auto stream = client_->open_stream (
    McpHttpClient::Request{ .method = "GET" }
      .header ("Accept", "text/event-stream")
      .session (session_id)
      .bearer (http_test::to_bytes (kToken)));
  ASSERT_EQ (stream->response ().status, 200);
  EXPECT_TRUE (
    stream->response ().header ("Content-Type").contains ("text/event-stream"));

  const auto subscribe = client_->send (legacy_post (
    http_test::rpc_request (
      2, "resources/subscribe",
      nlohmann::json{
        { "uri", kNotesUri }
  }),
    session_id));
  ASSERT_EQ (subscribe.status, 200) << subscribe.body.toStdString ();
  EXPECT_EQ (subscribe.json ().at ("result"), nlohmann::json::object ());

  provider_->set_text ("changed");
  const auto update = stream->next_event ();
  ASSERT_TRUE (update.has_value ()) << "the stream got no update";
  EXPECT_EQ (update->json ().at ("method"), "notifications/resources/updated");
  EXPECT_EQ (update->json ().at ("params").at ("uri"), kNotesUri);
  // The notification is not a response, so it has no id.
  EXPECT_FALSE (update->json ().contains ("id"));

  // The resource has the new text, and an unknown resource is not found.
  const auto read = client_->send (legacy_post (
    http_test::rpc_request (
      3, "resources/read",
      nlohmann::json{
        { "uri", kNotesUri }
  }),
    session_id));
  ASSERT_EQ (read.status, 200) << read.body.toStdString ();
  EXPECT_EQ (
    read.json ().at ("result").at ("contents").at (0).at ("text"), "changed");
  const auto missing = client_->send (legacy_post (
    http_test::rpc_request (
      4, "resources/read",
      nlohmann::json{
        { "uri", "test://no-such-resource" }
  }),
    session_id));
  EXPECT_EQ (missing.status, 200);
  EXPECT_EQ (error_code (missing), -32002);

  // A change of the resource list reaches every session with a stream.
  provider_->report_list_change ();
  const auto list_changed = stream->next_event ();
  ASSERT_TRUE (list_changed.has_value ());
  EXPECT_EQ (
    list_changed->json ().at ("method"), "notifications/resources/list_changed");
}

TEST_F (McpServerTest, LegacyPingAndNotifications)
{
  start ();
  const auto session_id = start_legacy_session ();
  ASSERT_FALSE (session_id.isEmpty ());
  const auto ping = client_->send (
    legacy_post (http_test::rpc_request (2, "ping"), session_id));
  ASSERT_EQ (ping.status, 200) << ping.body.toStdString ();
  EXPECT_EQ (ping.json ().at ("result"), nlohmann::json::object ());
  const auto notification = client_->send (legacy_post (
    http_test::rpc_notification ("notifications/initialized"), session_id));
  EXPECT_EQ (notification.status, 202);
  // A JSON-RPC error of the legacy revision travels in an HTTP 200
  // response.
  const auto unknown_tool = client_->send (legacy_post (
    http_test::rpc_request (
      3, "tools/call",
      nlohmann::json{
        { "name", "no-such-tool" }
  }),
    session_id));
  EXPECT_EQ (unknown_tool.status, 200);
  EXPECT_EQ (error_code (unknown_tool), -32602);
}

TEST_F (McpServerTest, LegacyDeleteEndsTheSession)
{
  start ();
  const auto session_id = start_legacy_session ();
  ASSERT_FALSE (session_id.isEmpty ());
  const auto deleted = client_->send (
    McpHttpClient::Request{ .method = "DELETE" }
      .session (session_id)
      .bearer (http_test::to_bytes (kToken)));
  EXPECT_EQ (deleted.status, 200);
  const auto after = client_->send (
    legacy_post (http_test::rpc_request (2, "ping"), session_id));
  EXPECT_EQ (after.status, 404);
  EXPECT_EQ (error_code (after), number_of (JsonRpcErrorCode::UnknownSession));
}

} // namespace zrythm::mcp
