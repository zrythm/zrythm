// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QUrl>
#include <QtNetwork/QHttpHeaders>

namespace zrythm::mcp
{

/** The HTTP methods that the server tells apart. */
enum class HttpMethod : std::uint8_t
{
  Get,
  Post,
  Delete,
  /** A method that the server does not serve. */
  Other,
};

[[nodiscard]] constexpr auto
to_string (HttpMethod method) noexcept -> std::string_view
{
  switch (method)
    {
    case HttpMethod::Get:
      return "GET";
    case HttpMethod::Post:
      return "POST";
    case HttpMethod::Delete:
      return "DELETE";
    case HttpMethod::Other:
      break;
    }
  return "";
}

/** The parts of one HTTP request that the server works with. */
class HttpRequest
{
public:
  HttpRequest (HttpMethod method, QUrl url, QHttpHeaders headers, QByteArray body)
      : method_ (method), url_ (std::move (url)),
        headers_ (std::move (headers)), body_ (std::move (body))
  {
  }

  [[nodiscard]] auto method () const -> HttpMethod { return method_; }

  /** The path of the request, without the query. */
  [[nodiscard]] auto path () const -> QString { return url_.path (); }

  /**
   * @brief Returns the first value of the header @p name.
   *
   * The name is compared without case. The value is empty when the request
   * has no such header.
   */
  [[nodiscard]] auto header (const QByteArrayView name) const -> QByteArray
  {
    return headers_.value (name).toByteArray ();
  }

  /**
   * @brief Returns the values of the header @p name as one list.
   *
   * A request may send a header in several lines. The values are joined
   * with commas, as the HTTP standard allows for a list of values.
   */
  [[nodiscard]] auto
  combined_header (const QByteArrayView name) const -> QByteArray
  {
    return headers_.combinedValue (name);
  }

  [[nodiscard]] auto body () const -> const QByteArray & { return body_; }

private:
  HttpMethod   method_;
  QUrl         url_;
  QHttpHeaders headers_;
  QByteArray   body_;
};

} // namespace zrythm::mcp
