// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <ranges>
#include <span>

#include "frontends/mcp/mcp_json.h"
#include "frontends/mcp/mcp_service.h"
#include "frontends/mcp/mcp_tool_catalog.h"
#include "undo/journal.h"
#include "undo/undo_stack.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QProcessEnvironment>
#include <QSet>
#include <QThread>
#include <QtMcpCommon/QMcpCallToolResult>
#include <QtMcpCommon/QMcpTextContent>

#include "operations/operation_dispatcher.h"
#include "operations/operation_registry.h"

namespace zrythm::frontends::mcp
{

namespace
{

QString
status_name (operations::OperationStatus status)
{
  switch (status)
    {
    case operations::OperationStatus::Applied:
      return QStringLiteral ("applied");
    case operations::OperationStatus::Refused:
      return QStringLiteral ("refused");
    case operations::OperationStatus::Error:
      return QStringLiteral ("error");
    }
  return QStringLiteral ("error");
}

QString
journal_kind_name (undo::JournalEntryKind kind)
{
  switch (kind)
    {
    case undo::JournalEntryKind::Push:
      return QStringLiteral ("push");
    case undo::JournalEntryKind::Undo:
      return QStringLiteral ("undo");
    case undo::JournalEntryKind::Redo:
      return QStringLiteral ("redo");
    case undo::JournalEntryKind::Abort:
      return QStringLiteral ("abort");
    }
  return QStringLiteral ("push");
}

/** Renders an operation report as the tool result the client sees. */
QMcpCallToolResult
render_report (const operations::OperationReport &report)
{
  QJsonObject json;
  json.insert (QStringLiteral ("status"), status_name (report.status));
  if (!report.message.empty ())
    {
      json.insert (
        QStringLiteral ("message"),
        utils::Utf8String::from_utf8_encoded_string (report.message)
          .to_qstring ());
    }
  if (report.status == operations::OperationStatus::Refused)
    {
      json.insert (
        QStringLiteral ("refusal_kind"),
        utils::Utf8String::from_utf8_encoded_string (
          utils::to_string (report.refusal_kind))
          .to_qstring ());
    }
  if (!report.result.is_null ())
    {
      json.insert (
        QStringLiteral ("result"), qjson_from_nlohmann (report.result));
    }
  auto command_texts = QJsonArray{};
  for (const auto &text : report.command_texts)
    {
      command_texts.append (
        utils::Utf8String::from_utf8_encoded_string (text).to_qstring ());
    }
  json.insert (QStringLiteral ("command_texts"), command_texts);
  auto affected = QJsonArray{};
  for (const auto &object : report.affected_objects)
    {
      affected.append (
        QJsonObject{
          { "uuid",
           utils::Utf8String::from_utf8_encoded_string (object.uuid)
              .to_qstring () },
          { "kind",
           utils::Utf8String::from_utf8_encoded_string (object.kind)
              .to_qstring () },
      });
    }
  json.insert (QStringLiteral ("affected_objects"), affected);
  json.insert (
    QStringLiteral ("revision_before"),
    static_cast<double> (report.revision_before));
  json.insert (
    QStringLiteral ("revision_after"),
    static_cast<double> (report.revision_after));

  QMcpCallToolResult result;
  QMcpTextContent    content;
  content.setText (
    QString::fromUtf8 (QJsonDocument (json).toJson (QJsonDocument::Compact)));
  result.setContent ({ content });
  result.setIsError (report.status != operations::OperationStatus::Applied);
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
      const bool valid_hash =
        hash.size () == 64
        && std::ranges::all_of (
          std::span (hash.utf16 (), hash.size ()), [] (const char16_t c) {
            return (c >= u'0' && c <= u'9') || (c >= u'a' && c <= u'f');
          });
      if (
        parts.size () != 3 || parts.at (0).isEmpty () || !grant.has_value ()
        || !valid_hash)
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

std::optional<McpServiceConfig>
mcp_config_from_environment ()
{
  const auto environment = QProcessEnvironment::systemEnvironment ();
  const auto token_spec =
    environment.value (QStringLiteral ("ZRYTHM_MCP_TOKEN"));
  if (token_spec.isEmpty ())
    {
      return std::nullopt;
    }
  const auto parts = token_spec.split (u':');
  const auto grant =
    parts.size () == 3 ? grant_from_identity (parts.at (1)) : std::nullopt;
  if (
    parts.size () != 3 || parts.at (0).isEmpty () || parts.at (2).isEmpty ()
    || !grant.has_value ())
    {
      // A malformed specification is authoritative: the settings
      // configuration must not take over with different credentials.
      z_warning (
        "ZRYTHM_MCP_TOKEN must be 'name:grant:secret' with a non-empty "
        "secret and a grant of query, control or full; the endpoint stays "
        "off");
      return McpServiceConfig{};
    }
  McpServiceConfig config;
  config.enabled = true;
  config.tokens.append (
    McpToken::from_secret (parts.at (0), *grant, parts.at (2)));
  const auto port = environment.value (QStringLiteral ("ZRYTHM_MCP_PORT"));
  if (!port.isEmpty ())
    {
      bool       ok = false;
      const auto value = port.toInt (&ok);
      if (!ok || value <= 0 || value > 65535)
        {
          z_warning (
            "ZRYTHM_MCP_PORT must be a TCP port number; the endpoint stays "
            "off");
          return McpServiceConfig{};
        }
      config.port = value;
    }
  return config;
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
  const bool valid_port = config.port > 0 && config.port <= 65535;
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

  McpWorkerController::Config worker_config;
  worker_config.port = config_.port;
  for (const auto &token : config_.tokens)
    {
      worker_config.token_hashes.insert (token.sha256_hex, token.grant);
    }
  worker_config.catalog = make_tool_catalog (registry_);
  worker_config.initial_journal_entries = QJsonArray{};

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
    [this] (const QString &message) {
      // A failed endpoint stops: it reports not running, and applying
      // the same configuration again can restart it.
      z_warning (
        "MCP endpoint error: {}",
        utils::Utf8String::from_qstring (message).view ());
      Q_EMIT error_occurred (message);
      stop ();
    });

  // The journal callback runs synchronously on the main thread after each
  // undo stack mutation; entries are queued to the worker thread.
  undo_stack_.set_journal_callback ([this] (const undo::JournalEntry &entry) {
    QJsonObject json;
    json.insert (
      QStringLiteral ("revision"), static_cast<double> (entry.revision));
    json.insert (QStringLiteral ("kind"), journal_kind_name (entry.kind));
    auto command_texts = QJsonArray{};
    for (const auto &text : entry.command_texts)
      {
        command_texts.append (
          utils::Utf8String::from_utf8_encoded_string (text).to_qstring ());
      }
    json.insert (QStringLiteral ("command_texts"), command_texts);
    QMetaObject::invokeMethod (
      worker_,
      [worker = worker_, json] () mutable { worker->push_journal_entry (json); },
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
  const auto report = [&] {
    operations::DispatchOptions options;
    options.expected_revision = request.expected_revision;
    return dispatcher_.dispatch (
      std::string (utils::Utf8String::from_qstring (request.tool).view ()),
      nlohmann_from_qjson (request.arguments), options);
  }();
  const auto result = render_report (report);
  QMetaObject::invokeMethod (
    worker_,
    [worker = worker_, completion = request.completion, result] () mutable {
      worker->fulfill_call (completion, result);
    },
    Qt::QueuedConnection);
}

} // namespace zrythm::frontends::mcp
