// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <type_traits>
#include <utility>

#include <fmt/format.h>
#include <mcpserver/mcp_methods.h>

namespace zrythm::mcp
{

namespace
{

/** Returns true when a method with @p availability exists in @p revision. */
[[nodiscard]] constexpr auto
exists_in (
  const MethodAvailability availability,
  const ProtocolRevision   revision) -> bool
{
  switch (availability)
    {
    case MethodAvailability::SessionBased:
      return revision == ProtocolRevision::V2025_11_25;
    case MethodAvailability::Stateless:
      return revision == ProtocolRevision::V2026_07_28;
    case MethodAvailability::Both:
      return true;
    }
  return false;
}

/**
 * @brief Parses the params of a request into the Params type @p Params.
 *
 * The params object is empty when the request has no params. Returns an
 * error when the params are not an object.
 */
template <typename Params>
[[nodiscard]] auto
parse_params (const std::optional<Json> &params)
  -> std::expected<Params, DispatchError>
{
  static const Json empty = Json::object ();
  const Json       &object = params.has_value () ? *params : empty;
  if (!object.is_object ())
    {
      return std::unexpected (
        DispatchError{
          JsonRpcErrorCode::InvalidParams, "params must be an object" });
    }
  auto parsed = Params::from_json (object);
  if (!parsed.has_value ())
    {
      return std::unexpected (
        DispatchError{
          JsonRpcErrorCode::InvalidParams, std::move (parsed).error () });
    }
  return std::move (*parsed);
}

/** Parses the params of a request for the method @p Method. */
template <typename Method>
[[nodiscard]] auto
make_request (const std::optional<Json> &params)
  -> std::expected<AnyRequest, DispatchError>
{
  auto parsed = parse_params<typename Method::Params> (params);
  if (!parsed.has_value ())
    return std::unexpected (std::move (parsed).error ());
  return AnyRequest{ Method{ std::move (*parsed) } };
}

using Factory = std::expected<AnyRequest, DispatchError> (*) (
  const std::optional<Json> &);

/** One method of the client: its name, the revisions it exists in, and the
 * function that parses the params of a request. */
struct MethodEntry
{
  std::string_view   name;
  MethodAvailability availability;
  Factory            parse;
};

template <typename Method>
constexpr MethodEntry method_entry{
  Method::method_name, Method::availability, &make_request<Method>
};

/** One entry per alternative of @p Variant, in the order of the alternatives. */
template <typename... Methods>
consteval auto
method_table (std::type_identity<std::variant<Methods...>>)
  -> std::array<MethodEntry, sizeof...(Methods)>
{
  return { method_entry<Methods>... };
}

/** Every method of the client, derived from the alternatives of AnyRequest. */
constexpr auto kMethods = method_table (std::type_identity<AnyRequest>{});

/** The names of the methods in alphabetical order. */
constexpr auto kSortedMethodNames = [] {
  std::array<std::string_view, kMethods.size ()> names;
  std::ranges::transform (kMethods, names.begin (), &MethodEntry::name);
  std::ranges::sort (names);
  return names;
}();

static_assert (
  std::ranges::adjacent_find (kSortedMethodNames) == kSortedMethodNames.end (),
  "Two methods have the same name");

/** Returns an error when @p params has a non-empty cursor member. */
[[nodiscard]] auto
check_no_cursor (const Json &params) -> std::expected<void, std::string>
{
  const auto cursor = params.find ("cursor");
  if (cursor == params.end () || cursor->is_null ())
    return {};
  if (!cursor->is_string ())
    return std::unexpected ("params.cursor must be a string");
  if (!cursor->get_ref<const std::string &> ().empty ())
    {
      return std::unexpected (
        "The server returns no pages, so params.cursor must be empty");
    }
  return {};
}

/** Reads the member "uri" of @p params. */
[[nodiscard]] auto
read_uri (const Json &params) -> std::expected<ResourceUri, std::string>
{
  const auto uri = params.find ("uri");
  if (uri == params.end () || !uri->is_string ())
    return std::unexpected ("params.uri must be a string");
  auto parsed = ResourceUri::parse (uri->get_ref<const std::string &> ());
  if (!parsed.has_value ())
    return std::unexpected ("params.uri must be a non-empty string");
  return *parsed;
}

/** The `_meta` object that marks a message as part of the stream of @p
 * listen_id. */
[[nodiscard]] auto
subscription_meta (const RequestId &listen_id) -> Json
{
  return Json::object (
    {
      { kSubscriptionIdMetaKey, listen_id.value () }
  });
}

} // namespace

auto
RequestMeta::from_json (const Json &meta)
  -> std::expected<RequestMeta, std::string>
{
  if (!meta.is_object ())
    return std::unexpected ("params._meta must be an object");
  const auto version = meta.find (kProtocolVersionMetaKey);
  if (version == meta.end () || !version->is_string ())
    {
      return std::unexpected (
        fmt::format (
          "params._meta must have the member '{}' with the protocol revision",
          kProtocolVersionMetaKey));
    }
  const auto capabilities = meta.find (kClientCapabilitiesMetaKey);
  if (capabilities == meta.end () || !capabilities->is_object ())
    {
      return std::unexpected (
        fmt::format (
          "params._meta must have the member '{}' with an object",
          kClientCapabilitiesMetaKey));
    }
  // The client info is absent, null, or an object with a name and a version.
  auto client_info =
    [&] () -> std::expected<std::optional<Implementation>, std::string> {
    const auto info = meta.find (kClientInfoMetaKey);
    if (info == meta.end () || info->is_null ())
      return std::nullopt;
    auto parsed = Implementation::from_json (*info);
    if (!parsed.has_value ())
      {
        return std::unexpected (
          fmt::format (
            "params._meta '{}': {}", kClientInfoMetaKey, parsed.error ()));
      }
    return std::move (*parsed);
  }();
  if (!client_info.has_value ())
    return std::unexpected (std::move (client_info).error ());
  return RequestMeta{
    version->get<std::string> (), *capabilities, std::move (*client_info)
  };
}

auto
EmptyParams::from_json (const Json &params)
  -> std::expected<EmptyParams, std::string>
{
  (void) params;
  return EmptyParams{};
}

auto
Initialize::Params::from_json (const Json &params)
  -> std::expected<Initialize::Params, std::string>
{
  const auto version = params.find ("protocolVersion");
  if (version == params.end () || !version->is_string ())
    return std::unexpected ("params.protocolVersion must be a string");
  const auto capabilities = params.find ("capabilities");
  if (capabilities == params.end () || !capabilities->is_object ())
    return std::unexpected ("params.capabilities must be an object");
  if (!params.contains ("clientInfo"))
    return std::unexpected ("params.clientInfo is required");
  auto client_info = Implementation::from_json (params.at ("clientInfo"));
  if (!client_info.has_value ())
    {
      return std::unexpected ("params.clientInfo: " + client_info.error ());
    }
  return Params{
    version->get<std::string> (), *capabilities, std::move (*client_info)
  };
}

auto
Initialize::Result::to_json () const -> Json
{
  Json json = Json::object (
    {
      { "protocolVersion",
       std::string (zrythm::mcp::to_string (protocol_version)) },
      { "capabilities",    capabilities.to_json ()             },
      { "serverInfo",      server_info.to_json ()              },
  });
  if (instructions.has_value ())
    json["instructions"] = *instructions;
  return json;
}

auto
ToolsList::Params::from_json (const Json &params)
  -> std::expected<ToolsList::Params, std::string>
{
  if (auto result = check_no_cursor (params); !result.has_value ())
    return std::unexpected (std::move (result).error ());
  return Params{};
}

auto
ToolsList::Result::to_json () const -> Json
{
  Json tools_json = Json::array ();
  for (const auto &tool : tools)
    tools_json.push_back (tool.to_json ());
  return Json::object (
    {
      { "tools", std::move (tools_json) }
  });
}

auto
ResourcesList::Result::to_json () const -> Json
{
  Json resources_json = Json::array ();
  for (const auto &resource : resources)
    resources_json.push_back (resource.to_json ());
  return Json::object (
    {
      { "resources", std::move (resources_json) }
  });
}

auto
ResourcesRead::Params::from_json (const Json &params)
  -> std::expected<ResourcesRead::Params, std::string>
{
  auto uri = read_uri (params);
  if (!uri.has_value ())
    return std::unexpected (std::move (uri).error ());
  return Params{ std::move (*uri) };
}

auto
ResourcesRead::Result::to_json () const -> Json
{
  return Json::object (
    {
      { "contents", Json::array ({ zrythm::mcp::to_json (contents) }) }
  });
}

auto
ResourcesSubscribe::Params::from_json (const Json &params)
  -> std::expected<ResourcesSubscribe::Params, std::string>
{
  auto uri = read_uri (params);
  if (!uri.has_value ())
    return std::unexpected (std::move (uri).error ());
  return Params{ std::move (*uri) };
}

auto
ResourcesUnsubscribe::Params::from_json (const Json &params)
  -> std::expected<ResourcesUnsubscribe::Params, std::string>
{
  auto uri = read_uri (params);
  if (!uri.has_value ())
    return std::unexpected (std::move (uri).error ());
  return Params{ std::move (*uri) };
}

auto
SubscriptionsListen::Params::from_json (const Json &params)
  -> std::expected<SubscriptionsListen::Params, std::string>
{
  const auto filter = params.find ("notifications");
  if (filter == params.end () || !filter->is_object ())
    return std::unexpected ("params.notifications must be an object");
  auto parsed = SubscriptionFilter::from_json (*filter);
  if (!parsed.has_value ())
    return std::unexpected ("params.notifications: " + parsed.error ());
  return Params{ std::move (*parsed) };
}

auto
ServerDiscover::Result::to_json () const -> Json
{
  Json json = Json::object (
    {
      { "supportedVersions", supported_versions ()   },
      { "capabilities",      capabilities.to_json () },
  });
  if (instructions.has_value ())
    json["instructions"] = *instructions;
  return json;
}

auto
parse_request (
  const ProtocolRevision     revision,
  const std::string_view     method,
  const std::optional<Json> &params) -> std::expected<AnyRequest, DispatchError>
{
  const auto entry = std::ranges::find (kMethods, method, &MethodEntry::name);
  if (entry == kMethods.end () || !exists_in (entry->availability, revision))
    {
      return std::unexpected (
        DispatchError{
          JsonRpcErrorCode::MethodNotFound,
          "Unknown method: " + std::string (method) });
    }
  return entry->parse (params);
}

auto
subscriptions_acknowledged_notification (
  const RequestId          &listen_id,
  const SubscriptionFilter &honored) -> Json
{
  Json params = Json::object ();
  params["_meta"] = subscription_meta (listen_id);
  params["notifications"] = honored.to_json ();
  return notification_response (
    kSubscriptionsAcknowledgedMethod, std::move (params));
}

auto
resources_updated_notification (
  const ResourceUri              &uri,
  const std::optional<RequestId> &listen_id) -> Json
{
  Json params = Json::object ();
  params["uri"] = uri.str ();
  if (listen_id.has_value ())
    params["_meta"] = subscription_meta (*listen_id);
  return notification_response (kResourcesUpdatedMethod, std::move (params));
}

auto
resources_list_changed_notification (const std::optional<RequestId> &listen_id)
  -> Json
{
  if (!listen_id.has_value ())
    return notification_response (kResourcesListChangedMethod);
  return notification_response (
    kResourcesListChangedMethod,
    Json::object (
      {
        { "_meta", subscription_meta (*listen_id) }
  }));
}

auto
subscriptions_listen_closed_result (const RequestId &listen_id) -> Json
{
  Json result = Json::object (
    {
      { "resultType", "complete" }
  });
  result["_meta"] = subscription_meta (listen_id);
  return result;
}

} // namespace zrythm::mcp
