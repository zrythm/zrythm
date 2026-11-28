// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

#include "frontends/mcp/mcp_worker.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"
#include "utils/version.h"

#include <QAbstractSocket>
#include <QCryptographicHash>
#include <QFuture>
#include <QHostAddress>

#include "operations/operation.h"
#include <fmt/format.h>
#include <mcpserver/resource_provider.h>

namespace zrythm::frontends::mcp
{

namespace
{

constexpr std::string_view kJournalResourceUri = "zrythm://journal";

/** Entries kept in the journal resource, oldest dropped first. */
constexpr std::size_t kJournalTailLength = 256;

/**
 * Largest whole number that a double holds exactly.
 *
 * A JSON client may send an integer as a floating point number. A larger
 * one stands for several integers, so it cannot name a revision.
 */
constexpr double kMaxExactFloatInteger = 9007199254740992.0;

/** A future that is finished with @p result. */
QFuture<zrythm::mcp::CallToolResult>
finished_with (zrythm::mcp::CallToolResult result)
{
  return QtFuture::makeReadyValueFuture (std::move (result));
}

/** The URI of the journal resource. */
zrythm::mcp::ResourceUri
journal_uri ()
{
  // The constant is not empty, so parse() returns a value.
  return *zrythm::mcp::ResourceUri::parse (kJournalResourceUri);
}

/**
 * Reads the value of the expected-revision argument.
 *
 * The value must be a non-negative integer. A floating point number is
 * accepted when it is a whole number that a double holds exactly.
 */
std::expected<std::uint64_t, std::string>
parse_expected_revision (const nlohmann::json &value)
{
  if (value.is_number_unsigned ())
    {
      return value.get<std::uint64_t> ();
    }
  if (value.is_number_integer ())
    {
      const auto number = value.get<std::int64_t> ();
      if (number >= 0)
        return static_cast<std::uint64_t> (number);
    }
  else if (value.is_number_float ())
    {
      const auto number = value.get<double> ();
      if (
        number >= 0 && number == std::floor (number)
        && number <= kMaxExactFloatInteger)
        {
          return static_cast<std::uint64_t> (number);
        }
    }
  return std::unexpected (
    std::string ("'expected_revision' must be a non-negative integer"));
}

/** Maps a failed listen() of the server to the reason the endpoint reports. */
McpStartError
start_error_reason (const zrythm::mcp::ListenError &error)
{
  if (!error.socket_error ().has_value ())
    return McpStartError::Other;
  return *error.socket_error () == QAbstractSocket::AddressInUseError
           ? McpStartError::PortInUse
           : McpStartError::PortRefused;
}

} // namespace

/**
 * The undo history journal as one MCP resource.
 *
 * The text of the resource is a JSON object with one member "entries":
 * the last entries, oldest first.
 */
class McpWorkerController::JournalResource final
    : public zrythm::mcp::ResourceProvider
{
public:
  JournalResource () = default;

  auto resources () const -> std::vector<zrythm::mcp::Resource> override
  {
    return {
      zrythm::mcp::Resource{
                            journal_uri (), "journal", "Undo history journal, newest entry last",
                            "application/json", std::nullopt }
    };
  }

  auto read (const zrythm::mcp::ResourceUri &uri)
    -> QFuture<zrythm::mcp::ResourceContents> override
  {
    // The server reads only the URIs that resources() lists.
    z_return_val_if_fail (
      uri == journal_uri (), QFuture<zrythm::mcp::ResourceContents>{});
    return QtFuture::makeReadyValueFuture (
      zrythm::mcp::ResourceContents{
        zrythm::mcp::TextResourceContents{
                                          journal_uri (), text (), "application/json" }
    });
  }

  /** Appends @p entry, drops the entries past the tail length, and reports the
   * change. */
  auto push (nlohmann::json entry) -> void
  {
    entries_.push_back (std::move (entry));
    if (entries_.size () > kJournalTailLength)
      {
        entries_.erase (
          entries_.begin (),
          std::ranges::prev (entries_.end (), kJournalTailLength));
      }
    Q_EMIT resource_updated (journal_uri ());
  }

private:
  auto text () const -> std::string
  {
    return nlohmann::json{
      { "entries", entries_ }
    }.dump ();
  }

  std::vector<nlohmann::json> entries_;
};

McpWorkerController::McpWorkerController (McpDispatchFn dispatch_fn)
    : dispatch_fn_ (std::move (dispatch_fn))
{
  // server_error crosses a thread boundary, so its reason must be a metatype.
  qRegisterMetaType<McpStartError> ();
}

McpWorkerController::~McpWorkerController () = default;

void
McpWorkerController::start_server (Config config)
{
  config_ = std::move (config);
  // The journal keeps the entries that arrived while no server existed.
  if (journal_ == nullptr)
    {
      journal_ = std::make_unique<JournalResource> ();
    }

  if (config_.token_hashes.isEmpty ())
    {
      const auto message = QStringLiteral (
        "refusing to start without a token: the endpoint would accept "
        "unauthenticated callers");
      z_warning ("{}", utils::Utf8String::from_qstring (message).view ());
      Q_EMIT server_error (message, McpStartError::NoToken);
      return;
    }

  zrythm::mcp::ServerOptions options;
  options.server = {
    "zrythm", std::string (utils::get_app_version_string (false).view ())
  };
  options.instructions =
    "Zrythm operations. Tools map one-to-one to operations. Every tool "
    "call returns a JSON report with the project revision before and "
    "after. Edit tools require an 'expected_revision' argument: when it "
    "is missing or does not match the current revision the edit is "
    "refused and the report carries the current revision; re-read the "
    "state and send the edit again with that value. Journal entries are "
    "published as updates of the zrythm://journal resource.";
  auto server = std::make_unique<Server> (std::move (options));

  server->set_authenticator (
    [this] (const zrythm::mcp::HttpRequest &request)
      -> std::expected<TokenIdentity, std::string> {
      const auto header = request.header ("Authorization").trimmed ();
      const auto prefix = QByteArrayView ("Bearer ");
      if (
        header.size () < prefix.size ()
        || header.left (prefix.size ()).compare (prefix, Qt::CaseInsensitive) != 0)
        {
          return std::unexpected (
            std::string ("the request carries no bearer token"));
        }
      const auto token = header.mid (prefix.size ()).trimmed ();
      const auto digest = QString::fromLatin1 (
        QCryptographicHash::hash (token, QCryptographicHash::Sha256).toHex ());
      const auto grant = config_.token_hashes.constFind (digest);
      if (grant == config_.token_hashes.constEnd ())
        {
          return std::unexpected (
            std::string ("the bearer token is not accepted"));
        }
      return TokenIdentity{ grant.value () };
    });

  server->set_call_tool_handler (
    [this] (
      const zrythm::mcp::ToolCall &call, const Server::RequestContext &context)
      -> QFuture<zrythm::mcp::CallToolResult> {
      const auto tool = config_.catalog.by_name.find (call.name);
      // The catalog holds an entry for every tool that it publishes.
      z_return_val_if_fail (
        tool != config_.catalog.by_name.end (),
        finished_with (
          zrythm::mcp::CallToolResult::error ("the tool has no operation")));
      if (!grant_allows (context.identity.grant, tool->second.kind))
        {
          return finished_with (
            zrythm::mcp::CallToolResult::error (
              fmt::format (
                "the '{}' grant does not allow '{}'",
                utils::Utf8String::from_qstring (
                  grant_identity (context.identity.grant))
                  .view (),
                call.name)));
        }

      auto arguments = call.arguments.value_or (nlohmann::json::object ());
      McpDispatchRequest dispatch;
      dispatch.operation = tool->second.id;
      if (
        const auto expected =
          arguments.find (operations::kExpectedRevisionArgument);
        expected != arguments.end ())
        {
          const auto revision = parse_expected_revision (*expected);
          if (!revision.has_value ())
            {
              return finished_with (
                zrythm::mcp::CallToolResult::error (revision.error ()));
            }
          dispatch.expected_revision = *revision;
          arguments.erase (expected);
        }
      dispatch.arguments = std::move (arguments);
      const auto completion =
        std::make_shared<QPromise<zrythm::mcp::CallToolResult>> ();
      completion->start ();
      dispatch.completion = completion;
      dispatch_fn_ (dispatch);
      return completion->future ();
    });

  for (const auto &tool : config_.catalog.tools)
    {
      const auto added = server->add_tool (tool);
      if (!added.has_value ())
        {
          const auto message =
            utils::Utf8String::from_utf8_encoded_string (
              fmt::format ("the tool catalog is not valid: {}", added.error ()))
              .to_qstring ();
          z_warning ("{}", utils::Utf8String::from_qstring (message).view ());
          Q_EMIT server_error (message, McpStartError::Other);
          return;
        }
    }
  server->add_resource_provider (*journal_);

  const auto listening =
    server->listen (QHostAddress (QHostAddress::LocalHost), config_.port);
  if (!listening.has_value ())
    {
      const auto &error = listening.error ();
      const auto  message =
        utils::Utf8String::from_utf8_encoded_string (error.message ())
          .to_qstring ();
      z_warning (
        "MCP endpoint error: {}",
        utils::Utf8String::from_qstring (message).view ());
      Q_EMIT server_error (message, start_error_reason (error));
      return;
    }
  server_ = std::move (server);
  Q_EMIT server_started ();
}

void
McpWorkerController::fulfill_call (
  std::shared_ptr<QPromise<zrythm::mcp::CallToolResult>> completion,
  zrythm::mcp::CallToolResult                            result)
{
  completion->addResult (std::move (result));
  completion->finish ();
}

void
McpWorkerController::push_journal_entry (nlohmann::json entry)
{
  // Entries can queue before the server exists; they stay in the
  // resource and are served once a client reads it.
  z_return_if_fail (journal_ != nullptr);
  journal_->push (std::move (entry));
}

} // namespace zrythm::frontends::mcp
