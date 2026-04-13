// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <boost/describe.hpp>
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#include <fmt/format.h>

// Universal formatter for all types described by BOOST_DESCRIBE_STRUCT or
// BOOST_DESCRIBE_CLASS.
template <class T>
struct fmt::formatter<
  T,
  char,
  std::enable_if_t<
    boost::describe::has_describe_bases<T>::value
    && boost::describe::has_describe_members<T>::value && !std::is_union_v<T>>>
{
  constexpr auto parse (format_parse_context &ctx)
  {
    const auto * it = ctx.begin ();
    const auto * end = ctx.end ();

    if (it != end && *it != '}')
      {
        throw format_error ("invalid format");
      }

    return it;
  }

  auto format (T const &t, format_context &ctx) const
  {
    using namespace boost::describe;

    using Bd = describe_bases<T, mod_any_access>;
    using Md = describe_members<T, mod_any_access>;

    auto out = ctx.out ();

    *out++ = '{';

    bool first = true;

    boost::mp11::mp_for_each<Bd> ([&] (auto D) {
      if (!first)
        {
          *out++ = ',';
        }

      first = false;

      out = fmt::format_to (out, " {}", (typename decltype (D)::type const &) t);
    });

    boost::mp11::mp_for_each<Md> ([&] (auto D) {
      if (!first)
        {
          *out++ = ',';
        }

      first = false;

      out = fmt::format_to (out, " .{}={}", D.name, t.*D.pointer);
    });

    if (!first)
      {
        *out++ = ' ';
      }

    *out++ = '}';

    return out;
  }
};

namespace zrythm::detail
{
struct BoostDescribeFormatTest
{
  int         a_;
  std::string b_;
  BOOST_DESCRIBE_CLASS (BoostDescribeFormatTest, (), (a_, b_), (), ())
};
static_assert (fmt::formattable<BoostDescribeFormatTest>);
}
