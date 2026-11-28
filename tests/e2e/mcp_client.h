// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

/**
 * @file
 * Asks the MCP endpoint of a running application for the list of its
 * tracks.
 *
 * The run gets its endpoint through the environment variables
 * ZRYTHM_MCP_TOKEN and ZRYTHM_MCP_PORT, which the application reads in
 * place of the settings.
 */

#include <atomic>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <thread>

#include <QByteArray>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>

#include "launcher.h"

namespace zrythm::e2e
{

/** Time between two calls while the application starts and loads. */
inline constexpr auto kMcpCallInterval = std::chrono::milliseconds{ 100 };

/** Time for one answer of the endpoint. */
inline constexpr auto kMcpAnswerTimeout = std::chrono::seconds{ 5 };

/** The secret of the tokens of the runs. A token grants query access only. */
inline constexpr auto kMcpSecret = "end-to-end-test";

/** The MCP name of the operation that lists the tracks of the project. */
inline constexpr auto kTrackListTool = "track_list";

/** A run with the MCP endpoint enabled, and the tracks it listed. */
struct McpRunResult
{
  /** How the run ended. */
  RunResult run;
  /**
   * The summaries of the track_list tool (uuid, name and type), in the
   * order of the track list. Empty when the endpoint listed no track.
   */
  QJsonArray tracks;
  /** The body of the last answer of the endpoint. */
  QByteArray last_answer;

  /** Returns the tracks as one JSON text, for the messages of
   * GoogleTest. */
  [[nodiscard]] std::string describe_tracks () const
  {
    return to_utf8 (QJsonDocument (tracks).toJson (QJsonDocument::Compact));
  }
};

namespace detail
{

/** The status code of @p response. */
[[nodiscard]] inline int
status_of (const QByteArray &response)
{
  const auto start = response.indexOf (' ') + 1;
  return response.mid (start, response.indexOf (' ', start) - start).toInt ();
}

/** The body of @p response, with the chunks of a chunked body joined. */
[[nodiscard]] inline QByteArray
body_of (const QByteArray &response)
{
  const auto header_end = response.indexOf ("\r\n\r\n");
  if (header_end < 0)
    {
      return {};
    }
  const auto chunked =
    response.left (header_end).toLower ().contains ("transfer-encoding: chunked");
  auto body = response.mid (header_end + 4);
  if (!chunked)
    {
      return body;
    }
  QByteArray joined;
  for (;;)
    {
      const auto line_end = body.indexOf ("\r\n");
      const auto size = body.left (line_end).toLong (nullptr, 16);
      body.remove (0, line_end + 2);
      if (size == 0)
        {
          return joined;
        }
      joined += body.left (size);
      body.remove (0, size + 2);
    }
}

} // namespace detail

/**
 * @brief Sends the track_list call to the endpoint on @p port.
 *
 * @return The response with its status line, or no value when the
 * endpoint sent none.
 */
[[nodiscard]] inline std::optional<QByteArray>
call_track_list (quint16 port)
{
  const QJsonObject meta{
    { "io.modelcontextprotocol/protocolVersion",    "2026-07-28"  },
    { "io.modelcontextprotocol/clientCapabilities", QJsonObject{} },
  };
  const auto message =
    QJsonDocument (
      QJsonObject{
        { "jsonrpc", "2.0"        },
        { "id",      1            },
        { "method",  "tools/call" },
        { "params",
         QJsonObject{
            { "name", kTrackListTool },
            { "arguments", QJsonObject{} },
            { "_meta", meta } }   }
  })
      .toJson (QJsonDocument::Compact);
  const QByteArray request =
    "POST /mcp HTTP/1.1\r\n"
    "Host: 127.0.0.1\r\n"
    "Authorization: Bearer "
    + QByteArray (kMcpSecret)
    + "\r\n"
      "Content-Type: application/json\r\n"
      "Accept: application/json, text/event-stream\r\n"
      "MCP-Protocol-Version: 2026-07-28\r\n"
      "Mcp-Method: tools/call\r\n"
      "Mcp-Name: track_list\r\n"
      "Connection: close\r\n"
      "Content-Length: "
    + QByteArray::number (message.size ()) + "\r\n\r\n" + message;

  QTcpSocket socket;
  socket.connectToHost (QHostAddress::LocalHost, port);
  if (!socket.waitForConnected (1000))
    {
      return std::nullopt;
    }
  socket.write (request);
  // The endpoint ends the answer by closing the connection
  socket.waitForDisconnected (to_msecs (kMcpAnswerTimeout));
  const QByteArray response = socket.readAll ();
  if (response.isEmpty ())
    {
      return std::nullopt;
    }
  return response;
}

/**
 * @brief Returns the track list that the endpoint holds in @p response.
 *
 * @return The track summaries, or no value when the response is not the
 * answer of a successful call that lists tracks.
 */
[[nodiscard]] inline std::optional<QJsonArray>
tracks_of (const QByteArray &response)
{
  if (detail::status_of (response) != 200)
    {
      return std::nullopt;
    }
  const auto envelope = QJsonDocument::fromJson (detail::body_of (response));
  const auto content =
    envelope.object ().value ("result").toObject ().value ("content").toArray ();
  if (content.isEmpty ())
    {
      return std::nullopt;
    }
  // The text of the result is the report of the operation
  const auto report = QJsonDocument::fromJson (
    content.first ().toObject ().value ("text").toString ().toUtf8 ());
  if (report.isNull ())
    {
      return std::nullopt;
    }
  if (report.object ().value ("status").toString () != "applied")
    {
      return std::nullopt;
    }
  const auto tracks =
    report.object ().value ("result").toObject ().value ("items").toArray ();
  if (tracks.isEmpty ())
    {
      return std::nullopt;
    }
  return tracks;
}

/**
 * @brief Runs the application with its MCP endpoint enabled, asks for
 * the track list until the endpoint lists a track, and waits for the end
 * of the run.
 *
 * @param launcher Launcher of the run.
 * @param arguments Command line arguments, without the program name.
 * The run must end by itself, as a run with --quit-after-startup does.
 * @return The run and the first track list that the endpoint returned.
 */
[[nodiscard]] inline McpRunResult
run_and_list_tracks (const ZrythmLauncher &launcher, QStringList arguments)
{
  // The probe releases the port before the application listens on it
  QTcpServer probe;
  probe.listen (QHostAddress::LocalHost, 0);
  const auto port = probe.serverPort ();
  probe.close ();
  if (port == 0)
    {
      throw std::runtime_error ("Cannot find a free port for the MCP endpoint");
    }

  // The application takes the endpoint from the environment of the run,
  // which is the environment of this process
  qputenv ("ZRYTHM_MCP_TOKEN", "e2e:query:" + QByteArray (kMcpSecret));
  qputenv ("ZRYTHM_MCP_PORT", QByteArray::number (port));

  McpRunResult      result;
  std::atomic<bool> run_finished{ false };
  std::jthread      run_thread ([&] {
    result.run = launcher.run (std::move (arguments), kQuitAfterStartup);
    run_finished = true;
  });
  // The endpoint listens once the session exists and lists the tracks
  // once the project is loaded, so the calls repeat until the first
  // track list or the end of the run
  const auto deadline = std::chrono::steady_clock::now () + kQuitAfterStartup;
  while (
    result.tracks.isEmpty () && !run_finished
    && std::chrono::steady_clock::now () < deadline)
    {
      const auto answer = call_track_list (port);
      if (answer)
        {
          result.last_answer = *answer;
          if (const auto tracks = tracks_of (*answer))
            {
              result.tracks = *tracks;
              break;
            }
        }
      std::this_thread::sleep_for (kMcpCallInterval);
    }
  run_thread.join ();

  qunsetenv ("ZRYTHM_MCP_TOKEN");
  qunsetenv ("ZRYTHM_MCP_PORT");
  return result;
}

} // namespace zrythm::e2e
