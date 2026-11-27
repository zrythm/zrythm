// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <mcpserver/json_rpc.h>

namespace zrythm::mcp
{

namespace
{

/** The id of the message @p message. Returns nullopt when the message has
 * no id or when its id is neither a string nor a number. */
auto
request_id_of (const Json &message) -> std::optional<RequestId>
{
  if (!message.is_object ())
    return std::nullopt;
  const auto id = message.find ("id");
  if (id == message.end ())
    return std::nullopt;
  return RequestId::parse (*id);
}

} // namespace

auto
to_string (const JsonRpcErrorCode code) noexcept -> std::string_view
{
  switch (code)
    {
    case JsonRpcErrorCode::ParseError:
      return "Parse error";
    case JsonRpcErrorCode::InvalidRequest:
      return "Invalid Request";
    case JsonRpcErrorCode::MethodNotFound:
      return "Method not found";
    case JsonRpcErrorCode::InvalidParams:
      return "Invalid params";
    case JsonRpcErrorCode::InternalError:
      return "Internal error";
    case JsonRpcErrorCode::ResourceNotFound:
      return "Resource not found";
    case JsonRpcErrorCode::HeaderMismatch:
      return "Header mismatch";
    case JsonRpcErrorCode::UnsupportedProtocolVersion:
      return "Unsupported protocol version";
    case JsonRpcErrorCode::LimitReached:
      return "Server limit reached";
    case JsonRpcErrorCode::UnknownSession:
      return "Unknown session";
    }
  return "Unknown error";
}

auto
RequestId::parse (const Json &value) -> std::optional<RequestId>
{
  if (value.is_string () || value.is_number ())
    return RequestId{ value };
  return std::nullopt;
}

auto
JsonRpcError::to_json () const -> Json
{
  Json json = Json::object ();
  json["code"] = static_cast<std::int32_t> (code);
  json["message"] = message;
  if (!data.is_null ())
    json["data"] = data;
  return json;
}

auto
error_response (
  std::optional<RequestId> id,
  const JsonRpcErrorCode   code,
  std::string              message,
  Json                     data) -> JsonRpcErrorResponse
{
  return JsonRpcErrorResponse{
    std::move (id), JsonRpcError{ code, std::move (message), std::move (data) }
  };
}

auto
JsonRpcErrorResponse::to_json () const -> Json
{
  Json json = Json::object ();
  json["jsonrpc"] = "2.0";
  if (id.has_value ())
    json["id"] = id->value ();
  else
    json["id"] = nullptr;
  json["error"] = error.to_json ();
  return json;
}

auto
parse_json (const std::string_view text)
  -> std::expected<Json, JsonRpcErrorResponse>
{
  const Json parsed = Json::parse (text.begin (), text.end (), nullptr, false);
  if (parsed.is_discarded ())
    {
      return std::unexpected (error_response (
        std::nullopt, JsonRpcErrorCode::ParseError, "Invalid JSON"));
    }
  return parsed;
}

auto
parse_json_rpc_message (const Json &message)
  -> std::expected<JsonRpcMessage, JsonRpcErrorResponse>
{
  const auto invalid = [&message] (std::string reason) {
    return std::unexpected (error_response (
      request_id_of (message), JsonRpcErrorCode::InvalidRequest,
      std::move (reason)));
  };
  if (!message.is_object ())
    {
      return invalid (
        message.is_array ()
          ? "Batch requests are not supported"
          : "The message must be a JSON object");
    }
  const auto jsonrpc = message.find ("jsonrpc");
  if (jsonrpc == message.end () || *jsonrpc != "2.0")
    return invalid (R"(The jsonrpc member must be "2.0")");
  if (message.contains ("result") || message.contains ("error"))
    {
      if (message.contains ("method"))
        return invalid ("A message has a method or a result or an error");
      return JsonRpcMessage{ ClientResponse{} };
    }
  const auto method = message.find ("method");
  if (
    method == message.end () || !method->is_string ()
    || method->get_ref<const std::string &> ().empty ())
    return invalid ("The method member must be a non-empty string");
  const auto params_it = message.find ("params");
  if (params_it != message.end () && !params_it->is_structured ())
    return invalid ("The params member must be an object or an array");
  const auto id_it = message.find ("id");
  const auto id =
    id_it == message.end () ? std::nullopt : RequestId::parse (*id_it);
  if (id_it != message.end () && !id.has_value ())
    return invalid ("The id member must be a string or a number");
  // The result below moves the params, so they are not const.
  auto params =
    params_it == message.end () ? std::nullopt : std::optional<Json>{ *params_it };
  if (id_it == message.end ())
    {
      return JsonRpcMessage{
        JsonRpcNotification{ method->get<std::string> (), std::move (params) }
      };
    }
  return JsonRpcMessage{
    JsonRpcRequest{ *id, method->get<std::string> (), std::move (params) }
  };
}

auto
result_response (const RequestId &id, const Json &result) -> Json
{
  Json json = Json::object ();
  json["jsonrpc"] = "2.0";
  json["id"] = id.value ();
  json["result"] = result;
  return json;
}

auto
notification_response (const std::string_view method, std::optional<Json> params)
  -> Json
{
  Json json = Json::object ();
  json["jsonrpc"] = "2.0";
  json["method"] = method;
  if (params.has_value ())
    json["params"] = std::move (*params);
  return json;
}

} // namespace zrythm::mcp
