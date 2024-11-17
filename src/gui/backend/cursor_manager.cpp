
#include "common/utils/logger.h"
#include "gui/backend/cursor_manager.h"
#include "gui/backend/resource_manager.h"

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
CursorManager::setResizeEndCursor ()
{
  auto icon =
    ResourceManager::getIconUrl (u"zrythm-dark"_s, u"e-resize-cursor.svg"_s);
  setCursorWithSize (icon.toString (), 10, 11, 24);
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