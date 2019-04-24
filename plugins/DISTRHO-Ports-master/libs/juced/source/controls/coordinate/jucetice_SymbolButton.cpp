/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

  Original Code by: braindoc

 ==============================================================================
*/

BEGIN_JUCE_NAMESPACE

SymbolButton::SymbolButton() : ToggleButton(String())
{
 setSize(16, 16);
 symbolIndex = 0;
}

SymbolButton::~SymbolButton(void)
{

}

void SymbolButton::setSymbolIndex(int newSymbolIndex)
{
 symbolIndex = newSymbolIndex;
}


void SymbolButton::paintButton(Graphics &g, bool isMouseOverButton, 
                               bool isButtonDown)
{
 ToggleButton::paintButton(g, isMouseOverButton, isButtonDown);

 float x1, x2;
 float y1, y2;
 switch( symbolIndex )
 {
 case PLUS:
  {
   x1 = 0.5f*getWidth();
   x2 = x1;
   y1 = getHeight()-4.f;
   y2 = 4.f;
   g.drawLine(x1, y1, x2, y2, 2.f);

   x1 = 4.f;
   x2 = getWidth()-4.f;
   y1 = 0.5f*(getHeight());
   y2 = y1;
   g.drawLine(x1, y1, x2, y2, 2.f);
  }
  break;
 case MINUS:
  {
   x1 = 4.f;
   x2 = getWidth()-4.f;
   y1 = 0.5f*(getHeight());
   y2 = y1;
   g.drawLine(x1, y1, x2, y2, 2.f);
  }
  break;
 }

}

END_JUCE_NAMESPACE
