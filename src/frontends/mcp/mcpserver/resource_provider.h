// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QtCore/QFuture>
#include <QtCore/QMetaType>
#include <QtCore/QObject>

#include <mcpserver/mcp_types.h>

namespace zrythm::mcp
{

/**
 * @brief The resources of one source that the server lists and reads.
 *
 * The server calls resources() and read() on the thread that owns the server.
 * The signals may be emitted from any thread.
 */
class ResourceProvider : public QObject
{
  Q_OBJECT

public:
  ~ResourceProvider () override = default;

  /** The resources that the provider serves. */
  [[nodiscard]] virtual auto resources () const -> std::vector<Resource> = 0;

  /** Reads one resource that resources() lists. The future may finish on any
   * thread. */
  [[nodiscard]] virtual auto
  read (const ResourceUri &uri) -> QFuture<ResourceContents> = 0;

Q_SIGNALS:
  /** The list that resources() returns changed. */
  void resources_changed ();

  /** The content of the resource @p uri changed. */
  void resource_updated (const zrythm::mcp::ResourceUri &uri);
};

} // namespace zrythm::mcp

Q_DECLARE_METATYPE (zrythm::mcp::ResourceUri)
