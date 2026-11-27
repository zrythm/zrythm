// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cassert>
#include <ranges>

#include "frontends/mcp/mcp_service.h"
#include "frontends/mcp/mcp_tool_catalog.h"
#include "undo/journal.h"
#include "undo/undo_stack.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QMetaObject>
#include <QProcessEnvironment>
#include <QRandomGenerator>
#include <QSet>
#include <QThread>

#include "operations/operation_dispatcher.h"
#include "operations/operation_registry.h"
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace zrythm::frontends::mcp
{

namespace
{

std::string_view
status_name (operations::OperationStatus status)
{
  switch (status)
    {
    case operations::OperationStatus::Applied:
      return "applied";
    case operations::OperationStatus::Refused:
      return "refused";
    case operations::OperationStatus::Error:
      return "error";
    }
  return "error";
}

std::string_view
journal_kind_name (undo::JournalEntryKind kind)
{
  switch (kind)
    {
    case undo::JournalEntryKind::Push:
      return "push";
    case undo::JournalEntryKind::Undo:
      return "undo";
    case undo::JournalEntryKind::Redo:
      return "redo";
    case undo::JournalEntryKind::Abort:
      return "abort";
    }
  return "push";
}

/** The length of a token secret in bytes. */
constexpr std::size_t secret_byte_count = 32;

/** Whether @p text is a lowercase hex SHA-256 digest. */
bool
is_sha256_hex (const QString &text)
{
  return text.size () == 64 && std::ranges::all_of (text, [] (QChar c) {
           return (c >= u'0' && c <= u'9') || (c >= u'a' && c <= u'f');
         });
}

/** A new token secret: random bytes from the operating system, as lowercase
 * hex. */
QString
generate_secret ()
{
  std::array<quint32, secret_byte_count / sizeof (quint32)> words{};
  QRandomGenerator::system ()->fillRange (
    words.data (), static_cast<qsizetype> (words.size ()));
  const auto bytes = QByteArray (
    reinterpret_cast<const char *> (words.data ()),
    static_cast<qsizetype> (sizeof (words)));
  return QString::fromLatin1 (bytes.toHex ());
}

/** What the process environment asks of the endpoint. */
struct EnvironmentRequest
{
  McpEnvironmentOverride state = McpEnvironmentOverride::None;

  /** The configuration to apply when the state is Active. */
  McpServiceConfig config;

  /** Whether ZRYTHM_MCP_PORT is the part that is not valid. */
  bool bad_port = false;
};

/** Reads the endpoint request of the process environment. Logs nothing. */
EnvironmentRequest
read_environment_request ()
{
  const auto environment = QProcessEnvironment::systemEnvironment ();
  const auto token_spec =
    environment.value (QStringLiteral ("ZRYTHM_MCP_TOKEN"));
  if (token_spec.isEmpty ())
    {
      return {};
    }
  const auto parts = token_spec.split (u':');
  const auto grant =
    parts.size () == 3 ? grant_from_identity (parts.at (1)) : std::nullopt;
  if (
    parts.size () != 3 || parts.at (0).isEmpty () || parts.at (2).isEmpty ()
    || !grant.has_value ())
    {
      EnvironmentRequest request;
      request.state = McpEnvironmentOverride::Malformed;
      return request;
    }
  EnvironmentRequest request;
  request.state = McpEnvironmentOverride::Active;
  request.config.enabled = true;
  request.config.tokens.append (
    McpToken::from_secret (parts.at (0), *grant, parts.at (2)));
  const auto port = environment.value (QStringLiteral ("ZRYTHM_MCP_PORT"));
  if (!port.isEmpty ())
    {
      bool       ok = false;
      const auto value = port.toInt (&ok);
      if (!ok || !is_valid_mcp_port (value))
        {
          request.state = McpEnvironmentOverride::Malformed;
          request.bad_port = true;
          return request;
        }
      request.config.port = value;
    }
  return request;
}

/** Renders an operation report as the tool result the client sees. */
zrythm::mcp::CallToolResult
render_report (const operations::OperationReport &report)
{
  nlohmann::json json = nlohmann::json::object ();
  json["status"] = status_name (report.status);
  if (!report.message.empty ())
    {
      json["message"] = report.message;
    }
  if (report.status == operations::OperationStatus::Refused)
    {
      json["refusal_kind"] =
        std::string (utils::to_string (report.refusal_kind));
    }
  if (!report.result.is_null ())
    {
      json["result"] = report.result;
    }
  json["command_texts"] = report.command_texts;
  auto affected = nlohmann::json::array ();
  for (const auto &object : report.affected_objects)
    {
      affected.push_back (
        nlohmann::json{
          { "uuid", object.uuid },
          { "kind", object.kind }
      });
    }
  json["affected_objects"] = std::move (affected);
  json["revision_before"] = report.revision_before;
  json["revision_after"] = report.revision_after;

  zrythm::mcp::CallToolResult result;
  result.content = { zrythm::mcp::TextContent{ json.dump () } };
  result.is_error = report.status != operations::OperationStatus::Applied;
  return result;
}

} // namespace

McpToken
McpToken::from_secret (const QString &name, McpGrant grant, const QString &secret)
{
  return McpToken{
    name,
    grant,
    QString::fromLatin1 (
      QCryptographicHash::hash (secret.toUtf8 (), QCryptographicHash::Sha256)
        .toHex ()),
  };
}

QList<McpToken>
parse_token_entries (const QStringList &entries)
{
  QList<McpToken> tokens;
  QSet<QString>   seen_hashes;
  for (const auto &entry : entries)
    {
      const auto parts = entry.split (u'|');
      const auto grant =
        parts.size () == 3 ? grant_from_identity (parts.at (1)) : std::nullopt;
      const auto hash =
        parts.size () == 3 ? parts.at (2).toLower () : QString ();
      if (
        parts.size () != 3 || parts.at (0).isEmpty () || !grant.has_value ()
        || !is_sha256_hex (hash))
        {
          z_warning (
            "ignoring malformed MCP token entry '{}'",
            utils::Utf8String::from_qstring (entry).view ());
          continue;
        }
      if (seen_hashes.contains (hash))
        {
          // Two tokens sharing one secret cannot be distinguished at
          // authentication; the first entry keeps its grant.
          z_warning (
            "ignoring MCP token entry '{}': its secret is already used by "
            "an earlier entry",
            utils::Utf8String::from_qstring (entry).view ());
          continue;
        }
      seen_hashes.insert (hash);
      tokens.append (McpToken{ parts.at (0), *grant, hash });
    }
  return tokens;
}

QStringList
write_token_entries (std::span<const McpToken> tokens)
{
  QStringList   entries;
  QSet<QString> seen_hashes;
  entries.reserve (static_cast<qsizetype> (tokens.size ()));
  for (const auto &token : tokens)
    {
      // The parser drops an entry that breaks any of these rules.
      assert (!token.name.isEmpty () && !token.name.contains (u'|'));
      assert (is_sha256_hex (token.sha256_hex));
      assert (!seen_hashes.contains (token.sha256_hex));
      seen_hashes.insert (token.sha256_hex);
      entries.append (
        token.name + u'|' + grant_identity (token.grant) + u'|'
        + token.sha256_hex);
    }
  return entries;
}

std::expected<void, TokenRejection>
validate_new_token_name (std::span<const McpToken> tokens, const QString &name)
{
  if (name.trimmed ().isEmpty ())
    {
      return std::unexpected (TokenRejection::EmptyName);
    }
  if (name.contains (u'|') || name.contains (u':'))
    {
      return std::unexpected (TokenRejection::ForbiddenCharacter);
    }
  if (name.front ().isSpace () || name.back ().isSpace ())
    {
      return std::unexpected (TokenRejection::SurroundingWhitespace);
    }
  if (std::ranges::any_of (tokens, [&name] (const McpToken &token) {
        return token.name == name;
      }))
    {
      return std::unexpected (TokenRejection::DuplicateName);
    }
  return {};
}

std::expected<QString, TokenRejection>
add_token (QList<McpToken> &tokens, const QString &name, McpGrant grant)
{
  if (const auto valid = validate_new_token_name (tokens, name); !valid)
    {
      return std::unexpected (valid.error ());
    }
  const auto secret = generate_secret ();
  tokens.append (McpToken::from_secret (name, grant, secret));
  return secret;
}

McpEnvironmentOverride
mcp_environment_override ()
{
  return read_environment_request ().state;
}

std::optional<McpServiceConfig>
mcp_config_from_environment ()
{
  const auto request = read_environment_request ();
  switch (request.state)
    {
    case McpEnvironmentOverride::None:
      return std::nullopt;
    case McpEnvironmentOverride::Active:
      return request.config;
    case McpEnvironmentOverride::Malformed:
      break;
    }
  if (request.bad_port)
    {
      z_warning (
        "ZRYTHM_MCP_PORT must be a TCP port number; the endpoint stays "
        "off");
    }
  else
    {
      // The settings configuration must not take over with different
      // credentials.
      z_warning (
        "ZRYTHM_MCP_TOKEN must be 'name:grant:secret' with a non-empty "
        "secret and a grant of query, control or full; the endpoint stays "
        "off");
    }
  return McpServiceConfig{};
}

McpService::McpService (
  const operations::OperationRegistry &registry,
  operations::OperationDispatcher     &dispatcher,
  undo::UndoStack                     &undo_stack,
  QObject *                            parent)
    : QObject (parent), registry_ (registry), dispatcher_ (dispatcher),
      undo_stack_ (undo_stack)
{
}

McpService::~McpService ()
{
  stop ();
}

void
McpService::apply_config (const McpServiceConfig &config)
{
  const bool valid_port = is_valid_mcp_port (config.port);
  if (config.enabled && !valid_port)
    {
      z_warning (
        "MCP port {} is not a TCP port number; the endpoint stays off",
        config.port);
    }
  const bool should_run =
    config.enabled && valid_port && !config.tokens.isEmpty ();
  if (
    should_run && is_running () && config_.port == config.port
    && config_.tokens == config.tokens)
    {
      return;
    }
  if (config.enabled && config.tokens.isEmpty ())
    {
      z_warning (
        "MCP is enabled but no tokens are configured; the endpoint stays "
        "off because it would accept unauthenticated callers");
    }
  stop ();
  config_ = config;
  if (should_run)
    {
      start ();
    }
}

void
McpService::start ()
{
  assert (!is_running ());

  auto catalog = make_tool_catalog (registry_);
  if (!catalog.has_value ())
    {
      // The catalog comes from the registry, so an invalid tool
      // definition is a programming error.
      const auto message =
        utils::Utf8String::from_utf8_encoded_string (catalog.error ())
          .to_qstring ();
      z_warning ("{}", utils::Utf8String::from_qstring (message).view ());
      Q_EMIT error_occurred (message, McpStartError::Other);
      return;
    }

  McpWorkerController::Config worker_config;
  worker_config.port = static_cast<quint16> (config_.port);
  for (const auto &token : config_.tokens)
    {
      worker_config.token_hashes.insert (token.sha256_hex, token.grant);
    }
  worker_config.catalog = std::move (*catalog);

  thread_ = new QThread (this);
  worker_ = new McpWorkerController ([this] (const McpDispatchRequest &request) {
    // Runs on the worker thread; queue the dispatch onto the main
    // thread. If this service is destroyed before the event is
    // delivered, Qt drops the event with it.
    QMetaObject::invokeMethod (
      this, [this, request] () mutable { run_dispatch (request); },
      Qt::QueuedConnection);
  });
  worker_->moveToThread (thread_);
  // The worker's objects (sockets, notifiers) belong to the worker thread;
  // deleting it there while the thread finishes avoids cross-thread
  // QObject destruction.
  connect (thread_, &QThread::finished, worker_, &QObject::deleteLater);

  connect (
    thread_, &QThread::started, worker_,
    [worker = worker_, config = std::move (worker_config)] () mutable {
      worker->start_server (config);
    },
    Qt::QueuedConnection);
  connect (worker_, &McpWorkerController::server_started, this, [this] () {
    z_info ("MCP endpoint listening on 127.0.0.1:{}", config_.port);
    Q_EMIT started ();
  });
  connect (
    worker_, &McpWorkerController::server_error, this,
    [this] (const QString &message, const McpStartError reason) {
      // A failed endpoint stops: it reports not running, and applying
      // the same configuration again can restart it.
      z_warning (
        "MCP endpoint error: {}",
        utils::Utf8String::from_qstring (message).view ());
      Q_EMIT error_occurred (message, reason);
      stop ();
    });

  // The journal callback runs synchronously on the main thread after each
  // undo stack mutation; entries are queued to the worker thread.
  undo_stack_.set_journal_callback ([this] (const undo::JournalEntry &entry) {
    nlohmann::json json = nlohmann::json::object ();
    json["revision"] = entry.revision;
    json["kind"] = journal_kind_name (entry.kind);
    json["command_texts"] = entry.command_texts;
    QMetaObject::invokeMethod (
      worker_,
      [worker = worker_, json = std::move (json)] () mutable {
        worker->push_journal_entry (std::move (json));
      },
      Qt::QueuedConnection);
  });

  thread_->start ();
}

void
McpService::stop ()
{
  if (!is_running ())
    {
      return;
    }
  // Clear the journal callback first: entries queued after this point
  // would arrive at a dying worker. The worker is deleted by its
  // deleteLater connection while the thread finishes, on the thread its
  // objects live on.
  undo_stack_.set_journal_callback ({});
  worker_ = nullptr;
  thread_->quit ();
  thread_->wait ();
  thread_->deleteLater ();
  thread_ = nullptr;
  Q_EMIT stopped ();
}

void
McpService::run_dispatch (const McpDispatchRequest &request)
{
  if (worker_ == nullptr)
    {
      // The endpoint stopped before the call reached the main thread;
      // the transport that would carry the response is gone, so the
      // promise stays unresolved and the edit is not applied.
      return;
    }
  operations::DispatchOptions options;
  options.expected_revision = request.expected_revision;
  const auto report =
    dispatcher_.dispatch (request.tool, request.arguments, options);
  const auto result = render_report (report);
  QMetaObject::invokeMethod (
    worker_,
    [worker = worker_, completion = request.completion, result] () mutable {
      worker->fulfill_call (completion, result);
    },
    Qt::QueuedConnection);
}

} // namespace zrythm::frontends::mcp
