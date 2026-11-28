// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::frontends::mcp
{

/**
 * @brief The state of the MCP endpoint as seen from outside the server.
 *
 * errorText holds the reason while state is Failed, and it is empty in
 * every other state.
 */
class McpEndpoint : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("Owned by the project session")

  Q_PROPERTY (
    zrythm::frontends::mcp::McpEndpoint::State state READ state NOTIFY
      stateChanged FINAL)
  Q_PROPERTY (QString errorText READ errorText NOTIFY errorTextChanged FINAL)

public:
  enum class State
  {
    /** The endpoint is off, and no failure is reported. */
    Off,
    /** The endpoint starts. */
    Starting,
    /** The endpoint listens, and clients can connect. */
    Listening,
    /** The endpoint could not start. errorText() says why. */
    Failed,
  };
  Q_ENUM (State)

  ~McpEndpoint () override;

  [[nodiscard]] State state () const { return state_; }

  /** The reason the endpoint is off while the state is Failed, and an
   * empty string in every other state. */
  [[nodiscard]] QString errorText () const { return error_text_; }

Q_SIGNALS:
  void stateChanged ();
  void errorTextChanged ();

protected:
  explicit McpEndpoint (QObject * parent = nullptr);

  /** Sets the state to @p state. A state other than Failed clears the
   * error text. */
  void set_state (State state);
  /** Sets the reason the endpoint is off. It is shown while the state
   * is Failed. */
  void set_error_text (const QString &text);

private:
  State   state_{ State::Off };
  QString error_text_;
};

} // namespace zrythm::frontends::mcp
