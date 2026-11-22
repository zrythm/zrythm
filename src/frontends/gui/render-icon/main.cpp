// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QStringList>
#include <QSvgRenderer>

int
main (int argc, char * argv[])
{
  QCoreApplication app (argc, argv);

  QCommandLineParser parser;
  parser.setApplicationDescription (
    "Renders an SVG icon to PNG files at exact pixel sizes");
  parser.addHelpOption ();
  parser.addPositionalArgument ("svg", "The SVG file to render");
  parser.addPositionalArgument (
    "output-dir", "Directory the PNG files are written to");
  parser.addPositionalArgument (
    "sizes", "One or more square pixel sizes", "sizes...");
  parser.process (app);

  const QStringList args = parser.positionalArguments ();
  if (args.size () < 3)
    parser.showHelp (EXIT_FAILURE);

  const QString svg_path = args.at (0);
  const QString output_dir = args.at (1);
  const QString base_name = QFileInfo (svg_path).completeBaseName ();

  QSvgRenderer renderer (svg_path);
  if (!renderer.isValid ())
    {
      qCritical ("not a valid SVG file: %s", qPrintable (svg_path));
      return EXIT_FAILURE;
    }

  if (!QDir ().mkpath (output_dir))
    {
      qCritical ("failed to create directory: %s", qPrintable (output_dir));
      return EXIT_FAILURE;
    }

  for (const QString &size_arg : args.mid (2))
    {
      bool      ok = false;
      const int size = size_arg.toInt (&ok);
      if (!ok || size <= 0)
        {
          qCritical ("not a valid pixel size: %s", qPrintable (size_arg));
          return EXIT_FAILURE;
        }

      // render the vectors at the target resolution instead of scaling
      // one large render down
      QImage image (size, size, QImage::Format_ARGB32);
      image.fill (Qt::transparent);
      QPainter painter (&image);
      renderer.render (&painter, QRectF (0, 0, size, size));
      painter.end ();

      const QString output_path =
        QDir (output_dir)
          .filePath (QStringLiteral ("%1-%2.png").arg (base_name).arg (size));
      if (!image.save (output_path, "PNG"))
        {
          qCritical ("failed to write file: %s", qPrintable (output_path));
          return EXIT_FAILURE;
        }
    }

  return EXIT_SUCCESS;
}
