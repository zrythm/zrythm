// ==================================================================================
// Copyright (c) 2012 HiFi-LoFi
//
// This is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ==================================================================================

#include "DecibelScale.h"

#include "CustomLookAndFeel.h"
#include "../DecibelScaling.h"


void DecibelScale::paint(Graphics& g)
{
  const int width = getWidth();
  const int height = getHeight();

  const float w = static_cast<float>(width);
  const float h = static_cast<float>(height);
  
  const CustomLookAndFeel& customLookAndFeel = CustomLookAndFeel::GetCustomLookAndFeel(this);
  const juce::Font font = customLookAndFeel.getScaleFont();
  const Colour colour = customLookAndFeel.getScaleColour();
  
  const int textWidth = width - 3;
  const int textHeight = static_cast<int>(::ceil(font.getHeight()));

  const float tickLeft = w - 6.0f;
  const float tickRight = w - 1.0f;

  const Justification textJustificationTopRight(Justification::topRight);

  g.setColour(colour);
  g.setFont(font);
  g.drawVerticalLine(width-1, 0.0f, h);

  const int steps[] = { 20, 0, -20, -40 };  
  for (size_t i=0; i<4; ++i)
  {
    const int db = steps[i];
    const int tickY = height - static_cast<int>(DecibelScaling::Db2Scale(static_cast<float>(db)) * h);
    g.drawHorizontalLine(tickY, tickLeft, tickRight);
    g.drawText(DecibelScaling::DecibelString(db),
               0,
               tickY + 1,
               textWidth,
               textHeight,
               textJustificationTopRight,
               false);
  }
  
  g.drawHorizontalLine(height-1, tickLeft, tickRight);
  g.drawText(DecibelScaling::DecibelString(static_cast<int>(DecibelScaling::MinScaleDb())),
             0,
             height - textHeight - 1,
             textWidth,
             textHeight,
             Justification(Justification::bottomRight),
             false);
}
