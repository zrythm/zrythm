/*
	==============================================================================
	This file is part of Tal-Dub-III by Patrick Kunz.

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

#if !defined(__TalPreset_h)
#define __TalPreset_h

class TalPreset
{
  public:
	String name;
	float programData[NUMPARAM];
    int   midiMap[255]; // 255 Midi Values to Map
  
    TalPreset()
	{
		// Set values to 0
		for (int i = 0; i < NUMPARAM; i++) {
			programData[i]=  0.0f;
		}
		// Init default midiMap
		for (int i = 0; i < 255; i++) 
		{
			midiMap[i]=  0;
		}
		name = T("default");
	}

    ~TalPreset() {}
};
#endif
