// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#ifdef HAVE_JACK

#  include "utils/string.h"

#  include <QString>

#  include "weakjack/weak_libjack.h"

using namespace Qt::StringLiterals;

namespace zrythm::utils::jack
{

static inline utils::Utf8String
get_error_message (jack_status_t status)
{
  return utils::Utf8String::from_qstring ([status] () -> QString {
    if (status & JackFailure)
      {
        return
          /* TRANSLATORS: JACK failure messages */
          QObject::tr ("Overall operation failed");
      }
    if (status & JackInvalidOption)
      {
        return QObject::tr (
          "The operation contained an invalid or unsupported option");
      }
    if (status & JackNameNotUnique)
      {
        return QObject::tr ("The desired client name was not unique");
      }
    if (status & JackServerFailed)
      {
        return QObject::tr ("Unable to connect to the JACK server");
      }
    if (status & JackServerError)
      {
        return QObject::tr ("Communication error with the JACK server");
      }
    if (status & JackNoSuchClient)
      {
        return QObject::tr ("Requested client does not exist");
      }
    if (status & JackLoadFailure)
      {
        return QObject::tr ("Unable to load internal client");
      }
    if (status & JackInitFailure)
      {
        return QObject::tr ("Unable to initialize client");
      }
    if (status & JackShmFailure)
      {
        return QObject::tr ("Unable to access shared memory");
      }
    if (status & JackVersionError)
      {
        return QObject::tr ("Client's protocol version does not match");
      }
    if (status & JackBackendError)
      {
        return QObject::tr ("Backend error");
      }
    if (status & JackClientZombie)
      {
        return QObject::tr ("Client zombie");
      }

    z_return_val_if_reached (u"unknown JACK error"_s);
  }());
}
}

#endif // defined(HAVE_JACK)
