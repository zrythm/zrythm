// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <utility>

#include "utils/refusal.h"

#include <QByteArray>

#include "operations/operation.h"
#include "operations/operation_environment.h"
#include "operations/param_types.h"
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace zrythm::operations
{

/** Version of the pagination cursor payload. */
inline constexpr int cursor_payload_version = 1;

namespace detail
{

/** Decodes an opaque cursor into its validated payload. */
inline std::expected<nlohmann::json, Refusal>
decode_cursor (std::string_view cursor)
{
  const auto result = QByteArray::fromBase64Encoding (
    QByteArray (cursor.data (), static_cast<qsizetype> (cursor.size ())),
    QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
  if (result.decodingStatus != QByteArray::Base64DecodingStatus::Ok)
    {
      return std::unexpected (
        Refusal{
          utils::RefusalKind::InvalidParameter,
          "cursor is not valid base64url" });
    }
  const std::string text (
    result.decoded.constData (),
    static_cast<std::size_t> (result.decoded.size ()));
  const auto payload = nlohmann::json::parse (text, nullptr, false);
  const auto valid =
    !payload.is_discarded () && payload.is_object () && payload.contains ("v")
    && payload.contains ("k") && payload.contains ("u")
    && payload.contains ("r") && payload.at ("v") == cursor_payload_version
    && payload.at ("k").is_number_unsigned () && payload.at ("u").is_string ()
    && payload.at ("r").is_number_unsigned ();
  if (!valid)
    {
      return std::unexpected (
        Refusal{
          utils::RefusalKind::InvalidParameter,
          "cursor is not a valid pagination cursor" });
    }
  return payload;
}

/** Encodes a cursor as an opaque base64url string. */
inline std::string
encode_cursor (std::uint64_t key, std::string_view uuid, std::uint64_t revision)
{
  const auto payload =
    nlohmann::json{
      { "v", cursor_payload_version },
      { "k", key                    },
      { "u", uuid                   },
      { "r", revision               },
  }
      .dump ();
  const auto encoded =
    QByteArray (payload.data (), static_cast<qsizetype> (payload.size ()))
      .toBase64 (QByteArray::Base64UrlEncoding);
  return { encoded.constData (), static_cast<std::size_t> (encoded.size ()) };
}

} // namespace detail

/**
 * An opaque pagination cursor for list queries.
 *
 * An empty string (the default) means no cursor: the first page. Any
 * other value must be a cursor minted by a previous response; values
 * that do not decode or that exceed maximum_length are refused.
 */
class CursorParam
{
public:
  using ValueType = std::string;

  /**
   * Cursors minted by responses stay well under this bound; longer
   * input is not one of ours and is refused before decoding.
   */
  static constexpr std::size_t maximum_length = 256;

  static constexpr bool is_required () { return false; }

  static nlohmann::json describe ()
  {
    return {
      { "type",      "string"       },
      { "maxLength", maximum_length },
      { "default",   ""             }
    };
  }

  static std::expected<CursorParam, Refusal>
  parse (const nlohmann::json &value, const OperationEnvironment &)
  {
    if (value.is_null ())
      {
        return CursorParam{};
      }
    if (!value.is_string ())
      {
        return std::unexpected (
          Refusal{
            utils::RefusalKind::InvalidParameter, "cursor must be a string" });
      }
    return from_value (value.get<std::string> ());
  }

  static std::expected<CursorParam, Refusal> from_value (std::string value)
  {
    if (value.empty ())
      {
        return CursorParam{};
      }
    if (value.size () > maximum_length)
      {
        return std::unexpected (
          Refusal{
            utils::RefusalKind::InvalidParameter,
            fmt::format ("cursor must be at most {} bytes", maximum_length) });
      }
    auto payload = detail::decode_cursor (value);
    if (!payload)
      {
        return std::unexpected (payload.error ());
      }
    CursorParam cursor;
    cursor.present_ = true;
    cursor.key_ = payload->at ("k").get<std::uint64_t> ();
    cursor.uuid_ = payload->at ("u").get<std::string> ();
    cursor.revision_ = payload->at ("r").get<std::uint64_t> ();
    return cursor;
  }

  /** Whether a cursor was supplied. */
  bool present () const { return present_; }

  /** Sort key of the last entry served by the page that minted it. */
  std::uint64_t key () const { return key_; }

  /** UUID of the last entry served by the page that minted it. */
  const std::string &uuid () const { return uuid_; }

  /** Project revision at the time the cursor was minted. */
  std::uint64_t revision () const { return revision_; }

private:
  CursorParam () = default;

  bool          present_ = false;
  std::uint64_t key_ = 0;
  std::string   uuid_;
  std::uint64_t revision_ = 0;
};

/**
 * Builds one page of a list query response.
 *
 * The handler feeds the query's entries in stable sort order via
 * consider(); finish() returns the response envelope with items,
 * next_cursor (null after the last page), total_count and
 * revision_changed.
 *
 * A page resumes strictly after the cursor's (sort key, UUID) pair.
 * Entries inserted before the cursor position can be served twice and
 * entries removed can be skipped, so pagination is best-effort under
 * concurrent modification: the cursor records the revision it was
 * minted at, revision_changed reports whether the project revision
 * moved since, and a client that needs a consistent view restarts
 * from the first page.
 */
class PageBuilder
{
public:
  PageBuilder (LimitParam limit, const CursorParam &cursor, std::uint64_t revision)
      : limit_ (static_cast<std::size_t> (limit.value ())), cursor_ (cursor),
        revision_ (revision)
  {
  }

  /**
   * Considers one entry, invoking @p make_item only when the entry is
   * appended to the page.
   */
  template <typename ItemFactory>
  void
  consider (std::uint64_t key, const std::string &uuid, ItemFactory make_item)
  {
    if (cursor_.present () && !is_after_cursor (key, uuid))
      {
        return;
      }
    ++eligible_;
    if (items_.size () >= limit_)
      {
        return;
      }
    items_.push_back (make_item ());
    last_key_ = key;
    last_uuid_ = uuid;
  }

  /** Builds the response envelope for the accumulated page. */
  nlohmann::json finish (std::uint64_t total_count)
  {
    const bool more = eligible_ > items_.size ();
    return nlohmann::json{
      { "items", std::move (items_) },
      { "next_cursor",
       more ? nlohmann::json (
                 detail::encode_cursor (last_key_, last_uuid_, revision_))
             : nlohmann::json (nullptr) },
      { "total_count", total_count },
      { "revision_changed",
       cursor_.present () && cursor_.revision () != revision_ },
    };
  }

private:
  bool is_after_cursor (std::uint64_t key, const std::string &uuid) const
  {
    return key > cursor_.key ()
           || (key == cursor_.key () && uuid > cursor_.uuid ());
  }

  std::size_t    limit_;
  CursorParam    cursor_;
  std::uint64_t  revision_;
  std::size_t    eligible_ = 0;
  nlohmann::json items_ = nlohmann::json::array ();
  std::uint64_t  last_key_ = 0;
  std::string    last_uuid_;
};

} // namespace zrythm::operations
