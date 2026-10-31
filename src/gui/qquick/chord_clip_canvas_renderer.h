// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/tick_types.h"

#include <QColor>
#include <QString>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::structure::arrangement
{
class ChordClip;
class ChordObject;
}

namespace zrythm::gui::qquick
{

/**
 * @brief One chord as it should be drawn within the clip, after loop
 * expansion. Pure data — the renderer converts to pixels.
 */
struct ComputedChordCell
{
  /** Absolute start position within the clip. */
  dsp::ContentTick abs_start{};
  /** Absolute end position within the clip. */
  dsp::ContentTick abs_end{};
  /** Pointer to the source chord object. */
  structure::arrangement::ChordObject * chord_object = nullptr;
  /** Index of the chord in insertion order (not sorted order). */
  int chord_index = 0;
};

/**
 * @brief Computes chord cells for a clip, expanding loops up to
 * @p display_end_tick.
 *
 * Extracted as a free function so it can be unit-tested without the Qt
 * Scene Graph. Uses for_each_loop_segment internally.
 */
std::vector<ComputedChordCell>
compute_chord_cells (
  const structure::arrangement::ChordClip &clip,
  dsp::ContentTick                         display_end_tick);

/**
 * @brief Renders chord clip content (colored cells, separator lines,
 * chord names with LOD).
 */
class ChordClipCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  ChordClipCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (ChordClipCanvasRenderer)

  void synchronizeData (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  struct CachedCell
  {
    float   x;
    float   width;
    QString full_name;
    QString root_note;
    float   full_name_width;
    QColor  bg_color;
    bool    muted;
  };

  std::vector<CachedCell> cells_;
  QColor                  text_color_;
  QFont                   font_;
  QFont                   bold_font_;
  float                   canvas_width_ = 0.0f;
  float                   canvas_height_ = 0.0f;
};

} // namespace zrythm::gui::qquick
