// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>

#include <mcpserver/http_request.h>
#include <mcpserver/json_rpc.h>

namespace zrythm::mcp
{

enum class HttpStatus : int
{
  Ok = 200,
  Accepted = 202,
  BadRequest = 400,
  Unauthorized = 401,
  Forbidden = 403,
  NotFound = 404,
  MethodNotAllowed = 405,
  NotAcceptable = 406,
  PayloadTooLarge = 413,
  UnsupportedMediaType = 415,
  TooManyRequests = 429,
  InternalServerError = 500,
};

struct HttpHeader
{
  QByteArray name;
  QByteArray value;
};

/** The final HTTP response to one exchange. */
class HttpReply
{
public:
  /** A reply without a body. */
  [[nodiscard]] static auto
  without_body (HttpStatus status, std::vector<HttpHeader> headers = {})
    -> HttpReply
  {
    return HttpReply (status, std::nullopt, QByteArray (), std::move (headers));
  }

  /** `202 Accepted` without a body. */
  [[nodiscard]] static auto accepted () -> HttpReply
  {
    return without_body (HttpStatus::Accepted);
  }

  /** A reply with the JSON body @p json. */
  [[nodiscard]] static auto
  json (HttpStatus status, const Json &json, std::vector<HttpHeader> headers = {})
    -> HttpReply
  {
    return HttpReply (
      status, json.dump (-1, ' ', false, Json::error_handler_t::replace),
      "application/json", std::move (headers));
  }

  /** A reply with the body of a JSON-RPC error response. */
  [[nodiscard]] static auto json_rpc_error (
    HttpStatus                  status,
    const JsonRpcErrorResponse &error,
    std::vector<HttpHeader>     headers = {}) -> HttpReply
  {
    return json (status, error.to_json (), std::move (headers));
  }

  /** `405 Method Not Allowed` with the methods that the endpoint serves. */
  [[nodiscard]] static auto
  method_not_allowed (std::initializer_list<HttpMethod> allowed) -> HttpReply
  {
    QByteArray names;
    for (const auto method : allowed)
      {
        if (!names.isEmpty ())
          names += ", ";
        names += QByteArray (to_string (method));
      }
    return without_body (
      HttpStatus::MethodNotAllowed,
      {
        { "Allow", std::move (names) }
    });
  }

  [[nodiscard]] auto status () const -> HttpStatus { return status_; }

  /** The body of the reply. It is absent for a reply without a body. */
  [[nodiscard]] auto body () const -> const std::optional<std::string> &
  {
    return body_;
  }

  /** The media type of the body. It is empty for a reply without a body. */
  [[nodiscard]] auto content_type () const -> const QByteArray &
  {
    return content_type_;
  }

  /** The headers that the reply adds to the response. */
  [[nodiscard]] auto headers () const -> std::span<const HttpHeader>
  {
    return headers_;
  }

private:
  HttpReply (
    HttpStatus                 status,
    std::optional<std::string> body,
    QByteArray                 content_type,
    std::vector<HttpHeader>    headers)
      : status_ (status), body_ (std::move (body)),
        content_type_ (std::move (content_type)), headers_ (std::move (headers))
  {
  }

  HttpStatus                 status_;
  std::optional<std::string> body_;
  QByteArray                 content_type_;
  std::vector<HttpHeader>    headers_;
};

using ExchangeId = std::uint64_t;

/**
 * @brief One HTTP request together with its response.
 *
 * An exchange ends with one reply or with a closed stream. Every function
 * must be called on the thread of the server.
 */
class HttpExchange
{
public:
  virtual ~HttpExchange () = default;

  [[nodiscard]] virtual auto id () const -> ExchangeId = 0;

  /** True once the client closed its side of the connection. */
  [[nodiscard]] virtual auto client_gone () const -> bool = 0;

  /**
   * @brief Ends the exchange with @p reply.
   *
   * Returns false when the exchange already ended.
   */
  [[nodiscard]] virtual auto reply (HttpReply reply) -> bool = 0;

  /**
   * @brief Turns the exchange into a stream of events.
   *
   * Returns false when the exchange already ended or the client is gone.
   */
  [[nodiscard]] virtual auto start_stream () -> bool = 0;

  /** Sends one event on the stream. Returns false when the stream is closed. */
  [[nodiscard]] virtual auto send (const Json &message) -> bool = 0;

  /** Ends the stream. */
  virtual auto close_stream () -> void = 0;
};

} // namespace zrythm::mcp
