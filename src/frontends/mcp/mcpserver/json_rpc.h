// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <nlohmann/json.hpp>

namespace zrythm::mcp
{

using Json = nlohmann::json;

/**
 * @brief Error codes of the JSON-RPC 2.0 errors, of the MCP errors that the
 * server sends, and of the errors of this server.
 *
 * The codes of this server are positive. They lie outside the range that
 * JSON-RPC 2.0 reserves (-32768 to -32000), where the 2026-07-28 revision
 * allows no new code.
 */
enum class JsonRpcErrorCode : std::int32_t
{
  ParseError = -32700,
  InvalidRequest = -32600,
  MethodNotFound = -32601,
  InvalidParams = -32602,
  InternalError = -32603,
  /** Session-based revisions: no provider serves the requested URI. */
  ResourceNotFound = -32002,
  /** A required HTTP header is missing, malformed, or differs from the body. */
  HeaderMismatch = -32020,
  /** The revision in the MCP-Protocol-Version header is not supported. */
  UnsupportedProtocolVersion = -32022,
  /** The server refuses to hold more state than its limits allow. */
  LimitReached = 1,
  /** Session-based revisions: the session of the request is unknown or has
   * ended. */
  UnknownSession = 2,
};

[[nodiscard]] auto
to_string (JsonRpcErrorCode code) noexcept -> std::string_view;

/**
 * @brief The id of a JSON-RPC request: a string or a number.
 *
 * The server only echoes it.
 */
class RequestId
{
public:
  /** Returns nullopt unless @p value is a string or a number. */
  [[nodiscard]] static auto
  parse (const Json &value) -> std::optional<RequestId>;

  [[nodiscard]] auto value () const -> const Json & { return value_; }
  auto               operator== (const RequestId &other) const -> bool
  {
    return value_ == other.value_;
  }

private:
  explicit RequestId (Json value) : value_ (std::move (value)) { }
  Json value_;
};

/** The error object of a JSON-RPC error response. */
struct JsonRpcError
{
  JsonRpcErrorCode code;
  std::string      message;
  /** Extra data of the error, or null when there is none. */
  Json data;

  [[nodiscard]] auto to_json () const -> Json;
};

/** A JSON-RPC error response. @ref id is the id of the failed request when it
 * is known. */
struct JsonRpcErrorResponse
{
  std::optional<RequestId> id;
  JsonRpcError             error;

  [[nodiscard]] auto to_json () const -> Json;
};

/** Builds an error response for the request @p id. */
[[nodiscard]] auto
error_response (
  std::optional<RequestId> id,
  JsonRpcErrorCode         code,
  std::string              message,
  Json                     data = nullptr) -> JsonRpcErrorResponse;

/** A request of the client. */
struct JsonRpcRequest
{
  RequestId   id;
  std::string method;
  /** The params of the request: an object or an array. Nullopt when absent. */
  std::optional<Json> params;
};

/** A notification of the client. */
struct JsonRpcNotification
{
  std::string         method;
  std::optional<Json> params;
};

/**
 * @brief A response that the client sent to a request of the server.
 *
 * The server keeps no requests in flight, so it ignores these messages.
 */
struct ClientResponse
{
};

using JsonRpcMessage =
  std::variant<JsonRpcRequest, JsonRpcNotification, ClientResponse>;

/** Parses the body of a message. The error is a ParseError response. */
[[nodiscard]] auto
parse_json (std::string_view text) -> std::expected<Json, JsonRpcErrorResponse>;

/**
 * @brief Validates one parsed message.
 *
 * Returns InvalidRequest for every message that is not a request, a
 * notification, or a response.
 */
[[nodiscard]] auto
parse_json_rpc_message (const Json &message)
  -> std::expected<JsonRpcMessage, JsonRpcErrorResponse>;

/** Builds the response to the request @p id with the result @p result. */
[[nodiscard]] auto
result_response (const RequestId &id, const Json &result) -> Json;

/** Builds a notification. The params are omitted when @p params is nullopt. */
[[nodiscard]] auto
notification_response (
  std::string_view    method,
  std::optional<Json> params = std::nullopt) -> Json;

} // namespace zrythm::mcp
