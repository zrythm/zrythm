// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <mcpserver/json_rpc.h>
#include <mcpserver/mcp_types.h>
#include <nlohmann/json.hpp>

namespace zrythm::mcp
{

/** Names of the members of the `_meta` objects that this server reads or
 * writes. */
inline constexpr std::string_view kProtocolVersionMetaKey =
  "io.modelcontextprotocol/protocolVersion";
inline constexpr std::string_view kClientCapabilitiesMetaKey =
  "io.modelcontextprotocol/clientCapabilities";
inline constexpr std::string_view kClientInfoMetaKey =
  "io.modelcontextprotocol/clientInfo";
inline constexpr std::string_view kServerInfoMetaKey =
  "io.modelcontextprotocol/serverInfo";
inline constexpr std::string_view kSubscriptionIdMetaKey =
  "io.modelcontextprotocol/subscriptionId";

/** The `_meta` object of a request of the stateless revision. */
struct RequestMeta
{
  /** The revision that the client put in the request. It need not be supported.
   */
  std::string protocol_version;
  /** The capabilities that the client declares for the request. */
  Json                          client_capabilities;
  std::optional<Implementation> client_info;

  /** Returns an error message when a required member is missing or malformed. */
  [[nodiscard]] static auto
  from_json (const Json &meta) -> std::expected<RequestMeta, std::string>;
};

/** The params of a method that takes none. An object is accepted and ignored. */
struct EmptyParams
{
  [[nodiscard]] static auto
  from_json (const Json &params) -> std::expected<EmptyParams, std::string>;
};

/** The protocol revisions in which a method exists. */
enum class MethodAvailability : std::uint8_t
{
  /** The method exists in the session-based revision only. */
  SessionBased,
  /** The method exists in the stateless revision only. */
  Stateless,
  /** The method exists in both revisions. */
  Both,
};

/** The capability that a method needs from the server. */
enum class CapabilityRequirement : std::uint8_t
{
  /** The method needs no capability. */
  Nothing,
  /** The method needs at least one tool. */
  Tools,
  /** The method needs at least one resource provider. */
  Resources,
};

/** `initialize`: starts a session. */
struct Initialize
{
  static constexpr std::string_view   method_name = "initialize";
  static constexpr MethodAvailability availability =
    MethodAvailability::SessionBased;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Nothing;

  struct Params
  {
    std::string    protocol_version;
    Json           capabilities;
    Implementation client_info;

    [[nodiscard]] static auto
    from_json (const Json &params) -> std::expected<Params, std::string>;
  };

  struct Result
  {
    ProtocolRevision           protocol_version;
    ServerCapabilities         capabilities;
    Implementation             server_info;
    std::optional<std::string> instructions;

    [[nodiscard]] auto to_json () const -> Json;
  };

  Params params;
};

/** `ping` */
struct Ping
{
  static constexpr std::string_view   method_name = "ping";
  static constexpr MethodAvailability availability =
    MethodAvailability::SessionBased;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Nothing;
  using Params = EmptyParams;

  Params params;
};

/** `tools/list` */
struct ToolsList
{
  static constexpr std::string_view   method_name = "tools/list";
  static constexpr MethodAvailability availability = MethodAvailability::Both;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Tools;

  struct Params
  {
    [[nodiscard]] static auto
    from_json (const Json &params) -> std::expected<Params, std::string>;
  };

  struct Result
  {
    std::vector<Tool> tools;

    [[nodiscard]] auto to_json () const -> Json;
  };

  Params params;
};

/** `tools/call` */
struct ToolsCall
{
  static constexpr std::string_view   method_name = "tools/call";
  static constexpr MethodAvailability availability = MethodAvailability::Both;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Tools;
  using Params = ToolCall;
  using Result = CallToolResult;

  Params params;
};

/** `resources/list` */
struct ResourcesList
{
  static constexpr std::string_view   method_name = "resources/list";
  static constexpr MethodAvailability availability = MethodAvailability::Both;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Resources;
  using Params = EmptyParams;

  struct Result
  {
    std::vector<Resource> resources;

    [[nodiscard]] auto to_json () const -> Json;
  };

  Params params;
};

/** `resources/templates/list`: the server serves no resource templates. */
struct ResourcesTemplatesList
{
  static constexpr std::string_view   method_name = "resources/templates/list";
  static constexpr MethodAvailability availability = MethodAvailability::Both;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Resources;
  using Params = EmptyParams;

  struct Result
  {
    [[nodiscard]] auto to_json () const -> Json
    {
      return Json::object (
        {
          { "resourceTemplates", Json::array () }
      });
    }
  };

  Params params;
};

/** `resources/read` */
struct ResourcesRead
{
  static constexpr std::string_view   method_name = "resources/read";
  static constexpr MethodAvailability availability = MethodAvailability::Both;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Resources;

  struct Params
  {
    ResourceUri uri;

    [[nodiscard]] static auto
    from_json (const Json &params) -> std::expected<Params, std::string>;
  };

  struct Result
  {
    ResourceContents contents;

    [[nodiscard]] auto to_json () const -> Json;
  };

  Params params;
};

/** `resources/subscribe` */
struct ResourcesSubscribe
{
  static constexpr std::string_view   method_name = "resources/subscribe";
  static constexpr MethodAvailability availability =
    MethodAvailability::SessionBased;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Resources;

  struct Params
  {
    ResourceUri uri;

    [[nodiscard]] static auto
    from_json (const Json &params) -> std::expected<Params, std::string>;
  };

  Params params;
};

/** `resources/unsubscribe` */
struct ResourcesUnsubscribe
{
  static constexpr std::string_view   method_name = "resources/unsubscribe";
  static constexpr MethodAvailability availability =
    MethodAvailability::SessionBased;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Resources;

  struct Params
  {
    ResourceUri uri;

    [[nodiscard]] static auto
    from_json (const Json &params) -> std::expected<Params, std::string>;
  };

  Params params;
};

/** `subscriptions/listen`: opens a stream that carries notifications. */
struct SubscriptionsListen
{
  static constexpr std::string_view   method_name = "subscriptions/listen";
  static constexpr MethodAvailability availability =
    MethodAvailability::Stateless;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Nothing;

  struct Params
  {
    SubscriptionFilter notifications;

    [[nodiscard]] static auto
    from_json (const Json &params) -> std::expected<Params, std::string>;
  };

  Params params;
};

/** `server/discover` */
struct ServerDiscover
{
  static constexpr std::string_view   method_name = "server/discover";
  static constexpr MethodAvailability availability =
    MethodAvailability::Stateless;
  static constexpr CapabilityRequirement requirement =
    CapabilityRequirement::Nothing;
  using Params = EmptyParams;

  struct Result
  {
    ServerCapabilities         capabilities;
    std::optional<std::string> instructions;

    [[nodiscard]] auto to_json () const -> Json;
  };

  Params params;
};

/** A request of the client with validated params. */
using AnyRequest = std::variant<
  Initialize,
  Ping,
  ToolsList,
  ToolsCall,
  ResourcesList,
  ResourcesTemplatesList,
  ResourcesRead,
  ResourcesSubscribe,
  ResourcesUnsubscribe,
  SubscriptionsListen,
  ServerDiscover>;

/** Why a request could not be dispatched. */
struct DispatchError
{
  JsonRpcErrorCode code;
  std::string      message;
};

/**
 * @brief Validates the method name and the params of a request.
 *
 * A method that does not exist in @p revision gives MethodNotFound. Params
 * that do not satisfy the method give InvalidParams.
 */
[[nodiscard]] auto
parse_request (
  ProtocolRevision revision,
  std::string_view method,
  const std::optional<Json> &params) -> std::expected<AnyRequest, DispatchError>;

/** Method names of the notifications that the server sends. */
inline constexpr std::string_view kSubscriptionsAcknowledgedMethod =
  "notifications/subscriptions/acknowledged";
inline constexpr std::string_view kResourcesUpdatedMethod =
  "notifications/resources/updated";
inline constexpr std::string_view kResourcesListChangedMethod =
  "notifications/resources/list_changed";

/** The first message of a listen stream, for the request @p listen_id. */
[[nodiscard]] auto
subscriptions_acknowledged_notification (
  const RequestId          &listen_id,
  const SubscriptionFilter &honored) -> Json;

/**
 * @brief `notifications/resources/updated`.
 *
 * The notification carries the id of the request @p listen_id when it goes
 * to a listen stream.
 */
[[nodiscard]] auto
resources_updated_notification (
  const ResourceUri              &uri,
  const std::optional<RequestId> &listen_id) -> Json;

/**
 * @brief `notifications/resources/list_changed`.
 *
 * The params carry the id of the request @p listen_id when the notification
 * goes to a listen stream.
 */
[[nodiscard]] auto
resources_list_changed_notification (const std::optional<RequestId> &listen_id)
  -> Json;

/** The result that ends the listen stream of the request @p listen_id. */
[[nodiscard]] auto
subscriptions_listen_closed_result (const RequestId &listen_id) -> Json;

} // namespace zrythm::mcp
