// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/utf8_string.h"

#include <QtCore/QByteArray>
#include <QtCore/QFuture>
#include <QtCore/QMetaType>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QUuid>

#include <fmt/format.h>
#include <mcpserver/mcp_methods.h>
#include <mcpserver/protocol_engine.h>
#include <mcpserver/resource_provider.h>
#include <mcpserver/streamable_http_transport.h>

namespace zrythm::mcp
{

namespace
{

/** Calls the lambda of a std::visit that matches the type of the value. */
template <typename... Visitors> struct Overloaded : Visitors...
{
  using Visitors::operator()...;
};
template <typename... Visitors>
Overloaded (Visitors...) -> Overloaded<Visitors...>;

/** The HTTP status of the response to a JSON-RPC error. */
[[nodiscard]] auto
http_status_for_error (
  const ProtocolRevision revision,
  const JsonRpcErrorCode code) -> HttpStatus
{
  switch (code)
    {
    case JsonRpcErrorCode::ParseError:
    case JsonRpcErrorCode::InvalidRequest:
    case JsonRpcErrorCode::HeaderMismatch:
    case JsonRpcErrorCode::UnsupportedProtocolVersion:
      return HttpStatus::BadRequest;
    case JsonRpcErrorCode::InvalidParams:
      return revision == ProtocolRevision::V2026_07_28
               ? HttpStatus::BadRequest
               : HttpStatus::Ok;
    case JsonRpcErrorCode::MethodNotFound:
      return revision == ProtocolRevision::V2026_07_28
               ? HttpStatus::NotFound
               : HttpStatus::Ok;
    case JsonRpcErrorCode::InternalError:
      return revision == ProtocolRevision::V2026_07_28
               ? HttpStatus::InternalServerError
               : HttpStatus::Ok;
    case JsonRpcErrorCode::LimitReached:
      return HttpStatus::TooManyRequests;
    case JsonRpcErrorCode::ResourceNotFound:
      return HttpStatus::Ok;
    case JsonRpcErrorCode::UnknownSession:
      return HttpStatus::NotFound;
    }
  return HttpStatus::Ok;
}

/** Returns true when @p value holds only the characters that HTTP allows in a
 * header value. */
[[nodiscard]] auto
is_valid_header_value (const QByteArrayView value) -> bool
{
  return std::ranges::all_of (value, [] (const char character) {
    const auto byte = static_cast<unsigned char> (character);
    return (byte >= 0x21 && byte <= 0x7e) || byte == 0x20 || byte == 0x09;
  });
}

/** Returns the value of a header as it stands. Returns nullopt when the value
 * has characters that HTTP does not allow. */
[[nodiscard]] auto
plain_header_value (const QByteArray &value) -> std::optional<std::string>
{
  if (!is_valid_header_value (value))
    return std::nullopt;
  return utils::to_std_string (value);
}

/** Returns the value of a header that may use the base64 sentinel format.
 * Returns nullopt when the value has characters that HTTP does not allow or
 * when the base64 text is malformed. */
[[nodiscard]] auto
decode_header_value (const QByteArray &value) -> std::optional<std::string>
{
  static constexpr auto prefix = std::string_view ("=?base64?");
  static constexpr auto suffix = std::string_view ("?=");
  const auto            plain = plain_header_value (value);
  if (!plain.has_value ())
    return std::nullopt;
  const auto as_view = QByteArrayView (value);
  if (
    as_view.startsWith (QByteArrayView (prefix))
    && as_view.endsWith (QByteArrayView (suffix))
    && value.size () > static_cast<qsizetype> (prefix.size () + suffix.size ()))
    {
      const auto encoded = value.sliced (
        static_cast<qsizetype> (prefix.size ()),
        value.size () - static_cast<qsizetype> (prefix.size () + suffix.size ()));
      const auto decoded = QByteArray::fromBase64Encoding (encoded);
      if (decoded.decodingStatus != QByteArray::Base64DecodingStatus::Ok)
        return std::nullopt;
      return utils::to_std_string (decoded.decoded);
    }
  return plain;
}

/** Returns the string member @p key of the object @p object. */
[[nodiscard]] auto
member_string (const Json &object, const std::string_view key)
  -> std::optional<std::string>
{
  const auto it = object.find (key);
  if (it == object.end () || !it->is_string ())
    return std::nullopt;
  return it->get<std::string> ();
}

/** How a request is answered with the result of the handler that ran for it.
 */
template <typename T> struct AsyncResult;

template <> struct AsyncResult<CallToolResult>
{
  /** The kind of the request, for the error messages. */
  static constexpr std::string_view what = "tool call";
  /** The function that returned the result, for the error messages. */
  static constexpr std::string_view source = "tool handler";
  /** True when the revision marks the result as cacheable. */
  static constexpr bool cacheable = false;

  /** The result as the JSON result of the response. */
  [[nodiscard]] static auto to_json (const CallToolResult &result) -> Json
  {
    return result.to_json ();
  }
};

template <> struct AsyncResult<ResourceContents>
{
  static constexpr std::string_view what = "resource read";
  static constexpr std::string_view source = "resource provider";
  static constexpr bool             cacheable = true;

  [[nodiscard]] static auto to_json (const ResourceContents &contents) -> Json
  {
    return ResourcesRead::Result{ contents }.to_json ();
  }
};

/**
 * @brief Calls @p call and returns what it returned.
 *
 * The error holds the message of the exception that @p call threw. @p source
 * names the function that @p call stands for.
 */
template <typename Call>
[[nodiscard]] auto
call_guarded (const std::string_view source, Call &&call)
  -> std::expected<std::invoke_result_t<Call &>, std::string>
{
  try
    {
      return call ();
    }
  catch (const std::exception &exception)
    {
      return std::unexpected (
        fmt::format ("The {} threw an exception: {}", source, exception.what ()));
    }
  catch (...)
    {
      return std::unexpected (fmt::format ("The {} threw an exception", source));
    }
}

} // namespace

/** The implementation of the engine. It receives the requests of the
 * transport. */
class ProtocolEngine::Impl final : public McpRequestListener
{
public:
  /** A request whose tool call or resource read is still running. */
  struct PendingRequest
  {
    std::shared_ptr<HttpExchange> exchange;
    RequestId                     id;
    ProtocolRevision              revision;
  };

  /** A response stream that stays open. */
  struct Stream
  {
    std::shared_ptr<HttpExchange> exchange;
    /** The request that opened a listen stream. */
    std::optional<RequestId> listen_id;
    /** The notifications that the client of a listen stream asked for. */
    std::optional<SubscriptionFilter> filter;
    /** The session whose GET request opened the stream. */
    std::optional<std::string> session_id;
  };

  /** A session of the session-based revision. */
  struct LegacySession
  {
    std::string           id;
    std::set<ResourceUri> subscriptions;
    /** The ids of the open streams of the session. */
    std::vector<ExchangeId>               streams;
    std::chrono::steady_clock::time_point last_activity;
  };

  explicit Impl (ProtocolEngine &engine, ServerOptions options)
      : options_ (std::move (options)), owner_ (engine)
  {
    qRegisterMetaType<ResourceUri> ();
    expiry_timer_ = utils::make_qobject_unique<QTimer> (&owner_);
    const auto timeout = options_.limits.legacy_session_idle_timeout;
    expiry_timer_->setInterval (
      std::clamp (
        timeout / 8, std::chrono::milliseconds{ 1000 },
        std::chrono::milliseconds{ 60000 }));
    QObject::connect (expiry_timer_.get (), &QTimer::timeout, &owner_, [this] {
      expire_idle_sessions ();
    });
  }

  ~Impl () override { stop (); }
  Impl (const Impl &) = delete;
  Impl &operator= (const Impl &) = delete;

  [[nodiscard]] auto options () const -> const ServerOptions &
  {
    return options_;
  }

  auto set_authenticator (ProtocolEngine::Authenticator authenticator) -> void
  {
    z_return_if_fail (!is_listening ());
    authenticator_ = std::move (authenticator);
  }

  [[nodiscard]] auto tools () const -> std::span<const Tool> { return tools_; }

  [[nodiscard]] auto add_tool (Tool tool) -> std::expected<void, std::string>
  {
    z_return_val_if_fail (
      !is_listening (),
      std::unexpected ("Tools must be added before the server listens"));
    if (std::ranges::contains (tools_, tool.name (), &Tool::name))
      {
        return std::unexpected (
          "A tool named '" + tool.name () + "' exists already");
      }
    tools_.push_back (std::move (tool));
    return {};
  }

  auto set_call_tool_handler (ProtocolEngine::CallToolHandler handler) -> void
  {
    z_return_if_fail (!is_listening ());
    call_tool_handler_ = std::move (handler);
  }

  auto add_resource_provider (ResourceProvider &provider) -> void
  {
    z_return_if_fail (!is_listening ());
    QObject::connect (
      &provider, &ResourceProvider::resource_updated, &owner_,
      [this] (const ResourceUri &uri) { on_resource_updated (uri); });
    QObject::connect (
      &provider, &ResourceProvider::resources_changed, &owner_,
      [this] { on_resources_changed (); });
    providers_.emplace_back (&provider);
  }

  [[nodiscard]] auto listen (const QHostAddress &address, const quint16 port)
    -> std::expected<void, std::string>
  {
    if (transport_ != nullptr)
      return std::unexpected ("The server already listens");
    if (!authenticator_)
      {
        return std::unexpected (
          "The server needs an authenticator before it listens");
      }
    if (!tools_.empty () && !call_tool_handler_)
      {
        return std::unexpected (
          "The server has tools, so it needs a tool handler before it listens");
      }
    // Every stream holds a connection, so a smaller cap would refuse streams
    // that the limits below allow.
    const auto streams_allowed =
      options_.limits.max_listen_streams
      + options_.limits.max_legacy_sessions
          * options_.limits.max_streams_per_legacy_session;
    if (options_.limits.max_connections < streams_allowed)
      {
        return std::unexpected (
          "max_connections covers fewer connections than the limits on "
          "streams and sessions allow");
      }
    std::erase_if (providers_, [] (const auto &provider) {
      return provider.isNull ();
    });
    std::set<ResourceUri> uris;
    for (const auto &provider : providers_)
      {
        for (const auto &resource : provider->resources ())
          {
            if (!uris.insert (resource.uri).second)
              {
                return std::unexpected (
                  "Two providers serve the resource '" + resource.uri.str ()
                  + "'");
              }
          }
      }
    transport_ = std::make_unique<StreamableHttpTransport> (
      utils::Utf8String::from_utf8_encoded_string (options_.endpoint_path)
        .to_qstring (),
      options_.limits, authenticator_, *this, &owner_);
    if (auto result = transport_->listen (address, port); !result.has_value ())
      {
        transport_.reset ();
        return std::unexpected (std::move (result).error ());
      }
    expiry_timer_->start ();
    return {};
  }

  /** Ends every listen stream, every other stream, and every session. */
  auto stop () -> void
  {
    expiry_timer_->stop ();
    if (transport_ == nullptr)
      return;
    for (const auto &[_, stream] : streams_)
      {
        if (stream->listen_id.has_value ())
          {
            send_on_stream (
              *stream->exchange,
              result_response (
                *stream->listen_id,
                subscriptions_listen_closed_result (*stream->listen_id)));
          }
        stream->exchange->close_stream ();
      }
    streams_.clear ();
    drop_pending ();
    sessions_.clear ();
    transport_->stop ();
    transport_.reset ();
  }

  [[nodiscard]] auto is_listening () const -> bool
  {
    return transport_ != nullptr && transport_->is_listening ();
  }

  [[nodiscard]] auto listening_port () const -> quint16
  {
    return transport_ != nullptr ? transport_->listening_port () : 0;
  }

private:
  auto on_post (
    const HttpRequest                        &request,
    std::expected<Json, JsonRpcErrorResponse> body,
    const ProtocolEngine::Identity           &identity,
    std::shared_ptr<HttpExchange>             exchange) -> void override
  {
    if (!body.has_value ())
      {
        send_error (
          *exchange, std::move (body).error (), ProtocolRevision::V2025_11_25);
        return;
      }
    auto message = parse_json_rpc_message (*body);
    if (!message.has_value ())
      {
        send_error (
          *exchange, std::move (message).error (),
          ProtocolRevision::V2025_11_25);
        return;
      }
    // A notification gets no response, so an error for it has no id.
    const auto * request_message = std::get_if<JsonRpcRequest> (&*message);
    const auto   id =
      request_message == nullptr
        ? std::nullopt
        : std::optional<RequestId> (request_message->id);
    const auto revision = resolve_revision (request, id, *exchange);
    if (!revision.has_value ())
      return;
    if (std::holds_alternative<ClientResponse> (*message))
      {
        // The stateless revision sends no request, so it takes no response.
        if (*revision == ProtocolRevision::V2026_07_28)
          {
            reject (
              *exchange, std::nullopt, *revision,
              JsonRpcErrorCode::InvalidRequest,
              "The revision takes no response from the client");
            return;
          }
        answer (*exchange, HttpReply::accepted ());
        return;
      }
    if (std::holds_alternative<JsonRpcNotification> (*message))
      {
        if (
          *revision == ProtocolRevision::V2025_11_25
          && require_session (request, std::nullopt, *exchange) == nullptr)
          return;
        answer (*exchange, HttpReply::accepted ());
        return;
      }
    const auto &rpc = std::get<JsonRpcRequest> (*message);
    switch (*revision)
      {
      case ProtocolRevision::V2025_11_25:
        handle_session_request (request, rpc, identity, std::move (exchange));
        break;
      case ProtocolRevision::V2026_07_28:
        handle_stateless_request (request, rpc, identity, std::move (exchange));
        break;
      }
  }

  auto
  on_get (const HttpRequest &request, std::shared_ptr<HttpExchange> exchange)
    -> void override
  {
    if (refuse_stateless_method (request, *exchange))
      return;
    auto * session = require_session (request, std::nullopt, *exchange);
    if (session == nullptr)
      return;
    touch (*session);
    remove_ended_streams (*session);
    if (
      session->streams.size () >= options_.limits.max_streams_per_legacy_session)
      {
        reject (
          *exchange, std::nullopt, ProtocolRevision::V2025_11_25,
          JsonRpcErrorCode::LimitReached,
          "The session holds the largest number of streams that the server "
          "allows");
        return;
      }
    if (!exchange->start_stream ())
      return;
    const auto exchange_id = exchange->id ();
    session->streams.push_back (exchange_id);
    streams_.emplace (
      exchange_id,
      std::make_unique<Stream> (Stream{
        std::move (exchange), std::nullopt, std::nullopt, session->id }));
  }

  auto
  on_delete (const HttpRequest &request, std::shared_ptr<HttpExchange> exchange)
    -> void override
  {
    if (refuse_stateless_method (request, *exchange))
      return;
    auto * session = require_session (request, std::nullopt, *exchange);
    if (session == nullptr)
      return;
    end_session (session->id);
    answer (*exchange, HttpReply::without_body (HttpStatus::Ok));
  }

  auto on_client_gone (const ExchangeId id) -> void override
  {
    pending_.erase (id);
    if (const auto it = streams_.find (id); it != streams_.end ())
      {
        forget_stream (it->second.get ());
        it->second->exchange->close_stream ();
        streams_.erase (it);
      }
  }

  // --- Requests ---

  /**
   * @brief Reads the revision of a request from its MCP-Protocol-Version
   * header.
   *
   * @p id is the id of the request when the message carries one. The exchange
   * is answered when the header names no supported revision.
   */
  [[nodiscard]] auto resolve_revision (
    const HttpRequest              &request,
    const std::optional<RequestId> &id,
    HttpExchange                   &exchange) -> std::optional<ProtocolRevision>
  {
    const auto header = request.header ("MCP-Protocol-Version");
    if (header.isEmpty ())
      return ProtocolRevision::V2025_11_25;
    if (
      const auto revision =
        parse_protocol_revision (utils::to_std_string (header));
      revision.has_value ())
      {
        return revision;
      }
    reply_unsupported_version (exchange, id, header);
    return std::nullopt;
  }

  /** Answers a request whose MCP-Protocol-Version header names no supported
   * revision. @p id is the id of the request when it is known. */
  auto reply_unsupported_version (
    HttpExchange                   &exchange,
    const std::optional<RequestId> &id,
    const QByteArray               &header) -> void
  {
    answer (
      exchange,
      HttpReply::json_rpc_error (
        HttpStatus::BadRequest,
        error_response (
          id, JsonRpcErrorCode::UnsupportedProtocolVersion,
          "Unsupported protocol version",
          Json::object (
            {
              { "supported", supported_versions ()         },
              { "requested", utils::to_std_string (header) }
    }))));
  }

  /** Answers a request of the session-based revision. */
  auto handle_session_request (
    const HttpRequest              &request,
    const JsonRpcRequest           &rpc,
    const ProtocolEngine::Identity &identity,
    std::shared_ptr<HttpExchange>   exchange) -> void
  {
    constexpr auto revision = ProtocolRevision::V2025_11_25;
    if (rpc.method == Initialize::method_name)
      {
        auto parsed = parse_request (revision, rpc.method, rpc.params);
        if (!parsed.has_value ())
          {
            send_dispatch_error (*exchange, rpc.id, revision, parsed.error ());
            return;
          }
        auto * session = open_session (rpc.id, *exchange);
        if (session == nullptr)
          return;
        const Initialize::Result result{
          revision, capabilities_for (revision), options_.server,
          options_.instructions
        };
        answer (
          *exchange,
          HttpReply::json (
            HttpStatus::Ok, result_response (rpc.id, result.to_json ()),
            {
              { "Mcp-Session-Id", QByteArray::fromStdString (session->id) }
        }));
        return;
      }
    auto * session = require_session (request, rpc.id, *exchange);
    if (session == nullptr)
      return;
    touch (*session);
    auto parsed = parse_request (revision, rpc.method, rpc.params);
    if (!parsed.has_value ())
      {
        send_dispatch_error (*exchange, rpc.id, revision, parsed.error ());
        return;
      }
    if (!capability_allows (*parsed))
      {
        reject (
          *exchange, rpc.id, revision, JsonRpcErrorCode::MethodNotFound,
          "The server does not offer " + rpc.method);
        return;
      }
    dispatch (
      *parsed, std::move (exchange), rpc.id, revision, session, identity);
  }

  /** Answers a request of the stateless revision. */
  auto handle_stateless_request (
    const HttpRequest              &request,
    const JsonRpcRequest           &rpc,
    const ProtocolEngine::Identity &identity,
    std::shared_ptr<HttpExchange>   exchange) -> void
  {
    constexpr auto revision = ProtocolRevision::V2026_07_28;
    const auto header_mismatch = [this, &exchange, &rpc] (std::string message) {
      reject (
        *exchange, rpc.id, revision, JsonRpcErrorCode::HeaderMismatch,
        "Header mismatch: " + message);
    };
    // Only Mcp-Name and Mcp-Param-* may use the base64 sentinel format.
    const auto method_header =
      plain_header_value (request.header ("Mcp-Method"));
    if (
      !method_header.has_value () || method_header->empty ()
      || *method_header != rpc.method)
      {
        header_mismatch (
          "the Mcp-Method header must equal the method of the request");
        return;
      }
    // The methods that name a tool or a resource need the Mcp-Name header.
    const bool needs_name =
      rpc.method == ToolsCall::method_name
      || rpc.method == ResourcesRead::method_name;
    if (needs_name)
      {
        const auto name_header =
          decode_header_value (request.header ("Mcp-Name"));
        if (!name_header.has_value () || name_header->empty ())
          {
            header_mismatch ("the request needs a Mcp-Name header");
            return;
          }
        const auto body_name =
          rpc.params.has_value () && rpc.params->is_object ()
            ? member_string (
                *rpc.params, rpc.method == ToolsCall::method_name ? "name" : "uri")
            : std::nullopt;
        if (body_name.has_value () && *body_name != *name_header)
          {
            header_mismatch (
              "the Mcp-Name header does not equal the name in the params");
            return;
          }
      }
    auto parsed = parse_request (revision, rpc.method, rpc.params);
    if (
      !parsed.has_value ()
      && parsed.error ().code == JsonRpcErrorCode::MethodNotFound)
      {
        send_dispatch_error (*exchange, rpc.id, revision, parsed.error ());
        return;
      }
    if (!rpc.params.has_value () || !rpc.params->is_object ())
      {
        reject (
          *exchange, rpc.id, revision, JsonRpcErrorCode::InvalidParams,
          "The request needs a params object with a _meta object");
        return;
      }
    const auto meta_it = rpc.params->find ("_meta");
    if (meta_it == rpc.params->end () || !meta_it->is_object ())
      {
        reject (
          *exchange, rpc.id, revision, JsonRpcErrorCode::InvalidParams,
          "params must have a _meta object");
        return;
      }
    auto meta = RequestMeta::from_json (*meta_it);
    if (!meta.has_value ())
      {
        reject (
          *exchange, rpc.id, revision, JsonRpcErrorCode::InvalidParams,
          std::move (meta).error ());
        return;
      }
    if (meta->protocol_version != to_string (revision))
      {
        header_mismatch (
          "the MCP-Protocol-Version header does not equal the protocol revision in params._meta");
        return;
      }
    if (!parsed.has_value ())
      {
        send_dispatch_error (*exchange, rpc.id, revision, parsed.error ());
        return;
      }
    if (!capability_allows (*parsed))
      {
        reject (
          *exchange, rpc.id, revision, JsonRpcErrorCode::MethodNotFound,
          "The server does not offer " + rpc.method);
        return;
      }
    dispatch (
      *parsed, std::move (exchange), rpc.id, revision, nullptr, identity);
  }

  /**
   * @brief Answers GET and DELETE for a client of the stateless revision.
   *
   * Returns true when @p exchange was answered. The stateless revision has
   * POST requests only.
   */
  [[nodiscard]] auto
  refuse_stateless_method (const HttpRequest &request, HttpExchange &exchange)
    -> bool
  {
    const auto header = request.header ("MCP-Protocol-Version");
    if (header.isEmpty ())
      return false;
    const auto revision =
      parse_protocol_revision (utils::to_std_string (header));
    if (!revision.has_value ())
      {
        reply_unsupported_version (exchange, std::nullopt, header);
        return true;
      }
    if (*revision == ProtocolRevision::V2026_07_28)
      {
        answer (exchange, HttpReply::method_not_allowed ({ HttpMethod::Post }));
        return true;
      }
    return false;
  }

  /** Answers a request with validated params. */
  auto dispatch (
    AnyRequest                      request,
    std::shared_ptr<HttpExchange>   exchange,
    const RequestId                &id,
    const ProtocolRevision          revision,
    LegacySession *                 session,
    const ProtocolEngine::Identity &identity) -> void
  {
    std::visit (
      Overloaded{
        [&] (const Initialize &) {
          internal_error (
            *exchange, id, revision,
            "An initialize request reached the dispatcher");
        },
        [&] (const Ping &) {
          complete (*exchange, id, revision, Json::object (), false);
        },
        [&] (const ToolsList &) {
          complete (*exchange, id, revision, tools_list_result (), true);
        },
        [&] (ToolsCall &call) {
          start_tool_call (
            std::move (call.params), identity, std::move (exchange), id,
            revision);
        },
        [&] (const ResourcesList &) {
          auto resources = resources_list_result ();
          if (!resources.has_value ())
            {
              internal_error (
                *exchange, id, revision, std::move (resources).error ());
              return;
            }
          complete (*exchange, id, revision, std::move (*resources), true);
        },
        [&] (const ResourcesTemplatesList &) {
          complete (
            *exchange, id, revision,
            ResourcesTemplatesList::Result{}.to_json (), true);
        },
        [&] (const ResourcesRead &read) {
          start_resource_read (
            read.params.uri, std::move (exchange), id, revision);
        },
        [&] (const ResourcesSubscribe &subscribe) {
          const auto &uri = subscribe.params.uri;
          auto        provider = provider_for (uri);
          if (!provider.has_value ())
            {
              internal_error (
                *exchange, id, revision, std::move (provider).error ());
              return;
            }
          if (*provider == nullptr)
            {
              reject (
                *exchange, id, revision,
                policy_for (revision).resource_not_found_code,
                "No provider serves the resource '" + uri.str () + "'");
              return;
            }
          if (session == nullptr)
            {
              internal_error (
                *exchange, id, revision,
                "A subscribe request reached the dispatcher without a session");
              return;
            }
          session->subscriptions.insert (uri);
          complete (*exchange, id, revision, Json::object (), false);
        },
        [&] (const ResourcesUnsubscribe &unsubscribe) {
          if (session == nullptr)
            {
              internal_error (
                *exchange, id, revision,
                "An unsubscribe request reached the dispatcher without a "
                "session");
              return;
            }
          session->subscriptions.erase (unsubscribe.params.uri);
          complete (*exchange, id, revision, Json::object (), false);
        },
        [&] (const SubscriptionsListen &listen) {
          start_listen_stream (
            listen.params.notifications, std::move (exchange), id);
        },
        [&] (const ServerDiscover &) {
          const ServerDiscover::Result result{
            capabilities_for (revision), options_.instructions
          };
          complete (*exchange, id, revision, result.to_json (), true);
        },
      },
      request);
  }

  // --- Results ---

  /** Ends the pending @p exchange with @p reply. */
  auto answer (HttpExchange &exchange, HttpReply reply) -> void
  {
    std::ignore = exchange.reply (std::move (reply));
  }

  /** Sends @p message on @p stream. A stream whose client closed its
   * connection receives nothing. */
  auto send_on_stream (HttpExchange &stream, const Json &message) -> void
  {
    std::ignore = stream.send (message);
  }

  /** Sends a JSON-RPC error response. The HTTP status follows @p revision. */
  auto send_error (
    HttpExchange               &exchange,
    const JsonRpcErrorResponse &error,
    const ProtocolRevision      revision) -> void
  {
    answer (
      exchange,
      HttpReply::json_rpc_error (
        http_status_for_error (revision, error.error.code), error));
  }

  /** Sends the error @p code with @p message for the request @p id. The HTTP
   * status follows @p revision. */
  auto reject (
    HttpExchange                   &exchange,
    const std::optional<RequestId> &id,
    const ProtocolRevision          revision,
    const JsonRpcErrorCode          code,
    std::string                     message) -> void
  {
    send_error (
      exchange, error_response (id, code, std::move (message)), revision);
  }

  /** Sends the error of a request whose method or params did not validate. */
  auto send_dispatch_error (
    HttpExchange          &exchange,
    const RequestId       &id,
    const ProtocolRevision revision,
    const DispatchError   &error) -> void
  {
    reject (exchange, id, revision, error.code, error.message);
  }

  /** Sends the result of a request, decorated as @p revision requires. */
  auto complete (
    HttpExchange          &exchange,
    const RequestId       &id,
    const ProtocolRevision revision,
    Json                   result,
    const bool             cacheable) -> void
  {
    decorate (result, revision, cacheable);
    answer (
      exchange, HttpReply::json (HttpStatus::Ok, result_response (id, result)));
  }

  auto
  decorate (Json &result, const ProtocolRevision revision, const bool cacheable)
    -> void
  {
    if (revision != ProtocolRevision::V2026_07_28)
      return;
    result["resultType"] = "complete";
    result["_meta"] = Json::object (
      {
        { kServerInfoMetaKey, options_.server.to_json () }
    });
    if (cacheable)
      {
        result["ttlMs"] = 0;
        result["cacheScope"] = "private";
      }
  }

  [[nodiscard]] auto
  capabilities_for (const ProtocolRevision revision) const -> ServerCapabilities
  {
    ServerCapabilities capabilities;
    if (!tools_.empty ())
      capabilities.tools = ToolsCapability{ false };
    if (has_resources ())
      {
        capabilities.resources = ResourcesCapability{
          revision == ProtocolRevision::V2025_11_25, true
        };
      }
    return capabilities;
  }

  /** Returns true when the server has the capability @p requirement. */
  [[nodiscard]] auto
  has_capability (const CapabilityRequirement requirement) const -> bool
  {
    switch (requirement)
      {
      case CapabilityRequirement::Nothing:
        return true;
      case CapabilityRequirement::Tools:
        return !tools_.empty ();
      case CapabilityRequirement::Resources:
        return has_resources ();
      }
    z_return_val_if_reached (false);
  }

  /** Returns true when the server has the capability that @p request needs. */
  [[nodiscard]] auto capability_allows (const AnyRequest &request) const -> bool
  {
    return std::visit (
      [this] (const auto &method) {
        return has_capability (std::decay_t<decltype (method)>::requirement);
      },
      request);
  }

  [[nodiscard]] auto tools_list_result () const -> Json
  {
    ToolsList::Result result;
    for (const auto &tool : tools_)
      result.tools.push_back (tool);
    return result.to_json ();
  }

  /** The resources of @p provider. The error holds the message of the
   * exception that the provider threw. */
  [[nodiscard]] auto resources_of (const ResourceProvider &provider) const
    -> std::expected<std::vector<Resource>, std::string>
  {
    return call_guarded (AsyncResult<ResourceContents>::source, [&] {
      return provider.resources ();
    });
  }

  /** The result of a resources/list request. The error holds the message of
   * the exception that a provider threw. */
  [[nodiscard]] auto
  resources_list_result () const -> std::expected<Json, std::string>
  {
    ResourcesList::Result result;
    for (const auto &provider : providers_)
      {
        if (provider.isNull ())
          continue;
        auto resources = resources_of (*provider);
        if (!resources.has_value ())
          return std::unexpected (std::move (resources).error ());
        for (auto &resource : *resources)
          result.resources.push_back (std::move (resource));
      }
    return result.to_json ();
  }

  [[nodiscard]] auto has_resources () const -> bool
  {
    return std::ranges::any_of (providers_, [] (const auto &provider) {
      return !provider.isNull ();
    });
  }

  /**
   * @brief Returns the provider that serves @p uri, or nullptr when no
   * provider serves it.
   *
   * The error holds the message of the exception that a provider threw.
   */
  [[nodiscard]] auto provider_for (const ResourceUri &uri) const
    -> std::expected<ResourceProvider *, std::string>
  {
    for (const auto &provider : providers_)
      {
        if (provider.isNull ())
          continue;
        auto resources = resources_of (*provider);
        if (!resources.has_value ())
          return std::unexpected (std::move (resources).error ());
        if (std::ranges::any_of (*resources, [&uri] (const Resource &resource) {
              return resource.uri == uri;
            }))
          return provider.data ();
      }
    return nullptr;
  }

  // --- Tool calls and resource reads ---

  auto start_tool_call (
    ToolCall                        call,
    const ProtocolEngine::Identity &identity,
    std::shared_ptr<HttpExchange>   exchange,
    const RequestId                &id,
    const ProtocolRevision          revision) -> void
  {
    const auto tool = std::ranges::find (tools_, call.name, &Tool::name);
    if (tool == tools_.end ())
      {
        reject (
          *exchange, id, revision, JsonRpcErrorCode::InvalidParams,
          "Unknown tool: " + call.name);
        return;
      }
    if (call_tool_handler_ == nullptr)
      {
        internal_error (
          *exchange, id, revision, "The server has tools and no tool handler");
        return;
      }
    auto future = call_guarded (AsyncResult<CallToolResult>::source, [&] {
      return call_tool_handler_ (call, revision, identity);
    });
    if (!future.has_value ())
      {
        internal_error (*exchange, id, revision, std::move (future).error ());
        return;
      }
    answer_with (std::move (*future), std::move (exchange), id, revision);
  }

  auto start_resource_read (
    const ResourceUri            &uri,
    std::shared_ptr<HttpExchange> exchange,
    const RequestId              &id,
    const ProtocolRevision        revision) -> void
  {
    auto provider = provider_for (uri);
    if (!provider.has_value ())
      {
        internal_error (*exchange, id, revision, std::move (provider).error ());
        return;
      }
    if (*provider == nullptr)
      {
        reject (
          *exchange, id, revision, policy_for (revision).resource_not_found_code,
          "No provider serves the resource '" + uri.str () + "'");
        return;
      }
    auto future = call_guarded (AsyncResult<ResourceContents>::source, [&] {
      return (*provider)->read (uri);
    });
    if (!future.has_value ())
      {
        internal_error (*exchange, id, revision, std::move (future).error ());
        return;
      }
    answer_with (std::move (*future), std::move (exchange), id, revision);
  }

  /**
   * @brief Answers @p exchange with the result of @p future.
   *
   * @p future holds the result of the tool call or of the resource read that
   * runs for the request @p id. An unfinished future is answered when it
   * finishes.
   */
  template <typename T>
  auto answer_with (
    QFuture<T>                    future,
    std::shared_ptr<HttpExchange> exchange,
    const RequestId              &id,
    const ProtocolRevision        revision) -> void
  {
    if (!future.isValid ())
      {
        internal_error (
          *exchange, id, revision,
          fmt::format ("The {} returned no result", AsyncResult<T>::source));
        return;
      }
    if (future.isFinished ())
      {
        finish (future, *exchange, id, revision);
        return;
      }
    const auto exchange_id = exchange->id ();
    pending_.emplace (
      exchange_id, PendingRequest{ std::move (exchange), id, revision });
    future.then (&owner_, [this, exchange_id] (const QFuture<T> &result) {
      finish (result, exchange_id);
    });
  }

  /** Answers the pending request @p exchange_id with the result of the
   * finished future @p future. */
  template <typename T>
  auto finish (const QFuture<T> &future, const ExchangeId exchange_id) -> void
  {
    const auto it = pending_.find (exchange_id);
    if (it == pending_.end ())
      return;
    const auto pending = std::move (it->second);
    pending_.erase (it);
    finish (future, *pending.exchange, pending.id, pending.revision);
  }

  /** Answers @p exchange with the result of the finished future @p future. */
  template <typename T>
  auto finish (
    const QFuture<T>      &future,
    HttpExchange          &exchange,
    const RequestId       &id,
    const ProtocolRevision revision) -> void
  {
    using Result = AsyncResult<T>;
    if (future.isCanceled ())
      {
        internal_error (
          exchange, id, revision,
          fmt::format ("The {} was cancelled", Result::what));
        return;
      }
    try
      {
        complete (
          exchange, id, revision, Result::to_json (future.result ()),
          Result::cacheable);
      }
    catch (const std::exception &exception)
      {
        internal_error (exchange, id, revision, exception.what ());
      }
    catch (...)
      {
        internal_error (
          exchange, id, revision,
          fmt::format ("The {} raised an exception", Result::what));
      }
  }

  auto internal_error (
    HttpExchange          &exchange,
    const RequestId       &id,
    const ProtocolRevision revision,
    std::string            message) -> void
  {
    z_warning ("MCP request failed: {}", message);
    reject (
      exchange, id, revision, JsonRpcErrorCode::InternalError,
      std::move (message));
  }

  auto drop_pending () -> void { pending_.clear (); }

  // --- Streams ---

  auto start_listen_stream (
    SubscriptionFilter            requested,
    std::shared_ptr<HttpExchange> exchange,
    const RequestId              &id) -> void
  {
    const auto listen_streams =
      std::ranges::count_if (streams_, [] (const auto &entry) {
        return entry.second->listen_id.has_value ();
      });
    if (
      static_cast<std::size_t> (listen_streams)
      >= options_.limits.max_listen_streams)
      {
        reject (
          *exchange, id, ProtocolRevision::V2026_07_28,
          JsonRpcErrorCode::LimitReached,
          "The server holds the largest number of listen streams that the "
          "server allows");
        return;
      }
    // The server sends no tools/list_changed and no prompts/list_changed.
    SubscriptionFilter honored = requested;
    honored.tools_list_changed.reset ();
    honored.prompts_list_changed.reset ();
    const auto exchange_id = exchange->id ();
    if (!exchange->start_stream ())
      return;
    streams_.emplace (
      exchange_id,
      std::make_unique<Stream> (Stream{
        std::move (exchange), id, std::move (honored), std::nullopt }));
    const auto &stream = *streams_.at (exchange_id);
    send_on_stream (
      *stream.exchange,
      subscriptions_acknowledged_notification (id, *stream.filter));
  }

  /** Handles resource_updated of a provider. */
  auto on_resource_updated (const ResourceUri &uri) -> void
  {
    // Every message of a session goes over the first open stream of the session.
    for (const auto &[_, session] : sessions_)
      {
        if (!session->subscriptions.contains (uri))
          continue;
        if (auto * exchange = first_open_stream (*session); exchange != nullptr)
          {
            send_on_stream (
              *exchange, resources_updated_notification (uri, std::nullopt));
          }
      }
    for (const auto &[_, stream] : streams_)
      {
        if (
          !stream->listen_id.has_value ()
          || !stream->filter->resource_subscriptions.has_value ())
          continue;
        const auto &subscriptions = *stream->filter->resource_subscriptions;
        if (std::ranges::contains (subscriptions, uri))
          {
            send_on_stream (
              *stream->exchange,
              resources_updated_notification (uri, *stream->listen_id));
          }
      }
  }

  /** Handles resources_changed of a provider. */
  auto on_resources_changed () -> void
  {
    for (const auto &[_, session] : sessions_)
      {
        if (auto * exchange = first_open_stream (*session); exchange != nullptr)
          {
            send_on_stream (
              *exchange, resources_list_changed_notification (std::nullopt));
          }
      }
    for (const auto &[_, stream] : streams_)
      {
        if (
          stream->listen_id.has_value ()
          && stream->filter->resources_list_changed.value_or (false))
          {
            send_on_stream (
              *stream->exchange,
              resources_list_changed_notification (stream->listen_id));
          }
      }
  }

  /**
   * @brief Returns the first open stream of @p session whose client is still
   * connected, or nullptr.
   *
   * A stream whose client closed its connection receives nothing anymore.
   * The liveness check of the transport removes it.
   */
  [[nodiscard]] auto
  first_open_stream (LegacySession &session) -> HttpExchange *
  {
    remove_ended_streams (session);
    for (const auto id : session.streams)
      {
        auto * exchange = streams_.at (id)->exchange.get ();
        if (!exchange->client_gone ())
          return exchange;
      }
    return nullptr;
  }

  /** Removes the ids of ended streams from @p session. */
  auto remove_ended_streams (LegacySession &session) -> void
  {
    std::erase_if (session.streams, [this] (const ExchangeId id) {
      return !streams_.contains (id);
    });
  }

  /** Removes the stream @p stream from the bookkeeping of its session. */
  auto forget_stream (const Stream * stream) -> void
  {
    if (!stream->session_id.has_value ())
      return;
    const auto session = sessions_.find (*stream->session_id);
    if (session != sessions_.end ())
      std::erase (session->second->streams, stream->exchange->id ());
  }

  // --- Sessions ---

  [[nodiscard]] auto new_session_id () -> std::string
  {
    return std::string (
      utils::Utf8String::from_qstring (
        QUuid::createUuid ().toString (QUuid::WithoutBraces))
        .view ());
  }

  /** Starts a session. The exchange is answered when the server is full. */
  [[nodiscard]] auto
  open_session (const RequestId &id, HttpExchange &exchange) -> LegacySession *
  {
    if (sessions_.size () >= options_.limits.max_legacy_sessions)
      {
        reject (
          exchange, id, ProtocolRevision::V2025_11_25,
          JsonRpcErrorCode::LimitReached,
          "The server holds the largest number of sessions that the server "
          "allows");
        return nullptr;
      }
    auto session = std::make_unique<LegacySession> ();
    session->id = new_session_id ();
    session->last_activity = std::chrono::steady_clock::now ();
    auto * inserted = session.get ();
    sessions_.emplace (session->id, std::move (session));
    return inserted;
  }

  /**
   * @brief Returns the session of the request.
   *
   * The exchange is answered when the request has no session header or the
   * session is unknown. @p id is the id of the request when it is known.
   */
  [[nodiscard]] auto require_session (
    const HttpRequest              &request,
    const std::optional<RequestId> &id,
    HttpExchange                   &exchange) -> LegacySession *
  {
    const auto header = request.header ("Mcp-Session-Id");
    if (header.isEmpty ())
      {
        answer_without_session (exchange, id, HttpStatus::BadRequest);
        return nullptr;
      }
    const auto session = sessions_.find (utils::to_std_string (header));
    if (session == sessions_.end ())
      {
        answer_without_session (exchange, id, HttpStatus::NotFound);
        return nullptr;
      }
    return session->second.get ();
  }

  auto answer_without_session (
    HttpExchange                   &exchange,
    const std::optional<RequestId> &id,
    const HttpStatus                status) -> void
  {
    const auto message =
      status == HttpStatus::BadRequest
        ? "The request has no Mcp-Session-Id header"
        : "The session is unknown or has ended";
    if (!id.has_value ())
      {
        answer (exchange, HttpReply::without_body (status));
        return;
      }
    answer (
      exchange,
      HttpReply::json_rpc_error (
        status,
        error_response (
          *id,
          status == HttpStatus::BadRequest
            ? JsonRpcErrorCode::InvalidRequest
            : JsonRpcErrorCode::UnknownSession,
          message)));
  }

  auto touch (LegacySession &session) const -> void
  {
    session.last_activity = std::chrono::steady_clock::now ();
  }

  /** Ends the streams and the state of the session @p session_id. */
  auto end_session (const std::string &session_id) -> void
  {
    const auto session = sessions_.find (session_id);
    if (session == sessions_.end ())
      return;
    for (const auto stream_id : session->second->streams)
      {
        if (
          const auto stream = streams_.find (stream_id);
          stream != streams_.end ())
          {
            stream->second->exchange->close_stream ();
            streams_.erase (stream);
          }
      }
    sessions_.erase (session);
  }

  /** Ends the sessions that had no request and no open stream for the idle
   * timeout. */
  auto expire_idle_sessions () -> void
  {
    const auto now = std::chrono::steady_clock::now ();
    std::erase_if (sessions_, [this, now] (auto &entry) {
      auto &session = *entry.second;
      remove_ended_streams (session);
      return session.streams.empty ()
             && now - session.last_activity
                  >= options_.limits.legacy_session_idle_timeout;
    });
  }

  ServerOptions                                           options_;
  ProtocolEngine::Authenticator                           authenticator_;
  ProtocolEngine::CallToolHandler                         call_tool_handler_;
  std::vector<Tool>                                       tools_;
  std::vector<QPointer<ResourceProvider>>                 providers_;
  ProtocolEngine                                         &owner_;
  utils::QObjectUniquePtr<QTimer>                         expiry_timer_;
  std::unique_ptr<StreamableHttpTransport>                transport_;
  std::unordered_map<ExchangeId, PendingRequest>          pending_;
  std::unordered_map<ExchangeId, std::unique_ptr<Stream>> streams_;
  std::unordered_map<std::string, std::unique_ptr<LegacySession>> sessions_;
};

// --- ProtocolEngine ---

ProtocolEngine::ProtocolEngine (ServerOptions options, QObject * parent)
    : QObject (parent),
      impl_ (std::make_unique<Impl> (*this, std::move (options)))
{
}

ProtocolEngine::~ProtocolEngine () = default;

auto
ProtocolEngine::options () const -> const ServerOptions &
{
  return impl_->options ();
}

auto
ProtocolEngine::tools () const -> std::span<const Tool>
{
  return impl_->tools ();
}

auto
ProtocolEngine::set_authenticator (Authenticator authenticator) -> void
{
  impl_->set_authenticator (std::move (authenticator));
}

auto
ProtocolEngine::add_tool (Tool tool) -> std::expected<void, std::string>
{
  return impl_->add_tool (std::move (tool));
}

auto
ProtocolEngine::set_call_tool_handler (CallToolHandler handler) -> void
{
  impl_->set_call_tool_handler (std::move (handler));
}

auto
ProtocolEngine::add_resource_provider (ResourceProvider &provider) -> void
{
  impl_->add_resource_provider (provider);
}

auto
ProtocolEngine::listen (const QHostAddress &address, const quint16 port)
  -> std::expected<void, std::string>
{
  return impl_->listen (address, port);
}

auto
ProtocolEngine::stop () -> void
{
  impl_->stop ();
}

auto
ProtocolEngine::is_listening () const -> bool
{
  return impl_->is_listening ();
}

auto
ProtocolEngine::listening_port () const -> quint16
{
  return impl_->listening_port ();
}

} // namespace zrythm::mcp
