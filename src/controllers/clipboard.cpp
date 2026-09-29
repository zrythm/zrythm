// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "controllers/clipboard.h"
#include "utils/exceptions.h"

#include <QClipboard>
#include <QCryptographicHash>
#include <QGuiApplication>
#include <QMimeData>
#include <QTimer>

namespace zrythm::controllers
{

namespace
{
struct ReentrancyGuard
{
  bool &flag_;
  explicit ReentrancyGuard (bool &flag) : flag_ (flag) { flag_ = true; }
  ~ReentrancyGuard () { flag_ = false; }
};
} // namespace

Clipboard::Clipboard (QObject * parent) : QObject (parent)
{
  QObject::connect (
    QGuiApplication::clipboard (), &QClipboard::dataChanged, this,
    &Clipboard::reloadFromSystemClipboard);

  // Adopt a payload that was copied before this session started. Deferred
  // to the event loop: fetching the text can spin a nested event loop,
  // which must not run while the application is still constructing
  QTimer::singleShot (0, this, &Clipboard::reloadFromSystemClipboard);
}

Clipboard::Clipboard (
  ClipboardTextProvider text_provider,
  ClipboardTextWriter   text_writer,
  QObject *             parent)
    : QObject (parent), text_provider_ (std::move (text_provider)),
      text_writer_ (std::move (text_writer))
{
}

bool
Clipboard::hasArrangerObjects () const
{
  return payload_.has_value ()
         && payload_->type ()
              == structure::project::ClipboardPayload::Type::ArrangerObjects;
}

bool
Clipboard::hasTracks () const
{
  return payload_.has_value ()
         && payload_->type () == structure::project::ClipboardPayload::Type::Tracks;
}

bool
Clipboard::hasPlugins () const
{
  return payload_.has_value ()
         && payload_->type () == structure::project::ClipboardPayload::Type::Plugins;
}

void
Clipboard::setPayload (structure::project::ClipboardPayload payload)
{
  // Encode first: if it throws, the clipboard keeps its previous payload
  // and listeners are not told about a payload that never landed
  const auto text = payload.encode_to_clipboard_text ();

  // Refuse at the source: the decoder rejects texts this large, so a
  // payload that encodes past the cap could never be adopted by another
  // instance (including this one after a restart)
  if (
    text.size () > structure::project::ClipboardPayload::kMaxClipboardTextLength)
    {
      throw utils::ZrythmException (
        fmt::format (
          "Clipboard payload of {} characters exceeds the maximum of {} "
          "and cannot be copied",
          text.size (),
          structure::project::ClipboardPayload::kMaxClipboardTextLength));
    }

  payload_ = std::move (payload);
  ++payload_generation_;
  remember_text (text);
  if (text_writer_ != nullptr)
    {
      text_writer_ (text);
    }
  else
    {
      QGuiApplication::clipboard ()->setText (text);
    }

  Q_EMIT payloadChanged ();
}

bool
Clipboard::text_is_known (const QString &text) const
{
  return text.size () == last_seen_text_length_
         && QCryptographicHash::hash (text.toUtf8 (), QCryptographicHash::Sha256)
              == last_seen_text_hash_;
}

void
Clipboard::remember_text (const QString &text)
{
  last_seen_text_length_ = text.size ();
  last_seen_text_hash_ =
    QCryptographicHash::hash (text.toUtf8 (), QCryptographicHash::Sha256);
}

void
Clipboard::reloadFromSystemClipboard ()
{
  // Fetching the text can spin a nested event loop (X11/Wayland IPC with
  // the clipboard owner), which re-enters dataChanged: ignore nested
  // calls so a stale reload cannot overwrite a newer payload
  if (reloading_)
    return;
  ReentrancyGuard reentrancy_guard{ reloading_ };

  const auto generation_before_fetch = payload_generation_;
  const auto text = [this] () -> QString {
    if (text_provider_ != nullptr)
      return text_provider_ ();
    // Fetching the text transfers the whole payload from the clipboard
    // owner: probe first so non-text clipboards cost no transfer
    const auto * mime = QGuiApplication::clipboard ()->mimeData ();
    if (mime == nullptr || !mime->hasText ())
      return {};
    return QGuiApplication::clipboard ()->text ();
  }();
  // The fetch can spin a nested event loop, during which a new payload may
  // have been stored: the fetched text is stale then and must not
  // overwrite the newer payload
  if (payload_generation_ != generation_before_fetch)
    return;
  if (text_is_known (text))
    return;

  auto decoded =
    structure::project::ClipboardPayload::decode_from_clipboard_text (text);
  if (!decoded.has_value ())
    {
      // Decoding is deterministic: remember prefixed texts that failed
      // so the decompress+parse+validate cost is paid once per distinct
      // text. Oversized texts are rejected on size alone and skip the
      // hashing (a hash of them would cost a full UI-thread pass).
      if (
        text.size ()
          <= structure::project::ClipboardPayload::kMaxClipboardTextLength
        && text.startsWith (
          QString::fromUtf8 (
            structure::project::ClipboardPayload::kTextPrefix.data (),
            structure::project::ClipboardPayload::kTextPrefix.size ())))
        {
          remember_text (text);
        }
      return;
    }

  remember_text (text);
  payload_ = std::move (decoded);
  Q_EMIT payloadChanged ();
}

} // namespace zrythm::controllers
