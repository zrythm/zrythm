// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/project/clipboard_payload.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::controllers
{

/**
 * @brief Application-wide clipboard for project objects (arranger objects,
 * tracks and plugins).
 *
 * Holds the in-process payload and bridges it to the OS clipboard as
 * compressed text, enabling paste across projects and Zrythm instances. A
 * single instance is shared by all project sessions.
 *
 * Valid payloads arriving on the OS clipboard (e.g. written by another
 * instance) are adopted automatically; unrelated clipboard contents leave
 * the in-process payload intact.
 */
class Clipboard : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    bool hasArrangerObjects READ hasArrangerObjects NOTIFY payloadChanged FINAL)
  Q_PROPERTY (bool hasTracks READ hasTracks NOTIFY payloadChanged FINAL)
  Q_PROPERTY (bool hasPlugins READ hasPlugins NOTIFY payloadChanged FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using ClipboardTextProvider = std::function<QString ()>;
  using ClipboardTextWriter = std::function<void (const QString &)>;

  /**
   * @brief Creates a clipboard bridged to the OS clipboard.
   */
  explicit Clipboard (QObject * parent = nullptr);

  /**
   * @brief Creates a clipboard with injected text access (for tests).
   *
   * No OS clipboard interaction happens; call reloadFromSystemClipboard()
   * to simulate external clipboard changes.
   */
  Clipboard (
    ClipboardTextProvider text_provider,
    ClipboardTextWriter   text_writer,
    QObject *             parent = nullptr);

  /**
   * @brief Stores a payload in-process and on the OS clipboard.
   *
   * The payload is taken by value: pass a temporary or std::move an
   * lvalue to avoid copying the registry JSON.
   *
   * @throw utils::ZrythmException if the encoded text exceeds
   * ClipboardPayload::kMaxClipboardTextLength (it could never be decoded
   * by another instance); the clipboard keeps its previous payload.
   */
  void setPayload (structure::project::ClipboardPayload payload);

  /**
   * @brief Returns the current payload, if any.
   *
   * May originate from another Zrythm instance (adopted from the OS
   * clipboard).
   */
  const std::optional<structure::project::ClipboardPayload> &payload () const
  {
    return payload_;
  }

  bool hasArrangerObjects () const;
  bool hasTracks () const;
  bool hasPlugins () const;

  /**
   * @brief Re-reads the system clipboard text and adopts it if it is a valid
   * Zrythm payload.
   *
   * Called automatically on QClipboard::dataChanged when bridged to the OS
   * clipboard.
   */
  Q_INVOKABLE void reloadFromSystemClipboard ();

Q_SIGNALS:
  void payloadChanged ();

private:
  std::optional<structure::project::ClipboardPayload> payload_;

  /** Length and hash of the clipboard text last seen (written by us or
   * adopted), to avoid reacting to the same contents twice without
   * pinning the text in memory. */
  qsizetype  last_seen_text_length_ = -1;
  QByteArray last_seen_text_hash_;

  /** True while reloadFromSystemClipboard() is running: fetching the
   * clipboard text can spin a nested event loop (X11/Wayland), which must
   * not re-enter the reload. */
  bool reloading_ = false;

  /** Bumped by every setPayload(). A reload whose text fetch spanned a
   * setPayload (nested event loop) detects the change and discards its
   * stale text. */
  std::uint64_t payload_generation_ = 0;

  /** Returns whether @p text is the text last remembered. */
  [[nodiscard]] bool text_is_known (const QString &text) const;

  /** Remembers @p text as the clipboard text last seen. */
  void remember_text (const QString &text);

  ClipboardTextProvider text_provider_;
  ClipboardTextWriter   text_writer_;
};

} // namespace zrythm::controllers
