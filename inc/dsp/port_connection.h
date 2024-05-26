// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Port connection.
 */

#ifndef __AUDIO_PORT_CONNECTION_H__
#define __AUDIO_PORT_CONNECTION_H__

#include "zrythm-config.h"

#include "dsp/port_identifier.h"
#include "utils/yaml.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * A connection between two ports.
 */
typedef struct PortConnection
{
  PortIdentifier * src_id;
  PortIdentifier * dest_id;

  /**
   * Multiplier to apply, where applicable.
   *
   * Range: 0 to 1.
   * Default: 1.
   */
  float multiplier;

  /**
   * Whether the connection can be removed or the
   * multiplier edited by the user.
   *
   * Ignored when connecting things internally and
   * only used to deter the user from breaking
   * necessary connections.
   */
  bool locked;

  /**
   * Whether the connection is enabled.
   *
   * @note The user can disable port connections only
   * if they are not locked.
   */
  bool enabled;

  /** Used for CV -> control port connections. */
  float base_value;
} PortConnection;

PortConnection *
port_connection_new (
  const PortIdentifier * src,
  const PortIdentifier * dest,
  float                  multiplier,
  bool                   locked,
  bool                   enabled);

NONNULL void
port_connection_update (
  PortConnection * self,
  float            multiplier,
  bool             locked,
  bool             enabled);

NONNULL bool
port_connection_is_send (const PortConnection * self);

NONNULL void
port_connection_print_to_str (
  const PortConnection * self,
  char *                 buf,
  size_t                 buf_sz);

NONNULL void
port_connection_print (const PortConnection * self);

/**
 * To be used during serialization.
 */
NONNULL PortConnection *
port_connection_clone (const PortConnection * src);

/**
 * Deletes port, doing required cleanup and updating counters.
 */
NONNULL void
port_connection_free (PortConnection * self);

/**
 * @}
 */

#endif /* __AUDIO_PORT_CONNECTION_H__ */
