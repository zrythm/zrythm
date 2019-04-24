/*
	==============================================================================
	This file is part of Tal-Vocoder by Patrick Kunz.

	Copyright(c) 2005-2010 Patrick Kunz, TAL
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

#ifndef __FFT_H
#define __FFT_H

#define M_PI 3.14159265358979323846

class Fft
{
public:
	Fft();
	~Fft();

	short FFT2(short int dir,long m,float *x,float *y);

private:
   //long n,i,i1,j,k,i2,l,l1,l2;
   //float c1,c2,tx,ty,t1,t2,u1,u2,z;

};
#endif