// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <span>
#include <utility>

#include <mcpserver/mcp_types.h>

using namespace std::string_view_literals;

namespace zrythm::mcp
{

namespace
{

/** Returns true when any object inside @p json has the member @p key. */
[[nodiscard]] auto
has_member_deep (const Json &json, const std::string_view key) -> bool
{
  const auto child_has_it = [key] (const Json &child) {
    return has_member_deep (child, key);
  };
  if (json.is_object ())
    return json.contains (key) || std::ranges::any_of (json, child_has_it);
  if (json.is_array ())
    return std::ranges::any_of (json, child_has_it);
  return false;
}

/**
 * @brief Checks the members of an object.
 *
 * Returns an error when @p json is not an object or has a member outside
 * @p known_members.
 */
[[nodiscard]] auto
check_members (
  const Json                             &json,
  const std::string_view                  what,
  const std::span<const std::string_view> known_members)
  -> std::expected<void, std::string>
{
  if (!json.is_object ())
    return std::unexpected (std::string (what) + " must be a JSON object");
  for (const auto &item : json.items ())
    {
      if (!std::ranges::contains (known_members, item.key ()))
        {
          return std::unexpected (
            std::string (what) + " has the unknown member '" + item.key ()
            + "'");
        }
    }
  return {};
}

/** Returns the error of the first argument that holds one. */
template <typename... Results>
[[nodiscard]] auto
first_error (const Results &... results) -> std::optional<std::string>
{
  std::optional<std::string> error = std::nullopt;
  const auto                 take = [&error] (const auto &result) {
    if (!result.has_value () && !error.has_value ())
      error = result.error ();
  };
  (take (results), ...);
  return error;
}

/** Returns the error that says that the member @p key has the wrong type. */
[[nodiscard]] auto
wrong_type (const std::string_view key) -> std::string
{
  return "The member '" + std::string (key) + "' has the wrong type";
}

/** Reads the member @p key of @p json as a string. Returns nullopt when the
 * member is absent or null, and an error when it has another type. */
[[nodiscard]] auto
optional_string (const Json &json, const std::string_view key)
  -> std::expected<std::optional<std::string>, std::string>
{
  const auto it = json.find (key);
  if (it == json.end () || it->is_null ())
    return std::nullopt;
  if (!it->is_string ())
    return std::unexpected (wrong_type (key));
  return it->get<std::string> ();
}

/** Reads the member @p key of @p json as a boolean, with the rules of
 * optional_string(). */
[[nodiscard]] auto
optional_bool (const Json &json, const std::string_view key)
  -> std::expected<std::optional<bool>, std::string>
{
  const auto it = json.find (key);
  if (it == json.end () || it->is_null ())
    return std::nullopt;
  if (!it->is_boolean ())
    return std::unexpected (wrong_type (key));
  return it->get<bool> ();
}

/**
 * @brief Reads the member @p key of @p json with @p parse.
 *
 * Returns nullopt when the member is absent. The error carries the name of
 * the member.
 */
template <typename T>
auto
parse_member (const Json &json, const std::string_view key, auto &&parse)
  -> std::expected<std::optional<T>, std::string>
{
  const auto member = json.find (key);
  if (member == json.end ())
    return std::nullopt;
  auto parsed = parse (*member);
  if (!parsed.has_value ())
    {
      return std::unexpected (
        std::string (key) + ": " + std::move (parsed).error ());
    }
  return std::move (*parsed);
}

/** Reads the member "resourceSubscriptions" of @p json. The member is
 * absent, null, or an array of non-empty strings. */
[[nodiscard]] auto
optional_resource_uris (const Json &json)
  -> std::expected<std::optional<std::vector<ResourceUri>>, std::string>
{
  const auto subscriptions = json.find ("resourceSubscriptions");
  if (subscriptions == json.end () || subscriptions->is_null ())
    return std::nullopt;
  if (!subscriptions->is_array ())
    return std::unexpected (wrong_type ("resourceSubscriptions"));
  std::vector<ResourceUri> uris;
  for (const auto &entry : *subscriptions)
    {
      if (!entry.is_string ())
        {
          return std::unexpected (
            "Every entry of 'resourceSubscriptions' must be a string");
        }
      auto uri = ResourceUri::parse (entry.get_ref<const std::string &> ());
      if (!uri.has_value ())
        {
          return std::unexpected (
            "Every entry of 'resourceSubscriptions' must be a non-empty "
            "string");
        }
      uris.push_back (std::move (*uri));
    }
  return uris;
}

} // namespace

auto
to_string (const ProtocolRevision revision) noexcept -> std::string_view
{
  switch (revision)
    {
    case ProtocolRevision::V2025_11_25:
      return "2025-11-25";
    case ProtocolRevision::V2026_07_28:
      return "2026-07-28";
    }
  return "";
}

auto
parse_protocol_revision (const std::string_view text)
  -> std::optional<ProtocolRevision>
{
  for (const auto revision : kSupportedRevisions)
    {
      if (to_string (revision) == text)
        return revision;
    }
  return std::nullopt;
}

auto
supported_versions () -> Json
{
  Json versions = Json::array ();
  for (const auto revision : kSupportedRevisions)
    versions.push_back (std::string (to_string (revision)));
  return versions;
}

auto
policy_for (const ProtocolRevision revision) -> const RevisionPolicy &
{
  static const RevisionPolicy session_based{
    JsonRpcErrorCode::ResourceNotFound
  };
  static const RevisionPolicy stateless{ JsonRpcErrorCode::InvalidParams };
  return revision == ProtocolRevision::V2026_07_28 ? stateless : session_based;
}

auto
Implementation::from_json (const Json &json)
  -> std::expected<Implementation, std::string>
{
  if (!json.is_object ())
    return std::unexpected ("the implementation must be a JSON object");
  const auto name = json.find ("name");
  const auto version = json.find ("version");
  if (
    name == json.end () || !name->is_string ()
    || name->get_ref<const std::string &> ().empty ())
    {
      return std::unexpected ("name must be a non-empty string");
    }
  if (
    version == json.end () || !version->is_string ()
    || version->get_ref<const std::string &> ().empty ())
    {
      return std::unexpected ("version must be a non-empty string");
    }
  return Implementation{
    name->get<std::string> (), version->get<std::string> ()
  };
}

auto
Implementation::to_json () const -> Json
{
  return Json::object (
    {
      { "name",    name    },
      { "version", version }
  });
}

auto
ToolsCapability::to_json () const -> Json
{
  Json json = Json::object ();
  if (list_changed)
    json["listChanged"] = true;
  return json;
}

auto
ResourcesCapability::to_json () const -> Json
{
  Json json = Json::object ();
  if (subscribe)
    json["subscribe"] = true;
  if (list_changed)
    json["listChanged"] = true;
  return json;
}

auto
ServerCapabilities::to_json () const -> Json
{
  Json json = Json::object ();
  if (tools.has_value ())
    json["tools"] = tools->to_json ();
  if (resources.has_value ())
    json["resources"] = resources->to_json ();
  return json;
}

auto
ToolAnnotations::from_json (const Json &json)
  -> std::expected<ToolAnnotations, std::string>
{
  if (
    auto result = check_members (
      json, "annotations",
      std::array{
        "title"sv, "readOnlyHint"sv, "destructiveHint"sv, "idempotentHint"sv,
        "openWorldHint"sv });
    !result.has_value ())
    {
      return std::unexpected (std::move (result).error ());
    }
  auto title = optional_string (json, "title");
  auto read_only = optional_bool (json, "readOnlyHint");
  auto destructive = optional_bool (json, "destructiveHint");
  auto idempotent = optional_bool (json, "idempotentHint");
  auto open_world = optional_bool (json, "openWorldHint");
  if (
    auto error =
      first_error (title, read_only, destructive, idempotent, open_world);
    error.has_value ())
    {
      return std::unexpected (std::move (*error));
    }
  return ToolAnnotations{
    std::move (*title), std::move (*read_only), std::move (*destructive),
    std::move (*idempotent), std::move (*open_world)
  };
}

auto
ToolAnnotations::to_json () const -> Json
{
  Json json = Json::object ();
  if (title.has_value ())
    json["title"] = *title;
  if (read_only_hint.has_value ())
    json["readOnlyHint"] = *read_only_hint;
  if (destructive_hint.has_value ())
    json["destructiveHint"] = *destructive_hint;
  if (idempotent_hint.has_value ())
    json["idempotentHint"] = *idempotent_hint;
  if (open_world_hint.has_value ())
    json["openWorldHint"] = *open_world_hint;
  return json;
}

auto
InputSchema::parse (const Json &json) -> std::expected<InputSchema, std::string>
{
  const auto type = json.find ("type");
  if (!json.is_object () || type == json.end () || *type != "object")
    {
      return std::unexpected (
        R"(the schema must be an object whose type is "object")");
    }
  if (has_member_deep (json, "x-mcp-header"))
    {
      return std::unexpected (
        "the schema must not use the x-mcp-header annotation");
    }
  return InputSchema{ json };
}

auto
Tool::from_json (const Json &definition) -> std::expected<Tool, std::string>
{
  if (
    auto result = check_members (
      definition, "the tool definition",
      std::array{
        "name"sv, "title"sv, "description"sv, "inputSchema"sv, "outputSchema"sv,
        "annotations"sv });
    !result.has_value ())
    {
      return std::unexpected (std::move (result).error ());
    }
  const auto name = definition.find ("name");
  if (
    name == definition.end () || !name->is_string ()
    || name->get_ref<const std::string &> ().empty ())
    {
      return std::unexpected ("the tool name must be a non-empty string");
    }
  if (!definition.contains ("inputSchema"))
    return std::unexpected ("the tool definition needs an inputSchema");
  auto input_schema = InputSchema::parse (definition.at ("inputSchema"));
  if (!input_schema.has_value ())
    return std::unexpected ("inputSchema: " + std::move (input_schema).error ());
  auto output_schema =
    parse_member<InputSchema> (definition, "outputSchema", InputSchema::parse);
  auto annotations = parse_member<ToolAnnotations> (
    definition, "annotations", ToolAnnotations::from_json);
  auto title = optional_string (definition, "title");
  auto description = optional_string (definition, "description");
  if (
    auto error = first_error (output_schema, annotations, title, description);
    error.has_value ())
    return std::unexpected (std::move (*error));
  // A tool without an annotations member gives no hints.
  const auto tool_annotations =
    annotations->has_value () ? std::move (**annotations) : ToolAnnotations{};
  return Tool{
    name->get<std::string> (),  std::move (*title),
    std::move (*description),   std::move (*input_schema),
    std::move (*output_schema), std::move (tool_annotations)
  };
}

auto
Tool::to_json () const -> Json
{
  Json json = Json::object ();
  json["name"] = name_;
  if (title_.has_value ())
    json["title"] = *title_;
  if (description_.has_value ())
    json["description"] = *description_;
  json["inputSchema"] = input_schema_.json ();
  if (output_schema_.has_value ())
    json["outputSchema"] = output_schema_->json ();
  const Json annotations = annotations_.to_json ();
  if (!annotations.empty ())
    json["annotations"] = annotations;
  return json;
}

auto
TextContent::to_json () const -> Json
{
  return Json::object (
    {
      { "type", "text" },
      { "text", text   }
  });
}

auto
CallToolResult::text (std::string text) -> CallToolResult
{
  return CallToolResult{ { TextContent{ std::move (text) } }, false };
}

auto
CallToolResult::error (std::string message) -> CallToolResult
{
  return CallToolResult{ { TextContent{ std::move (message) } }, true };
}

auto
CallToolResult::to_json () const -> Json
{
  Json json = Json::object ();
  Json blocks = Json::array ();
  for (const auto &block : content)
    blocks.push_back (
      std::visit ([] (const auto &typed) { return typed.to_json (); }, block));
  json["content"] = std::move (blocks);
  if (is_error)
    json["isError"] = true;
  return json;
}

auto
ToolCall::from_json (const Json &params) -> std::expected<ToolCall, std::string>
{
  const auto name = params.find ("name");
  if (
    name == params.end () || !name->is_string ()
    || name->get_ref<const std::string &> ().empty ())
    {
      return std::unexpected ("params.name must be a non-empty string");
    }
  const auto arguments_it = params.find ("arguments");
  if (arguments_it != params.end () && !arguments_it->is_object ())
    return std::unexpected ("params.arguments must be an object");
  auto arguments =
    arguments_it == params.end ()
      ? std::nullopt
      : std::optional<Json>{ *arguments_it };
  return ToolCall{ name->get<std::string> (), std::move (arguments) };
}

auto
Resource::to_json () const -> Json
{
  Json json = Json::object (
    {
      { "uri",  uri.str () },
      { "name", name       }
  });
  if (description.has_value ())
    json["description"] = *description;
  if (mime_type.has_value ())
    json["mimeType"] = *mime_type;
  if (size.has_value ())
    json["size"] = *size;
  return json;
}

auto
TextResourceContents::to_json () const -> Json
{
  Json json = Json::object (
    {
      { "uri",  uri.str () },
      { "text", text       }
  });
  if (mime_type.has_value ())
    json["mimeType"] = *mime_type;
  return json;
}

auto
BlobResourceContents::to_json () const -> Json
{
  Json json = Json::object (
    {
      { "uri",  uri.str () },
      { "blob", blob       }
  });
  if (mime_type.has_value ())
    json["mimeType"] = *mime_type;
  return json;
}

auto
to_json (const ResourceContents &contents) -> Json
{
  return std::visit (
    [] (const auto &typed) { return typed.to_json (); }, contents);
}

auto
SubscriptionFilter::from_json (const Json &json)
  -> std::expected<SubscriptionFilter, std::string>
{
  if (
    auto result = check_members (
      json, "the subscription filter",
      std::array{
        "toolsListChanged"sv, "promptsListChanged"sv, "resourcesListChanged"sv,
        "resourceSubscriptions"sv });
    !result.has_value ())
    {
      return std::unexpected (std::move (result).error ());
    }
  auto tools_list_changed = optional_bool (json, "toolsListChanged");
  auto prompts_list_changed = optional_bool (json, "promptsListChanged");
  auto resources_list_changed = optional_bool (json, "resourcesListChanged");
  auto resource_subscriptions = optional_resource_uris (json);
  if (
    auto error = first_error (
      tools_list_changed, prompts_list_changed, resources_list_changed,
      resource_subscriptions);
    error.has_value ())
    {
      return std::unexpected (std::move (*error));
    }
  return SubscriptionFilter{
    std::move (*tools_list_changed), std::move (*prompts_list_changed),
    std::move (*resources_list_changed), std::move (*resource_subscriptions)
  };
}

auto
SubscriptionFilter::to_json () const -> Json
{
  Json json = Json::object ();
  if (tools_list_changed.has_value ())
    json["toolsListChanged"] = *tools_list_changed;
  if (prompts_list_changed.has_value ())
    json["promptsListChanged"] = *prompts_list_changed;
  if (resources_list_changed.has_value ())
    json["resourcesListChanged"] = *resources_list_changed;
  if (resource_subscriptions.has_value ())
    {
      Json uris = Json::array ();
      for (const auto &uri : *resource_subscriptions)
        uris.push_back (uri.str ());
      json["resourceSubscriptions"] = std::move (uris);
    }
  return json;
}

} // namespace zrythm::mcp
