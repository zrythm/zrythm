// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/qml_utils.h"
#include "utils/math.h"
#include "utils/string.h"

#include <QRegularExpression>

QString
QmlUtils::toPathString (const QUrl &url)
{
  return utils::Utf8String::from_qurl (url).to_qstring ();
}

QUrl
QmlUtils::localFileToQUrl (const QString &path)
{
  return QUrl::fromLocalFile (path);
}

float
QmlUtils::amplitudeToDbfs (float amplitude)
{
  return utils::math::amp_to_dbfs (amplitude);
}

QStringList
QmlUtils::removeDuplicates (const QStringList &list)
{
  QStringList   result;
  QSet<QString> seen;

  for (const QString &item : list)
    {
      if (!seen.contains (item))
        {
          seen.insert (item);
          result.append (item);
        }
    }

  return result;
}

QStringList
QmlUtils::splitTextLines (const QString &text)
{
  QStringList lines;
  if (text.isEmpty ())
    {
      return lines;
    }

  // Split by any combination of \r\n, \r, or \n
  QStringList rawLines =
    text.split (QRegularExpression ("\r?\n|\r"), Qt::SkipEmptyParts);

  // Trim each line and filter out empty strings
  for (const QString &line : rawLines)
    {
      QString trimmed = line.trimmed ();
      if (!trimmed.isEmpty ())
        {
          lines.append (trimmed);
        }
    }

  return lines;
}

QString
QmlUtils::readTextFileContent (const QString &filePath)
{
  QFile f (filePath);
  if (!f.open (QFile::ReadOnly | QFile::Text))
    {
      return format_qstr ("Failed to open file at {}", f.fileName ());
    }
  return f.readAll ();
}

QColor
QmlUtils::saturate (const QColor &color, float perc)
{
  float h, s, v;
  color.getHslF (&h, &s, &v);
  return QColor::fromHslF (h, std::clamp (s * perc, 0.f, 1.f), v);
}

QColor
QmlUtils::makeBrighter (const QColor &color, float perc)
{
  float h, s, v;
  color.getHslF (&h, &s, &v);
  return QColor::fromHslF (h, s, std::clamp (v * perc, 0.f, 1.f));
}
