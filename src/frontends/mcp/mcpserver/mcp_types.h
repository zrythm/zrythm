// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <mcpserver/json_rpc.h>
#include <nlohmann/json.hpp>

namespace zrythm::mcp
{

/** The protocol revisions that the server speaks. */
enum class ProtocolRevision : std::uint8_t
{
  /** Sessions that start with an initialize handshake. */
  V2025_11_25,
  /** Single requests that carry the Mcp-Method and Mcp-Name headers. */
  V2026_07_28,
};

[[nodiscard]] auto
to_string (ProtocolRevision revision) noexcept -> std::string_view;

/** Returns nullopt when @p text is not a supported revision. */
[[nodiscard]] auto
parse_protocol_revision (std::string_view text)
  -> std::optional<ProtocolRevision>;

/** The supported revisions, newest first. */
inline constexpr std::array kSupportedRevisions{
  ProtocolRevision::V2026_07_28, ProtocolRevision::V2025_11_25
};

/** The supported revisions as a JSON array of strings, newest first. */
[[nodiscard]] auto
supported_versions () -> Json;

/** The rules that differ between revisions. */
struct RevisionPolicy
{
  /** Error code for a read of a URI that no provider serves. */
  JsonRpcErrorCode resource_not_found_code{ JsonRpcErrorCode::ResourceNotFound };
};

/** Returns the policy of @p revision. */
[[nodiscard]] auto
policy_for (ProtocolRevision revision) -> const RevisionPolicy &;

/** Name and version of a client or of the server. */
struct Implementation
{
  std::string name;
  std::string version;

  /** Returns an error when @p json lacks a non-empty name or version. */
  [[nodiscard]] static auto
  from_json (const Json &json) -> std::expected<Implementation, std::string>;
  [[nodiscard]] auto to_json () const -> Json;
};

struct ToolsCapability
{
  /** True when the server sends notifications/tools/list_changed. */
  bool list_changed{ false };

  [[nodiscard]] auto to_json () const -> Json;
};

struct ResourcesCapability
{
  /** True when a session can subscribe to a resource. */
  bool subscribe{ false };
  /** True when the server sends notifications/resources/list_changed. */
  bool list_changed{ false };

  [[nodiscard]] auto to_json () const -> Json;
};

/** What the server can do in one protocol revision. */
struct ServerCapabilities
{
  std::optional<ToolsCapability>     tools;
  std::optional<ResourcesCapability> resources;

  [[nodiscard]] auto to_json () const -> Json;
};

/** Hints about what a tool does. Every member is optional. */
struct ToolAnnotations
{
  std::optional<std::string> title;
  /** True when the tool changes no state. */
  std::optional<bool> read_only_hint;
  std::optional<bool> destructive_hint;
  std::optional<bool> idempotent_hint;
  std::optional<bool> open_world_hint;

  /** Returns an error when a member has the wrong type or is unknown. */
  [[nodiscard]] static auto
  from_json (const Json &json) -> std::expected<ToolAnnotations, std::string>;
  [[nodiscard]] auto to_json () const -> Json;
};

/** A JSON schema whose root is an object. */
class InputSchema
{
public:
  /**
   * @brief Validates an object schema.
   *
   * Returns an error when the root has no type "object" or when any property
   * uses the x-mcp-header annotation.
   */
  [[nodiscard]] static auto
  parse (const Json &json) -> std::expected<InputSchema, std::string>;

  [[nodiscard]] auto json () const -> const Json & { return json_; }

private:
  explicit InputSchema (Json json) : json_ (std::move (json)) { }
  Json json_;
};

/** A tool definition that the server validated. */
class Tool
{
public:
  /**
   * @brief Validates a tool definition.
   *
   * Returns an error when a member has the wrong type or is unknown, when the
   * name is empty, or when a schema is not an object schema.
   */
  [[nodiscard]] static auto
  from_json (const Json &definition) -> std::expected<Tool, std::string>;

  [[nodiscard]] auto name () const -> const std::string & { return name_; }
  [[nodiscard]] auto to_json () const -> Json;

private:
  Tool (
    std::string                name,
    std::optional<std::string> title,
    std::optional<std::string> description,
    InputSchema                input_schema,
    std::optional<InputSchema> output_schema,
    ToolAnnotations            annotations)
      : name_ (std::move (name)), title_ (std::move (title)),
        description_ (std::move (description)),
        input_schema_ (std::move (input_schema)),
        output_schema_ (std::move (output_schema)),
        annotations_ (std::move (annotations))
  {
  }

  std::string                name_;
  std::optional<std::string> title_;
  std::optional<std::string> description_;
  InputSchema                input_schema_;
  std::optional<InputSchema> output_schema_;
  ToolAnnotations            annotations_;
};

struct TextContent
{
  std::string text;

  [[nodiscard]] auto to_json () const -> Json;
};

/** One block of the content of a tool result. */
using ContentBlock = std::variant<TextContent>;

/** The outcome of a tool call. */
struct CallToolResult
{
  std::vector<ContentBlock> content;
  /** True when the tool failed. The failure is then described by the content. */
  bool is_error{ false };

  /** A successful result with one text block. */
  [[nodiscard]] static auto text (std::string text) -> CallToolResult;
  /** A failed call, described by one text block. */
  [[nodiscard]] static auto error (std::string message) -> CallToolResult;

  [[nodiscard]] auto to_json () const -> Json;
};

/** The tool to call and the arguments to pass to it. */
struct ToolCall
{
  std::string         name;
  std::optional<Json> arguments;

  /** Reads the params object of a tools/call request. */
  [[nodiscard]] static auto
  from_json (const Json &params) -> std::expected<ToolCall, std::string>;
};

/** The URI of a resource. A URI is never empty. */
class ResourceUri
{
public:
  /** Returns nullopt when @p uri is empty. */
  [[nodiscard]] static auto
  parse (const std::string_view uri) -> std::optional<ResourceUri>
  {
    if (uri.empty ())
      return std::nullopt;
    return ResourceUri (std::string (uri));
  }

  [[nodiscard]] auto str () const -> const std::string & { return uri_; }
  auto operator== (const ResourceUri &other) const -> bool = default;
  auto operator<=> (const ResourceUri &other) const = default;

private:
  explicit ResourceUri (std::string uri) : uri_ (std::move (uri)) { }
  std::string uri_;
};

/** One resource of a provider. */
struct Resource
{
  ResourceUri                uri;
  std::string                name;
  std::optional<std::string> description;
  std::optional<std::string> mime_type;
  /** Size of the content in bytes, when it is known. */
  std::optional<std::uint64_t> size;

  [[nodiscard]] auto to_json () const -> Json;
};

struct TextResourceContents
{
  ResourceUri                uri;
  std::string                text;
  std::optional<std::string> mime_type;

  [[nodiscard]] auto to_json () const -> Json;
};

struct BlobResourceContents
{
  ResourceUri uri;
  /** The content, encoded as base64. */
  std::string                blob;
  std::optional<std::string> mime_type;

  [[nodiscard]] auto to_json () const -> Json;
};

using ResourceContents =
  std::variant<TextResourceContents, BlobResourceContents>;

[[nodiscard]] auto
to_json (const ResourceContents &contents) -> Json;

/** The notifications that a client asks for on a listen stream. */
struct SubscriptionFilter
{
  std::optional<bool>                     tools_list_changed;
  std::optional<bool>                     prompts_list_changed;
  std::optional<bool>                     resources_list_changed;
  std::optional<std::vector<ResourceUri>> resource_subscriptions;

  /** Returns an error when a member has the wrong type or is unknown. */
  [[nodiscard]] static auto
  from_json (const Json &json) -> std::expected<SubscriptionFilter, std::string>;
  [[nodiscard]] auto to_json () const -> Json;
};

} // namespace zrythm::mcp
