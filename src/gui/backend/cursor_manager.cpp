// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/cursor_manager.h"
#include "gui/backend/resource_manager.h"
#include "utils/logger.h"

#include <QApplication>
#include <QIcon>
#include <QStyle>
#include <QtGui/QGuiApplication>
#include <QtQml>
using namespace Qt::StringLiterals;

void
CursorManager::set_cursor (const QString &cache_key, const QCursor &cursor)
{
  if (cache_key == last_cursor_cache_key_)
    {
      return;
    }
  unsetCursor ();
  QGuiApplication::setOverrideCursor (cursor);
  ++popped_cursor_count_;
  last_cursor_cache_key_ = cache_key;
}

void
CursorManager::
  setCursorWithSize (const QString &imagePath, int hotX, int hotY, int size)
{
  // Check cache first
  auto cacheKey =
    QString (u"%1_%2_%3_%4"_s).arg (imagePath).arg (hotX).arg (hotY).arg (size);
  if (cursor_cache_.contains (cacheKey))
    {
      set_cursor (cacheKey, cursor_cache_.value (cacheKey));
      return;
    }

  // Create new cursor
  QString actual_image_path (imagePath);
  if (actual_image_path.startsWith (u"qrc:"_s))
    {
      actual_image_path = actual_image_path.mid (3);
    }
  QIcon icon (actual_image_path);
  auto  cursor_size = size;
  z_debug ("image path: {}, cursor size: {}", actual_image_path, cursor_size);
  QCursor cursor (icon.pixmap (cursor_size, cursor_size), hotX, hotY);

  // Cache it
  cursor_cache_.insert (cacheKey, cursor);

  // Set it
  set_cursor (cacheKey, cursor);
}

void
CursorManager::setCursor (const QString &imagePath, int hotX, int hotY)
{
  setCursorWithSize (imagePath, hotX, hotY, 32);
}

void
CursorManager::setPointerCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"select-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 2, 1, 24);
}

void
CursorManager::setPencilCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"edit-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 2, 3, 24);
}

void
CursorManager::setBrushCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"brush-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 2, 3, 24);
}

void
CursorManager::setCutCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"cut-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 9, 7, 24);
}

void
CursorManager::setEraserCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"eraser-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 4, 2, 24);
}

void
CursorManager::setRampCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"ramp-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 2, 3, 24);
}

void
CursorManager::setAuditionCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"audition-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 10, 12, 24);
}

void
CursorManager::setOpenHandCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"move-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 12, 11, 24);
}

void
CursorManager::setClosedHandCursor ()
{
  QCursor cursor (Qt::CursorShape::ClosedHandCursor);
  set_cursor (QString::fromUtf8 (__func__), cursor);
}

void
CursorManager::setCopyCursor ()
{
  QCursor cursor (Qt::CursorShape::DragCopyCursor);
  set_cursor (QString::fromUtf8 (__func__), cursor);
}

void
CursorManager::setLinkCursor ()
{
  QCursor cursor (Qt::CursorShape::DragLinkCursor);
  set_cursor (QString::fromUtf8 (__func__), cursor);
}

void
CursorManager::setResizeStartCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"w-resize-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 14, 11, 24);
}
void
CursorManager::setStretchStartCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"w-stretch-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 14, 11, 24);
}
void
CursorManager::setResizeLoopStartCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"w-loop-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 14, 11, 24);
}

void
CursorManager::setResizeEndCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"e-resize-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 10, 11, 24);
}

void
CursorManager::setStretchEndCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"e-stretch-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 10, 11, 24);
}
void
CursorManager::setResizeLoopEndCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"e-loop-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 10, 11, 24);
}
void
CursorManager::setFadeInCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"fade-in-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 3, 1, 24);
}
void
CursorManager::setFadeOutCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"fade-out-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 3, 1, 24);
}

void
CursorManager::unsetCursor ()
{
  for (int i = 0; i < popped_cursor_count_; ++i)
    {
      z_debug ("unsetting cursor");
      QGuiApplication::restoreOverrideCursor ();
    }
  popped_cursor_count_ = 0;
  last_cursor_cache_key_ = u""_s;
}
