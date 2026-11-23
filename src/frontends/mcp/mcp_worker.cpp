// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <cstring>
#include <ranges>

#include "frontends/mcp/mcp_json.h"
#include "frontends/mcp/mcp_qtmcp_includes.h"
#include "frontends/mcp/mcp_worker.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QtMcpCommon/QMcpCallToolRequest>
#include <QtMcpCommon/QMcpJSONRPCErrorError>
#include <QtMcpCommon/QMcpListToolsRequest>
#include <QtMcpCommon/QMcpListToolsResult>
#include <QtMcpCommon/QMcpReadResourceResultContents>
#include <QtMcpCommon/QMcpResource>
#include <QtMcpCommon/QMcpTextContent>
#include <QtMcpCommon/QMcpTextResourceContents>

namespace zrythm::frontends::mcp
{

namespace
{

constexpr auto kJournalResourceUri = "zrythm://journal";

/** Entries kept in the journal resource, oldest dropped first. */
constexpr int kJournalTailLength = 256;

QMcpCallToolResult
error_result (const QString &message)
{
  QMcpCallToolResult result;
  QMcpTextContent    content;
  content.setText (message);
  result.setContent ({ content });
  result.setIsError (true);
  return result;
}

QMcpResource
make_journal_resource ()
{
  QMcpResource resource;
  resource.setUri (QUrl (QString::fromUtf8 (kJournalResourceUri)));
  resource.setName (QStringLiteral ("journal"));
  resource.setDescription (
    QStringLiteral ("Undo history journal, newest entry last"));
  return resource;
}

QMcpReadResourceResultContents
make_journal_contents (const QMcpResource &resource, const QString &text)
{
  QMcpReadResourceResultContents contents;
  QMcpTextResourceContents       text_contents;
  text_contents.setUri (resource.uri ());
  text_contents.setText (text);
  contents.setTextResourceContents (text_contents);
  return contents;
}

QString
journal_text (const QJsonArray &entries)
{
  return QString::fromUtf8 (
    QJsonDocument (
      QJsonObject{
        { "entries", entries },
  })
      .toJson (QJsonDocument::Compact));
}

} // namespace

McpWorkerController::McpWorkerController (McpDispatchFn dispatch_fn)
    : dispatch_fn_ (std::move (dispatch_fn))
{
}

McpWorkerController::~McpWorkerController () = default;

QMcpServerSession *
McpWorkerController::find_session (const QUuid &session_id) const
{
  const auto sessions = server_->sessions ();
  const auto it = std::ranges::find_if (
    sessions, [session_id] (const QMcpServerSession * session) {
      return session->sessionId () == session_id;
    });
  return it != sessions.end () ? *it : nullptr;
}

void
McpWorkerController::start_server (Config config)
{
  config_ = std::move (config);
  journal_entries_ = config_.initial_journal_entries;

  if (config_.token_hashes.isEmpty ())
    {
      const auto message = QStringLiteral (
        "refusing to start without a token: the endpoint would accept "
        "unauthenticated callers");
      z_warning ("{}", utils::Utf8String::from_qstring (message).view ());
      Q_EMIT server_error (message);
      return;
    }

  auto * backend = new QMcpServerStreamableHttp (this);
  backend->setAuthenticator ([this] (const QNetworkRequest &request) {
    const auto header =
      QString::fromUtf8 (request.rawHeader ("Authorization")).trimmed ();
    if (!header.startsWith (QStringLiteral ("Bearer "), Qt::CaseInsensitive))
      {
        return QString{};
      }
    const auto token = header.mid (strlen ("Bearer ")).trimmed ();
    const auto digest = QString::fromLatin1 (
      QCryptographicHash::hash (token.toUtf8 (), QCryptographicHash::Sha256)
        .toHex ());
    const auto grant = config_.token_hashes.constFind (digest);
    return grant != config_.token_hashes.constEnd ()
             ? grant_identity (grant.value ())
             : QString{};
  });

  connect (
    backend, &QMcpServerStreamableHttp::authenticated, this,
    [this] (const QUuid &session, const QString &identity) {
      const auto grant = grant_from_identity (identity);
      if (!grant.has_value ())
        {
          // The identity comes from this object's own authenticator, so an
          // unknown name is a programming error.
          z_warning (
            "MCP session {} authenticated with unknown grant '{}'",
            utils::Utf8String::from_qstring (session.toString ()).view (),
            utils::Utf8String::from_qstring (identity).view ());
          return;
        }
      session_grants_.insert (session, *grant);
    });

  auto * server = new QMcpServer (backend, this);
  server_ = server;
  server->setInstructions (QStringLiteral (
    "Zrythm operations. Tools map one-to-one to operations. Every tool "
    "call returns a JSON report with the project revision before and "
    "after. Edit tools accept an optional 'expected_revision' argument: "
    "when it does not match the current revision the edit is refused and "
    "the report carries the current revision. Journal entries are "
    "published as updates of the zrythm://journal resource."));

  server->addRequestHandler (
    [this] (
      const QUuid            &session_id, const QMcpListToolsRequest &,
      QMcpJSONRPCErrorError * error) {
      const auto * session = find_session (session_id);
      if (session == nullptr || !session->isInitialized ())
        {
          error->setCode (-32000);
          error->setMessage (QStringLiteral ("Session not initialized"));
          return QMcpListToolsResult{};
        }
      QMcpListToolsResult result;
      result.setTools (config_.catalog.tools);
      return result;
    });

  server->addRequestHandler (
    [this] (
      const QUuid &session, const QMcpCallToolRequest &request,
      QMcpJSONRPCErrorError *) -> QFuture<QMcpCallToolResult> {
      const auto completion = std::make_shared<QPromise<QMcpCallToolResult>> ();
      completion->start ();
      const auto future = completion->future ();

      const auto * session_object = find_session (session);
      if (session_object == nullptr || !session_object->isInitialized ())
        {
          completion->addResult (
            error_result (QStringLiteral ("session not initialized")));
          completion->finish ();
          return future;
        }

      const auto &tool = request.params ().name ();
      const auto  kind = config_.catalog.kinds_by_name.constFind (tool);
      if (kind == config_.catalog.kinds_by_name.constEnd ())
        {
          completion->addResult (
            error_result (QStringLiteral ("unknown tool '%1'").arg (tool)));
          completion->finish ();
          return future;
        }

      const auto grant = session_grants_.constFind (session);
      if (grant == session_grants_.constEnd ())
        {
          // A session without a recorded grant is refused rather than
          // downgraded: permission lookups fail closed.
          completion->addResult (error_result (QStringLiteral (
            "the session holds no grant; the call is refused")));
          completion->finish ();
          return future;
        }
      if (!grant_allows (grant.value (), kind.value ()))
        {
          completion->addResult (error_result (
            QStringLiteral ("the '%1' grant does not allow '%2'")
              .arg (grant_identity (grant.value ()), tool)));
          completion->finish ();
          return future;
        }

      McpDispatchRequest dispatch;
      dispatch.tool = tool;
      dispatch.arguments = request.params ().arguments ();
      dispatch.completion = completion;
      if (dispatch.arguments.contains (QStringLiteral ("expected_revision")))
        {
          const auto expected =
            dispatch.arguments.value (QStringLiteral ("expected_revision"));
          const auto number = expected.toDouble ();
          if (
            expected.isDouble () && number >= 0 && number == std::floor (number)
            && number <= kMaxExactJsonInteger)
            {
              dispatch.expected_revision = static_cast<std::uint64_t> (number);
            }
          else
            {
              completion->addResult (error_result (QStringLiteral (
                "'expected_revision' must be a non-negative "
                "integer")));
              completion->finish ();
              return future;
            }
          dispatch.arguments.remove (QStringLiteral ("expected_revision"));
        }
      dispatch_fn_ (dispatch);
      return future;
    });

  connect (
    server, &QMcpServer::newSession, this,
    [this] (QMcpServerSession * session) { append_journal_resource (session); });
  connect (
    server, &QMcpServer::sessionDestroyed, this,
    [this] (QMcpServerSession * session) {
      session_grants_.remove (session->sessionId ());
    });
  connect (server, &QMcpServer::started, this, [this] () {
    Q_EMIT server_started ();
  });
  connect (
    server, &QMcpServer::errorOccurred, this,
    [this] (const QString &message) { Q_EMIT server_error (message); });

  server->start (QStringLiteral ("127.0.0.1:%1").arg (config_.port));
}

void
McpWorkerController::fulfill_call (
  std::shared_ptr<QPromise<QMcpCallToolResult>> completion,
  QMcpCallToolResult                            result)
{
  completion->addResult (std::move (result));
  completion->finish ();
}

void
McpWorkerController::push_journal_entry (QJsonObject entry)
{
  journal_entries_.append (entry);
  while (journal_entries_.size () > kJournalTailLength)
    {
      journal_entries_.removeAt (0);
    }
  update_journal_resources ();
}

void
McpWorkerController::append_journal_resource (QMcpServerSession * session)
{
  const auto resource = make_journal_resource ();
  session->appendResource (
    resource, make_journal_contents (resource, journal_text (journal_entries_)));
}

void
McpWorkerController::update_journal_resources ()
{
  // Journal entries can queue before the server exists; they stay in
  // journal_entries_ and publish when a session connects.
  if (server_ == nullptr)
    return;
  const auto sessions = server_->sessions ();
  if (sessions.isEmpty ())
    return;
  const auto text = journal_text (journal_entries_);
  for (auto * session : sessions)
    {
      const auto resource = make_journal_resource ();
      session->replaceResource (
        resource.uri (), resource, make_journal_contents (resource, text));
    }
}

} // namespace zrythm::frontends::mcp
