/*
	==============================================================================
	This file is part of Tal-Reverb by Patrick Kunz.

	Copyright(c) 2005-2009 Patrick Kunz, TAL
	Togu Audio Line, Inc.
	http://kunz.corrupt.ch

	This file may be licensed under the terms of of the
	GNU General Public License Version 2 (the ``GPL'').

	Software distributed under the License is distributed
	on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
	express or implied. See the GPL for the specific language
	governing rights and limitations.

	You should have received a copy of the GPL along with this
	program. If not, go to http://www.gnu.org/licenses/gpl.html
	or write to the Free Software Foundation, Inc.,  
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	==============================================================================
 */


#ifndef PositionUtility_H
#define PositionUtility_H

class PositionUtility
{
private:
    int width;
    int height;

public:
    PositionUtility(int width, int height)
    {
        this->height = height;
        this->width = width;
    }

    int getHeight()
    {
        return height;
    }

    int getWidth()
    {
        return width;
    }


    // In x[0..1], y[0..1]
    // Out x[0..width], y[0..height]
    Point<int> calculateViewPosition(Point<float> normalizedValues)
    {
        float viewX = normalizedValues.getX() * this->width;
        float viewY = (1.0f - normalizedValues.getY()) * this->height;
        return Point<int>( (int)(viewX + 0.5f), (int)(viewY + 0.5f));
    }

    Point<float> calculateNormalizedPosition(Point<int> pixelValues)
    {
        float viewX = (float)pixelValues.getX() / (float)this->width;
        float viewY = 1.0f - (float)pixelValues.getY() / (float)this->height;
        return Point<float>(viewX, viewY);
    }
};
#endif