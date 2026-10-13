// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cmath>
#include <expected>
#include <functional>
#include <limits>
#include <optional>
#include <variant>
#include <vector>

#include "utils/utf8_string.h"

#include <QObject>
#include <QVariant>
#include <QVariantMap>

namespace zrythm::structure::arrangement
{

/// A parameter value in its generic form, as produced by option parsing.
using ParamValue = std::variant<int, double, bool>;

/// The value range and widget kind of a function parameter.
struct ParamSpec
{
  enum class Kind
  {
    Integer,
    Double,
    Boolean,
    Choice,
  };

  /// Which control the dialog renders and how values are validated.
  Kind kind = Kind::Integer;

  /// Inclusive bounds for Integer and Double parameters.
  double minimum = 0;
  double maximum = 0;

  /// Value increment the dialog's slider snaps to; 0 derives it from
  /// the range.
  double step = 0;

  /// For Choice parameters: {display, value} maps, matching the combo box
  /// contract of the generic function dialog.
  QVariantList choices;

  ParamValue default_value{};
};

inline std::expected<ParamValue, utils::Utf8String>
validate_param_value (
  const utils::Utf8String &id,
  const ParamSpec         &spec,
  const QVariant          &value)
{
  const auto integer_in_range = [&] (double number) -> std::optional<int> {
    if (
      number == std::floor (number) && number >= spec.minimum
      && number <= spec.maximum)
      return static_cast<int> (number);
    return std::nullopt;
  };

  switch (spec.kind)
    {
    case ParamSpec::Kind::Integer:
      {
        // JSON round-trips numbers as doubles, so integers may arrive as
        // integral doubles
        bool         ok = false;
        const double number = value.toDouble (&ok);
        const auto   integer = ok ? integer_in_range (number) : std::nullopt;
        if (!integer)
          return std::unexpected (
            utils::Utf8String::from_qstring (
              QObject::tr ("%1 must be an integer between %2 and %3")
                .arg (id.to_qstring ())
                .arg (spec.minimum)
                .arg (spec.maximum)));
        return *integer;
      }
    case ParamSpec::Kind::Double:
      {
        bool         ok = false;
        const double number = value.toDouble (&ok);
        if (!ok || number < spec.minimum || number > spec.maximum)
          return std::unexpected (
            utils::Utf8String::from_qstring (
              QObject::tr ("%1 must be between %2 and %3")
                .arg (id.to_qstring ())
                .arg (spec.minimum)
                .arg (spec.maximum)));
        return number;
      }
    case ParamSpec::Kind::Boolean:
      if (value.typeId () != QMetaType::Type::Bool)
        return std::unexpected (
          utils::Utf8String::from_qstring (
            QObject::tr ("%1 must be true or false").arg (id.to_qstring ())));
      return value.toBool ();
    case ParamSpec::Kind::Choice:
      {
        bool         ok = false;
        const double number = value.toDouble (&ok);
        // The choices list is the value domain: any integer listed there is
        // accepted, within the range of representable ints
        if (
          ok && number == std::floor (number)
          && number >= static_cast<double> (std::numeric_limits<int>::min ())
          && number <= static_cast<double> (std::numeric_limits<int>::max ()))
          {
            const int integer = static_cast<int> (number);
            for (const auto &choice : spec.choices)
              {
                if (choice.toMap ().value (QStringLiteral ("value")) == integer)
                  return integer;
              }
          }
        return std::unexpected (
          utils::Utf8String::from_qstring (
            QObject::tr ("%1 has an invalid value").arg (id.to_qstring ())));
      }
    }
  return std::unexpected (
    utils::Utf8String::from_qstring (
      QObject::tr ("%1 has an invalid value").arg (id.to_qstring ())));
}

/**
 * @brief The declaration of one parameter of an editor function kind.
 *
 * A parameter's declaration drives everything that needs to know about
 * it: the generic function dialog renders a control from the spec, and
 * option parsing validates and assigns through @ref set, so the
 * parameter is declared exactly once.
 *
 * @tparam FunctionT The function kind the parameter belongs to; its
 * instance is the parameter set.
 */
template <typename FunctionT> struct EditorFunctionParam
{
  utils::Utf8String id;
  utils::Utf8String label;
  ParamSpec         spec;

  /// Assigns a validated value to the matching field of @p function.
  std::function<void (FunctionT &, ParamValue)> set;
};

/**
 * @brief Applies the options passed for a function kind to its instance.
 *
 * Every entry of @p options must match a parameter of @p params by id and
 * lie within the parameter's declared range (or, for Choice parameters,
 * be one of the declared values); anything else is rejected with an
 * error message. Options not present keep their defaults.
 *
 * @param options Values by parameter id.
 * @param function The kind instance receiving the values.
 * @param params The kind's parameter declarations.
 * @return Nothing on success, or an error message.
 */
template <typename FunctionT>
[[nodiscard]] std::expected<void, utils::Utf8String>
apply_options (
  const QVariantMap                                 &options,
  FunctionT                                         &function,
  const std::vector<EditorFunctionParam<FunctionT>> &params)
{
  for (auto it = options.begin (); it != options.end (); ++it)
    {
      const utils::Utf8String key = utils::Utf8String::from_qstring (it.key ());
      const auto              param_it = std::ranges::find_if (
        params, [&key] (const EditorFunctionParam<FunctionT> &param) {
          return param.id == key;
        });
      if (param_it == params.end ())
        {
          return std::unexpected (
            utils::Utf8String::from_qstring (
              QObject::tr ("Unknown parameter %1").arg (it.key ())));
        }
      const auto validated =
        validate_param_value (param_it->id, param_it->spec, it.value ());
      if (!validated)
        return std::unexpected (validated.error ());
      param_it->set (function, *validated);
    }
  return {};
}

} // namespace zrythm::structure::arrangement
