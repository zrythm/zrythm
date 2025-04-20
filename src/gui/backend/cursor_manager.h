// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QCursor>
#include <QtQmlIntegration>

/**
 * @brief Cursor manager for arrangers.
 */
class CursorManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  Q_INVOKABLE void
  setCursorWithSize (const QString &imagePath, int hotX, int hotY, int size);
  Q_INVOKABLE void setCursor (const QString &imagePath, int hotX, int hotY);

  Q_INVOKABLE void setPointerCursor ();
  Q_INVOKABLE void setPencilCursor ();
  Q_INVOKABLE void setBrushCursor ();
  Q_INVOKABLE void setCutCursor ();
  Q_INVOKABLE void setEraserCursor ();
  Q_INVOKABLE void setRampCursor ();
  Q_INVOKABLE void setAuditionCursor ();
  Q_INVOKABLE void setOpenHandCursor ();
  Q_INVOKABLE void setClosedHandCursor ();
  Q_INVOKABLE void setCopyCursor ();
  Q_INVOKABLE void setLinkCursor ();
  Q_INVOKABLE void setResizeStartCursor ();
  Q_INVOKABLE void setStretchStartCursor ();
  Q_INVOKABLE void setResizeLoopStartCursor ();
  Q_INVOKABLE void setResizeEndCursor ();
  Q_INVOKABLE void setStretchEndCursor ();
  Q_INVOKABLE void setResizeLoopEndCursor ();
  Q_INVOKABLE void setFadeInCursor ();
  Q_INVOKABLE void setFadeOutCursor ();

  Q_INVOKABLE void unsetCursor ();

private:
  void set_cursor (const QString &cache_key, const QCursor &cursor);

private:
  int                     popped_cursor_count_ = 0;
  QHash<QString, QCursor> cursor_cache_;
  QString                 last_cursor_cache_key_;
};
