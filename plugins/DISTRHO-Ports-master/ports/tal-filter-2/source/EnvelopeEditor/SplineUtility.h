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


#ifndef SplineUtility_H
#define SplineUtility_H

class SplineUtility
{
private:

public:
    SplineUtility()
    {
    }

    /*
    cp is a 4 element array where:
    cp[0] is the starting point, or P0 in the above diagram
    cp[1] is the first control point, or P1 in the above diagram
    cp[2] is the second control point, or P2 in the above diagram
    cp[3] is the end point, or P3 in the above diagram
    t is the parameter value, 0 <= t <= 1
    */
    Point<float> PointOnCubicBezier(float t, Point<float> p1, Point<float> p2, Point<float> p3, Point<float> p4)
    {
        float ax, bx, cx;
        float ay, by, cy;
        float tSquared, tCubed;
        Point<float> result;

        /* calculate the polynomial coefficients */
        cx = 3.0f * (p2.getX() - p1.getX());
        bx = 3.0f * (p3.getX() - p2.getX()) - cx;
        ax = p4.getX() - p1.getX() - cx - bx;

        cy = 3.0f * (p2.getY() - p1.getY());
        by = 3.0f * (p3.getY() - p2.getY()) - cy;
        ay = p4.getY() - p1.getY() - cy - by;

        /* calculate the curve point at parameter value t */

        tSquared = t * t;
        tCubed = tSquared * t;

        result.setX((ax * tCubed) + (bx * tSquared) + (cx * t) + p1.getX());
        result.setY((ay * tCubed) + (by * tSquared) + (cy * t) + p1.getY());

        return result;
    }
};
#endif