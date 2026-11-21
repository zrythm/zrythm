// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>
#include <utility>

#include "utils/utf8_string.h"

namespace zrythm::utils
{

/**
 * @brief Why a request cannot be carried out.
 *
 * The kind is stable across releases: peers react to the kind, not to
 * the human-readable text.
 */
enum class RefusalKind
{
  /** A required parameter was not supplied. */
  MissingParameter,

  /** A supplied parameter has the wrong shape (type, format, encoding). */
  InvalidParameter,

  /** A supplied parameter is outside its accepted range. */
  OutOfRange,

  /** A referenced object does not exist. */
  NotFound,

  /**
   * The referenced objects' current state or relationships forbid the
   * request.
   */
  InvalidState,

  /** A session-level service the request needs is absent. */
  Unavailable,

  /** The caller's expected revision does not match the current one. */
  StaleRevision,

  /** The request would create a routing loop. */
  CyclicRoute,

  /** The endpoints take different signal types. */
  TypeMismatch,

  /** The thing to create already exists. */
  AlreadyExists,

  /**
   * The request never applies to the referenced objects (their nature
   * or policy forbids it).
   */
  Unsupported,
};

/** @brief A refusal's kind and its human-readable text. */
struct RefusalReason
{
  RefusalKind kind;
  Utf8String  text;
};

/** @brief The stable wire name of the kind, e.g. "cyclic_route". */
inline std::string_view
to_string (RefusalKind kind)
{
  switch (kind)
    {
    case RefusalKind::MissingParameter:
      return "missing_parameter";
    case RefusalKind::InvalidParameter:
      return "invalid_parameter";
    case RefusalKind::OutOfRange:
      return "out_of_range";
    case RefusalKind::NotFound:
      return "not_found";
    case RefusalKind::InvalidState:
      return "invalid_state";
    case RefusalKind::Unavailable:
      return "unavailable";
    case RefusalKind::StaleRevision:
      return "stale_revision";
    case RefusalKind::CyclicRoute:
      return "cyclic_route";
    case RefusalKind::TypeMismatch:
      return "type_mismatch";
    case RefusalKind::AlreadyExists:
      return "already_exists";
    case RefusalKind::Unsupported:
      return "unsupported";
    }
  std::unreachable ();
}

} // namespace zrythm::utils
