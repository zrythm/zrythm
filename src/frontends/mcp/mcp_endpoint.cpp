// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "frontends/mcp/mcp_endpoint.h"

namespace zrythm::frontends::mcp
{

McpEndpoint::McpEndpoint (QObject * parent) : QObject (parent) { }

McpEndpoint::~McpEndpoint () = default;

void
McpEndpoint::set_state (const State state)
{
  // The reason belongs to the Failed state.
  if (state != State::Failed)
    {
      set_error_text (QString ());
    }
  if (state_ == state)
    {
      return;
    }
  state_ = state;
  Q_EMIT stateChanged ();
}

void
McpEndpoint::set_error_text (const QString &text)
{
  if (error_text_ == text)
    {
      return;
    }
  error_text_ = text;
  Q_EMIT errorTextChanged ();
}

} // namespace zrythm::frontends::mcp
