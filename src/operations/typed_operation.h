// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "operations/operation.h"
#include "operations/operation_context.h"
#include "operations/param_types.h"
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace zrythm::operations::detail
{

/**
 * Extracts the parameter types of a lambda or other callable with one
 * (non-generic) operator().
 *
 * This is the single seam between a typed handler signature and its
 * parameter metadata; nothing outside this namespace needs to know how
 * signatures are inspected.
 */
template <typename T> struct member_function_traits;

template <typename C, typename R, typename... A>
struct member_function_traits<R (C::*) (A...) const>
{
  using args = std::tuple<A...>;
};

template <typename T>
using callable_args_t = typename member_function_traits<
  decltype (&std::remove_cvref_t<T>::operator())>::args;

} // namespace zrythm::operations::detail

namespace zrythm::operations
{

/**
 * An operation declaration: a typed handler plus its metadata.
 *
 * The handler's signature is the parameter schema. It takes an
 * OperationContext followed by one value per parameter spec, in the same
 * order, and each parameter type must satisfy ParamType. A mismatch
 * between the specs and the signature is a compile error.
 *
 * Handlers return an OperationReport built from the context (applied or
 * refused); the dispatcher fills in pushed commands and affected objects.
 */
template <typename Handler, std::size_t NumParams> class TypedOperation
{
public:
  using Args = detail::callable_args_t<Handler>;

  static_assert (
    std::tuple_size_v<Args> == NumParams + 1,
    "the handler must take OperationContext& followed by one value per "
    "parameter spec");
  static_assert (
    std::is_same_v<std::tuple_element_t<0, Args>, OperationContext &>,
    "the handler's first parameter must be OperationContext&");

  template <std::size_t Index>
  using param_at = std::tuple_element_t<Index + 1, Args>;

  static_assert (
    []<std::size_t... I> (std::index_sequence<I...>) {
      return (ParamType<param_at<I>> && ...);
    }(std::make_index_sequence<NumParams>{}),
    "every handler parameter after the context must be a parameter type "
    "(ParamType)");

  TypedOperation (
    OperationKind                    kind,
    std::string_view                 name,
    std::string_view                 summary,
    std::array<ParamSpec, NumParams> specs,
    Handler                          handler)
      : kind_ (kind), name_ (std::string (name)),
        summary_ (std::string (summary)), specs_ (specs),
        handler_ (std::move (handler))
  {
  }

  OperationKind      kind () const { return kind_; }
  const std::string &name () const { return name_; }
  const std::string &summary () const { return summary_; }

  /**
   * Validates the arguments and runs the handler.
   *
   * Parameter validation failures are refused before the handler runs.
   */
  OperationReport
  invoke (const nlohmann::json &args, OperationEnvironment &env) const
  {
    OperationContext ctx (env);
    OperationReport  report =
      invoke_with (args, ctx, std::make_index_sequence<NumParams>{});
    report.command_texts = ctx.take_command_texts ();
    report.affected_objects = ctx.take_affected_objects ();
    if (
      kind_ == OperationKind::Edit && report.status == OperationStatus::Applied)
      {
        report.revision = env.next_revision ();
      }
    return report;
  }

  /**
   * The MCP-style tool definition derived from this declaration.
   */
  nlohmann::json make_tool_definition () const
  {
    nlohmann::json properties = nlohmann::json::object ();
    nlohmann::json required = nlohmann::json::array ();
    add_param_schemas (
      std::make_index_sequence<NumParams>{}, properties, required);
    return {
      { "name",        name_    },
      { "description", summary_ },
      { "inputSchema",
       {
          { "type", "object" },
          { "properties", std::move (properties) },
          { "required", std::move (required) },
        }                       },
    };
  }

private:
  template <std::size_t... I>
  void add_param_schemas (
    std::index_sequence<I...>,
    nlohmann::json &properties,
    nlohmann::json &required) const
  {
    (
      [&] {
        nlohmann::json schema = param_at<I>::describe ();
        schema["description"] = std::string (specs_[I].description);
        properties[std::string (specs_[I].name)] = std::move (schema);
        if (param_at<I>::is_required ())
          {
            required.push_back (std::string (specs_[I].name));
          }
      }(),
      ...);
  }

  template <typename Param>
  static std::expected<Param, Refusal> parse_one (
    const nlohmann::json       &args,
    const ParamSpec            &spec,
    const OperationEnvironment &env,
    std::optional<Refusal>     &failure)
  {
    if (failure.has_value ())
      {
        return std::unexpected (*failure);
      }
    const nlohmann::json value =
      args.contains (spec.name) ? args.at (spec.name) : nlohmann::json{};
    auto parsed = Param::parse (value, env);
    if (!parsed.has_value ())
      {
        failure = Refusal{
          fmt::format ("parameter '{}': {}", spec.name, parsed.error ().reason)
        };
        return std::unexpected (*failure);
      }
    return parsed;
  }

  template <std::size_t... I>
  OperationReport invoke_with (
    const nlohmann::json &args,
    OperationContext     &ctx,
    std::index_sequence<I...>) const
  {
    std::optional<Refusal> failure = std::nullopt;
    auto                   parsed = std::tuple{
      parse_one<param_at<I>> (args, specs_[I], ctx.environment (), failure)...
    };
    if (failure.has_value ())
      {
        return ctx.refused (std::move (failure->reason));
      }
    return std::apply (
      handler_,
      std::tuple_cat (
        std::forward_as_tuple (ctx),
        std::tuple{ std::move (*std::get<I> (parsed))... }));
  }

  OperationKind                    kind_;
  std::string                      name_;
  std::string                      summary_;
  std::array<ParamSpec, NumParams> specs_;
  Handler                          handler_;
};

/**
 * Deduction guide so lambda handlers deduce their type and spec count.
 */
template <typename Handler, std::size_t NumParams>
TypedOperation (
  OperationKind,
  std::string_view,
  std::string_view,
  std::array<ParamSpec, NumParams>,
  Handler) -> TypedOperation<Handler, NumParams>;

} // namespace zrythm::operations
