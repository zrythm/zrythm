// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/file_audio_source.h"
#include "gui/backend/qml_utils.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/automation_region.h"
#include "structure/arrangement/region_renderer.h"
#include "utils/math.h"
#include "utils/utf8_string.h"

#include <QQmlEngine>
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

QColor
QmlUtils::adjustOpacity (const QColor &color, float newOpacity)
{
  return QColor::fromRgbF (
    color.redF (), color.greenF (), color.blueF (), newOpacity);
}

QVector<gui::WaveformChannel *>
QmlUtils::getAudioRegionWaveform (QObject * audioRegion, int pixelWidth)
{
  QVector<gui::WaveformChannel *> result;

  if (audioRegion == nullptr || pixelWidth <= 0)
    {
      return result;
    }

  // Cast to AudioRegion
  auto * region =
    qobject_cast<structure::arrangement::AudioRegion *> (audioRegion);
  if (region == nullptr)
    {
      return result;
    }

  // Get the audio source from the region
  try
    {
      // Calculate total length in samples
      auto totalLengthSamples = static_cast<unsigned_frame_t> (
        region->bounds ()->length ()->samples ());

      if (totalLengthSamples == 0)
        {
          return result;
        }

      // Serialize the entire region to a buffer
      juce::AudioSampleBuffer buffer;
      structure::arrangement::RegionRenderer::serialize_to_buffer (
        *region, buffer);

      const int numChannels = buffer.getNumChannels ();
      const int numSamples = buffer.getNumSamples ();

      if (numSamples == 0)
        {
          return result;
        }

      // Generate waveform data for each channel
      for (int ch = 0; ch < numChannels; ++ch)
        {
          QVector<float> minValues (pixelWidth);
          QVector<float> maxValues (pixelWidth);

          // Initialize with opposite values
          std::fill (minValues.begin (), minValues.end (), 1.0f);
          std::fill (maxValues.begin (), maxValues.end (), -1.0f);

          const float * channelData = buffer.getReadPointer (ch);

          // Calculate samples per pixel
          const float samplesPerPixel =
            static_cast<float> (numSamples) / static_cast<float> (pixelWidth);

          // For each pixel, find min/max values
          for (int px = 0; px < pixelWidth; ++px)
            {
              const int startSample =
                static_cast<int> (static_cast<float> (px) * samplesPerPixel);
              const int endSample = std::min (
                static_cast<int> (static_cast<float> (px + 1) * samplesPerPixel),
                numSamples);

              if (startSample >= numSamples)
                break;

              float minVal = 1.0f;
              float maxVal = -1.0f;

              // Find min/max in this pixel range
              for (int sample = startSample; sample < endSample; ++sample)
                {
                  const float val = channelData[sample];
                  // Clamp to -1.0 to 1.0 range
                  const float clampedVal = std::clamp (val, -1.0f, 1.0f);
                  minVal = std::min (minVal, clampedVal);
                  maxVal = std::max (maxVal, clampedVal);
                }

              minValues[px] = minVal;
              maxValues[px] = maxVal;
            }

          // Create WaveformChannel and add to result
          auto * channel =
            new gui::WaveformChannel (minValues, maxValues, pixelWidth);

          // Transfer ownership to QML JavaScript engine for proper cleanup
          QQmlEngine::setObjectOwnership (
            channel, QQmlEngine::JavaScriptOwnership);

          result.append (channel);
        }

      return result;
    }
  catch (...)
    {
      // Return empty result on any error
      return result;
    }
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
