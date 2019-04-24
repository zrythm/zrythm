/*
	==============================================================================
	This file is part of Tal-Reverb by Patrick Kunz.

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

#ifndef TalComboBox_H
#define TalComboBox_H

#include "../Engine/AudioUtils.h"

class TalComboBox : public ComboBox
{
	AudioUtils audioUtils;

public:

	TalComboBox(int index) : ComboBox(juce::String(index))
	{
        this->index = index;
        getProperties().set(Identifier("index"), index);
	}

    void setNormalizedSelectedId(float normalizedSelectedId)
    {
        int selectedId = audioUtils.calcComboBoxValue(normalizedSelectedId, index);
        setSelectedId(selectedId, dontSendNotification);
    }

    float getNormalizedSelectedId()
    {
        int selectedValue = getSelectedId();
        return audioUtils.calcComboBoxValueNormalized(selectedValue, index);
    }

private:
    int index;
};

#endif
