// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#if HAVE_JACK

#  include "weakjack/weak_libjack.h"

namespace zrythm::utils::jack
{

static inline std::string
get_error_message (jack_status_t status)
{
  return
    [status] ()
      -> QString {
      if (status & JackFailure)
        {
          return
            /* TRANSLATORS: JACK failure messages */
            QObject::tr ("Overall operation failed");
        }
      else if (status & JackInvalidOption)
        {
          return QObject::tr (
            "The operation contained an invalid or unsupported option");
        }
      else if (status & JackNameNotUnique)
        {
          return QObject::tr ("The desired client name was not unique");
        }
      else if (status & JackServerFailed)
        {
          return QObject::tr ("Unable to connect to the JACK server");
        }
      else if (status & JackServerError)
        {
          return QObject::tr ("Communication error with the JACK server");
        }
      else if (status & JackNoSuchClient)
        {
          return QObject::tr ("Requested client does not exist");
        }
      else if (status & JackLoadFailure)
        {
          return QObject::tr ("Unable to load internal client");
        }
      else if (status & JackInitFailure)
        {
          return QObject::tr ("Unable to initialize client");
        }
      else if (status & JackShmFailure)
        {
          return QObject::tr ("Unable to access shared memory");
        }
      else if (status & JackVersionError)
        {
          return QObject::tr ("Client's protocol version does not match");
        }
      else if (status & JackBackendError)
        {
          return QObject::tr ("Backend error");
        }
      else if (status & JackClientZombie)
        {
          return QObject::tr ("Client zombie");
        }

      z_return_val_if_reached (QString::fromUtf8 ("unknown JACK error"));
    }()
           .toStdString ();
}
}

#endif // HAVE_JACK
