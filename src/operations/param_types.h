// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "structure/tracks/track.h"
#include "utils/color.h"
#include "utils/enum_utils.h"
#include "utils/utf8_string.h"

#include <QStringDecoder>
#include <QUuid>

#include "operations/operation.h"
#include "operations/operation_environment.h"
#include <nlohmann/json.hpp>
#include <type_safe/strong_typedef.hpp>

namespace zrythm::structure::tracks
{
class Track;
}

namespace zrythm::operations
{

/**
 * A parameter type usable in an operation handler signature.
 *
 * A parameter type describes itself as a JSON Schema fragment (describe),
 * states whether callers must supply it (is_required), turns a JSON value
 * into its own value form (parse), and validates a value given directly
 * by C++ callers (from_value). Both entry points refuse input they
 * cannot accept; the JSON value is null when the caller did not supply
 * the parameter.
 */
template <typename P>
concept ParamType =
  requires (const nlohmann::json &value, const OperationEnvironment &env) {
    { P::is_required () } -> std::convertible_to<bool>;
    { P::describe () } -> std::same_as<nlohmann::json>;
    { P::parse (value, env) } -> std::same_as<std::expected<P, Refusal>>;
    {
      P::from_value (std::declval<typename P::ValueType> ())
    } -> std::same_as<std::expected<P, Refusal>>;
  };

/**
 * Returns whether the given bytes are well-formed UTF-8.
 *
 * Utf8String accepts unchecked input, so strings from callers must be
 * validated before entering one.
 */
inline bool
is_valid_utf8 (std::string_view bytes)
{
  QStringDecoder decoder (
    QStringConverter::Utf8, QStringConverter::Flag::Stateless);
  const QString decoded = decoder.decode (
    QByteArray (bytes.data (), static_cast<qsizetype> (bytes.size ())));
  Q_UNUSED (decoded);
  return !decoder.hasError ();
}

/**
 * A required UTF-8 string parameter with length bounds (measured in bytes).
 */
template <std::size_t MinLength, std::size_t MaxLength> class StringParam
{
public:
  using ValueType = utils::Utf8String;

  static constexpr bool is_required () { return true; }

  static nlohmann::json describe ()
  {
    return {
      { "type",      "string"  },
      { "minLength", MinLength },
      { "maxLength", MaxLength },
    };
  }

  static std::expected<StringParam, Refusal>
  parse (const nlohmann::json &value, const OperationEnvironment &)
  {
    if (value.is_null ())
      {
        return std::unexpected (Refusal{ "this parameter is required" });
      }
    if (!value.is_string ())
      {
        return std::unexpected (Refusal{ "expected a string" });
      }
    const auto str = value.get<std::string> ();
    if (!is_valid_utf8 (str))
      {
        return std::unexpected (Refusal{ "not valid UTF-8" });
      }
    return from_value (utils::Utf8String::from_utf8_encoded_string (str));
  }

  /** Validates a value directly, with the same bounds as parse(). */
  static std::expected<StringParam, Refusal>
  from_value (utils::Utf8String value)
  {
    const auto length = value.view ().size ();
    if (length < MinLength || length > MaxLength)
      {
        return std::unexpected (
          Refusal{ fmt::format (
            "length must be between {} and {} bytes", MinLength, MaxLength) });
      }
    return StringParam{ std::move (value) };
  }

  const utils::Utf8String &value () const { return value_; }

private:
  explicit StringParam (utils::Utf8String value)
      : value_ (std::move (value)) { }
  utils::Utf8String value_;
};

/**
 * A required color parameter given as a hex string ("#rrggbb" or
 * "#rrggbbaa").
 */
class HexColorParam
{
public:
  using ValueType = utils::Color;

  static constexpr bool is_required () { return true; }

  static nlohmann::json describe ()
  {
    return {
      { "type",    "string"                             },
      { "pattern", "^#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$" },
    };
  }

  static std::expected<HexColorParam, Refusal>
  parse (const nlohmann::json &value, const OperationEnvironment &)
  {
    if (value.is_null ())
      {
        return std::unexpected (Refusal{ "this parameter is required" });
      }
    if (!value.is_string ())
      {
        return std::unexpected (
          Refusal{ "expected a color string like \"#rrggbb\"" });
      }
    const auto str = value.get<std::string> ();
    const auto digit = [] (char c) {
      return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
             || (c >= 'A' && c <= 'F');
    };
    const bool valid =
      (str.size () == 7 || str.size () == 9) && str[0] == '#'
      && std::ranges::all_of (str.substr (1), digit);
    if (!valid)
      {
        return std::unexpected (
          Refusal{ fmt::format (
            "invalid color '{}': expected \"#rrggbb\" or \"#rrggbbaa\"", str) });
      }
    return HexColorParam{ utils::Color (std::string_view{ str }) };
  }

  /** A constructed color is always valid, so this never refuses. */
  static std::expected<HexColorParam, Refusal> from_value (utils::Color value)
  {
    return HexColorParam{ value };
  }

  const utils::Color &value () const { return value_; }

private:
  explicit HexColorParam (utils::Color value) : value_ (value) { }
  utils::Color value_;
};

/**
 * A required parameter for a scoped enumeration, given as the enumerator
 * name (for example "Audio").
 */
template <typename Enum>
  requires std::is_enum_v<Enum>
class EnumParam
{
public:
  using ValueType = Enum;

  static constexpr bool is_required () { return true; }

  static nlohmann::json describe ()
  {
    auto names = nlohmann::json::array ();
    auto values = nlohmann::json::array ();
    for (auto entry : magic_enum::enum_entries<Enum> ())
      {
        names.push_back (std::string (entry.second));
        values.push_back (magic_enum::enum_integer (entry.first));
      }
    // Enums serialize as integers in this project; the names are accepted as
    // input alongside the integers.
    return {
      { "oneOf",
       nlohmann::json::array (
          { nlohmann::json{ { "type", "string" }, { "enum", std::move (names) } },
            nlohmann::json{
              { "type", "integer" }, { "enum", std::move (values) } } }) },
    };
  }

  static std::expected<EnumParam, Refusal>
  parse (const nlohmann::json &value, const OperationEnvironment &)
  {
    const auto allowed_names = [] {
      std::string joined;
      for (auto name : magic_enum::enum_names<Enum> ())
        {
          if (!joined.empty ())
            {
              joined += ", ";
            }
          joined += std::string (name);
        }
      return joined;
    }();
    if (value.is_null ())
      {
        return std::unexpected (Refusal{ "this parameter is required" });
      }
    if (value.is_string ())
      {
        const auto str = value.get<std::string> ();
        const auto parsed = magic_enum::enum_cast<Enum> (str);
        if (parsed.has_value ())
          {
            return EnumParam{ *parsed };
          }
        return std::unexpected (
          Refusal{ fmt::format ("'{}' is not one of: {}", str, allowed_names) });
      }
    if (value.is_number_integer ())
      {
        const auto number = value.get<int> ();
        const auto parsed = magic_enum::enum_cast<Enum> (number);
        if (parsed.has_value ())
          {
            return EnumParam{ *parsed };
          }
        return std::unexpected (
          Refusal{ fmt::format (
            "{} is not a valid value (allowed: {})", number, allowed_names) });
      }
    return std::unexpected (
      Refusal{ fmt::format (
        "expected a name (one of: {}) or an integer value", allowed_names) });
  }

  /** Rejects enumerator values that are not part of the enumeration. */
  static std::expected<EnumParam, Refusal> from_value (Enum value)
  {
    if (magic_enum::enum_contains (value))
      {
        return EnumParam{ value };
      }
    return std::unexpected (Refusal{ "not one of this enumeration's values" });
  }

  Enum value () const { return value_; }

private:
  explicit EnumParam (Enum value) : value_ (value) { }
  Enum value_;
};

/**
 * An optional pagination limit for list queries.
 *
 * Defaults to 1000 entries when the caller does not supply it.
 */
class LimitParam
{
public:
  using ValueType = int;

  static constexpr int  default_value = 1000;
  static constexpr int  minimum = 1;
  static constexpr int  maximum = 5000;
  static constexpr bool is_required () { return false; }

  static nlohmann::json describe ()
  {
    return {
      { "type",    "integer"     },
      { "minimum", minimum       },
      { "maximum", maximum       },
      { "default", default_value },
    };
  }

  static std::expected<LimitParam, Refusal>
  parse (const nlohmann::json &value, const OperationEnvironment &)
  {
    if (value.is_null ())
      {
        return LimitParam{ default_value };
      }
    if (!value.is_number_integer () && !value.is_number_unsigned ())
      {
        return std::unexpected (
          Refusal{ fmt::format (
            "expected an integer between {} and {}", minimum, maximum) });
      }
    const auto number = value.get<std::int64_t> ();
    return from_value (static_cast<int> (number));
  }

  /** Validates a value directly, with the same range as parse(). */
  static std::expected<LimitParam, Refusal> from_value (int value)
  {
    if (value < minimum || value > maximum)
      {
        return std::unexpected (
          Refusal{ fmt::format ("must be between {} and {}", minimum, maximum) });
      }
    return LimitParam{ value };
  }

  int value () const { return value_; }

private:
  explicit LimitParam (int value) : value_ (value) { }
  int value_;
};

/**
 * A required parameter identifying a track by its project UUID.
 *
 * Parses the UUID and resolves it against the project's track collection;
 * a UUID that does not name a track in the project is refused.
 */
class TrackParam
{
public:
  using ValueType = structure::tracks::Track *;

  static constexpr bool is_required () { return true; }

  static nlohmann::json describe ()
  {
    return {
      { "type",   "string" },
      { "format", "uuid"   },
    };
  }

  static std::expected<TrackParam, Refusal>
  parse (const nlohmann::json &value, const OperationEnvironment &env)
  {
    if (value.is_null ())
      {
        return std::unexpected (Refusal{ "this parameter is required" });
      }
    if (!value.is_string ())
      {
        return std::unexpected (
          Refusal{ "expected the track UUID as a string" });
      }
    const auto str = value.get<std::string> ();
    const auto uuid =
      QUuid (utils::Utf8String::from_utf8_encoded_string (str).to_qstring ());
    if (uuid.isNull ())
      {
        return std::unexpected (
          Refusal{ fmt::format ("'{}' is not a UUID", str) });
      }
    auto * track = env.find_track (uuid);
    if (track == nullptr)
      {
        return std::unexpected (
          Refusal{ fmt::format ("no track with UUID {}", str) });
      }
    return TrackParam{ track, str };
  }

  /** Returns the UUID in the string form used in reports. */
  static std::string uuid_to_string (const QUuid &uuid)
  {
    return std::string (
      utils::Utf8String::from_qstring (uuid.toString (QUuid::WithoutBraces))
        .view ());
  }

  /**
   * Takes a live track; the UUID string reported in results is derived
   * from the track itself.
   */
  static std::expected<TrackParam, Refusal>
  from_value (structure::tracks::Track * track)
  {
    if (track == nullptr)
      {
        return std::unexpected (Refusal{ "the track is required" });
      }
    return TrackParam{
      track, uuid_to_string (type_safe::get (track->get_uuid ()))
    };
  }

  structure::tracks::Track * value () const { return track_; }

  /** The UUID string exactly as the caller supplied it. */
  const std::string &uuid_string () const { return uuid_; }

private:
  explicit TrackParam (structure::tracks::Track * track, std::string uuid)
      : track_ (track), uuid_ (std::move (uuid))
  {
  }
  structure::tracks::Track * track_;
  std::string                uuid_;
};

} // namespace zrythm::operations
