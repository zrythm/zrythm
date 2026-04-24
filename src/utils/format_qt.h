// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/qt.h"

#include <QObject>
#include <QString>
#include <QUuid>
#include <QVariant>

#include <fmt/format.h>

// Formatter for QPointer
template <typename T>
struct fmt::formatter<QPointer<T>> : fmt::formatter<std::string_view>
{
  auto format (const QPointer<T> &opt, fmt::format_context &ctx) const
    -> format_context::iterator
  {
    if (!opt.isNull ())
      {
        return fmt::formatter<std::string_view>::format (
          fmt::format ("{}", *opt), ctx);
      }
    return fmt::formatter<std::string_view>::format ("(null)", ctx);
  }
};

// Formatter for utils::QObjectUniquePtr
template <typename T>
struct fmt::formatter<zrythm::utils::QObjectUniquePtr<T>>
    : fmt::formatter<std::string_view>
{
  auto format (
    const zrythm::utils::QObjectUniquePtr<T> &opt,
    fmt::format_context &ctx) const -> format_context::iterator
  {
    if (opt)
      {
        return fmt::formatter<std::string_view>::format (
          fmt::format ("{}", *opt), ctx);
      }
    return fmt::formatter<std::string_view>::format ("(null)", ctx);
  }
};

// Formatter for QString
template <> struct fmt::formatter<QString> : fmt::formatter<std::string_view>
{
  auto format (const QString &s, fmt::format_context &ctx) const
    -> format_context::iterator
  {
    return fmt::formatter<std::string_view>::format (s.toUtf8 (), ctx);
  }
};
static_assert (fmt::formattable<QString>);

// Formatter for QStringList
template <>
struct fmt::formatter<QStringList> : fmt::formatter<std::string_view>
{
  auto format (const QStringList &s, fmt::format_context &ctx) const
    -> format_context::iterator
  {
    return fmt::formatter<std::string_view>::format (
      s.join (QString::fromUtf8 (", ")).toUtf8 (), ctx);
  }
};
static_assert (fmt::formattable<QStringList>);

template <typename... Args>
QString
format_qstr (const QString &format, Args &&... args)
{
  return QString::fromUtf8 (
    fmt::vformat (
      std::string_view (format.toUtf8 ()), fmt::make_format_args (args...)));
}

// Formatter for QVariant
template <> struct fmt::formatter<QVariant> : fmt::formatter<std::string_view>
{
  auto format (const QVariant &v, fmt::format_context &ctx) const
    -> format_context::iterator
  {
    return fmt::formatter<std::string_view>::format (
      v.toString ().toUtf8 (), ctx);
  }
};
static_assert (fmt::formattable<QVariant>);

// Formatter for QUuid
inline auto
format_as (const QUuid &uuid)
{
  return uuid.toString (QUuid::WithoutBraces);
}
static_assert (fmt::formattable<QUuid>);
