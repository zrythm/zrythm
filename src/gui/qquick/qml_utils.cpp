// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/file_audio_source.h"
#include "gui/qquick/qml_utils.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/automation_region.h"
#include "structure/arrangement/region_renderer.h"
#include "utils/io_utils.h"
#include "utils/math_utils.h"
#include "utils/utf8_string.h"

#include <QRectF>
#include <QRegularExpression>

namespace zrythm::gui::qquick
{

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

QString
QmlUtils::pathBasename (const QString &path)
{
  return utils::Utf8String::from_path (
           utils::io::path_get_basename (
             utils::Utf8String::from_qstring (path).to_path ()))
    .to_qstring ();
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

QColor
QmlUtils::adjustOpacity (const QColor &color, float newOpacity)
{
  return QColor::fromRgbF (
    color.redF (), color.greenF (), color.blueF (), newOpacity);
}

QVector<float>
QmlUtils::getAutomationRegionValues (QObject * automationRegion, int pixelWidth)
{
  QVector<float> result;

  if (automationRegion == nullptr || pixelWidth <= 0)
    {
      return result;
    }

  // Cast to AutomationRegion
  auto * region =
    qobject_cast<structure::arrangement::AutomationRegion *> (automationRegion);
  if (region == nullptr)
    {
      return result;
    }

  try
    {
      // Calculate total length in samples
      auto totalLengthSamples = static_cast<unsigned_frame_t> (
        region->bounds ()->length ()->samples ());

      if (totalLengthSamples == 0)
        {
          return result;
        }

      // Serialize the entire region to a vector of automation values
      std::vector<float> automationValues;
      structure::arrangement::RegionRenderer::serialize_to_automation_values (
        *region, automationValues);

      const int numSamples = static_cast<int> (automationValues.size ());

      if (numSamples == 0)
        {
          return result;
        }

      // Resize result vector
      result.resize (pixelWidth);

      // Calculate samples per pixel
      const float samplesPerPixel =
        static_cast<float> (numSamples) / static_cast<float> (pixelWidth);

      // For each pixel, find the average value
      for (int px = 0; px < pixelWidth; ++px)
        {
          const int startSample =
            static_cast<int> (static_cast<float> (px) * samplesPerPixel);
          const int endSample = std::min (
            static_cast<int> (static_cast<float> (px + 1) * samplesPerPixel),
            numSamples);

          if (startSample >= numSamples)
            break;

          float sum = 0.0f;
          int   count = 0;

          // Find average value in this pixel range
          for (int sample = startSample; sample < endSample; ++sample)
            {
              const float val = automationValues[sample];
              // Skip default values (-1.0f)
              if (val >= 0.0f)
                {
                  sum += val;
                  count++;
                }
            }

          // If we found any valid values, use the average
          // Otherwise, use -1.0f to indicate no automation data
          result[px] = (count > 0) ? (sum / static_cast<float> (count)) : -1.0f;
        }

      return result;
    }
  catch (...)
    {
      // Return empty result on any error
      return result;
    }
}

bool
QmlUtils::rectanglesIntersect (QRectF a, QRectF b)
{
  return a.intersects (b);
}

bool
QmlUtils::rectanglesIntersect (QRect a, QRect b)
{
  return a.intersects (b);
}

QItemSelection
QmlUtils::createRowSelection (
  QAbstractItemModel * model,
  const QList<int>    &rows,
  int                  column)
{
  QItemSelection selection;
  for (int row : rows)
    {
      const auto index = model->index (row, column);
      if (index.isValid ())
        {
          selection.select (index, index); // Selects a single item range
        }
    }
  return selection;
}

QItemSelection
QmlUtils::createRangeSelection (
  QAbstractItemModel * model,
  int                  startRow,
  int                  endRow,
  int                  startCol,
  int                  endCol)
{
  const auto topLeft = model->index (startRow, startCol);
  const auto bottomRight = model->index (endRow, endCol);
  return { topLeft, bottomRight };
}
}
