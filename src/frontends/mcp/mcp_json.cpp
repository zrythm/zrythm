// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <cstdint>

#include "frontends/mcp/mcp_json.h"
#include "utils/utf8_string.h"

#include <QJsonArray>
#include <QJsonObject>

namespace zrythm::frontends::mcp
{

QJsonValue
qjson_from_nlohmann (const nlohmann::json &value)
{
  switch (value.type ())
    {
    case nlohmann::json::value_t::null:
      return QJsonValue (QJsonValue::Null);
    case nlohmann::json::value_t::boolean:
      return QJsonValue (value.get<bool> ());
    case nlohmann::json::value_t::number_integer:
      return QJsonValue (static_cast<double> (value.get<std::int64_t> ()));
    case nlohmann::json::value_t::number_unsigned:
      return QJsonValue (static_cast<double> (value.get<std::uint64_t> ()));
    case nlohmann::json::value_t::number_float:
      return QJsonValue (value.get<double> ());
    case nlohmann::json::value_t::string:
      return QJsonValue (
        utils::Utf8String::from_utf8_encoded_string (value.get<std::string> ())
          .to_qstring ());
    case nlohmann::json::value_t::array:
      {
        QJsonArray array;
        for (const auto &item : value)
          {
            array.append (qjson_from_nlohmann (item));
          }
        return array;
      }
    case nlohmann::json::value_t::object:
      {
        QJsonObject object;
        for (auto it = value.begin (); it != value.end (); ++it)
          {
            object.insert (
              utils::Utf8String::from_utf8_encoded_string (it.key ())
                .to_qstring (),
              qjson_from_nlohmann (it.value ()));
          }
        return object;
      }
    case nlohmann::json::value_t::binary:
    case nlohmann::json::value_t::discarded:
      return QJsonValue (QJsonValue::Null);
    }
  return QJsonValue (QJsonValue::Undefined);
}

nlohmann::json
nlohmann_from_qjson (const QJsonValue &value)
{
  switch (value.type ())
    {
    case QJsonValue::Null:
    case QJsonValue::Undefined:
      return nlohmann::json ();
    case QJsonValue::Bool:
      return nlohmann::json (value.toBool ());
    case QJsonValue::Double:
      {
        const auto number = value.toDouble ();
        // Narrow to a 64-bit integer only when the value is exactly
        // representable; larger integral doubles stay doubles and are
        // rejected by the operations' parameter validation.
        if (
          number == std::floor (number)
          && std::abs (number) <= kMaxExactJsonInteger)
          {
            return number >= 0
                     ? nlohmann::json (static_cast<std::uint64_t> (number))
                     : nlohmann::json (static_cast<std::int64_t> (number));
          }
        return nlohmann::json (number);
      }
    case QJsonValue::String:
      return nlohmann::json (
        utils::Utf8String::from_qstring (value.toString ()).str ());
    case QJsonValue::Array:
      {
        auto array = nlohmann::json::array ();
        for (const auto &item : value.toArray ())
          {
            array.push_back (nlohmann_from_qjson (item));
          }
        return array;
      }
    case QJsonValue::Object:
      {
        auto       object = nlohmann::json::object ();
        const auto q_object = value.toObject ();
        for (auto it = q_object.begin (); it != q_object.end (); ++it)
          {
            object[utils::Utf8String::from_qstring (it.key ()).str ()] =
              nlohmann_from_qjson (it.value ());
          }
        return object;
      }
    }
  return nlohmann::json ();
}

} // namespace zrythm::frontends::mcp
