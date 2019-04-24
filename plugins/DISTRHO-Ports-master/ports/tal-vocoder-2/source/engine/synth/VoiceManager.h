/*
	==============================================================================
	This file is part of Tal-NoiseMaker by Patrick Kunz.

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

#ifndef VoiceManager_H
#define VoiceManager_H

#include <vector>
#include "SynthVoice.h"
using namespace std;

class VoiceManager
{
private:
	SynthVoice** voices;
	int numberOfVoices;

	vector<SynthVoice*> playingNotes;
	vector<int> monoNoteStack;

public:
	const static int MAX_VOICES = 6;

	VoiceManager(float sampleRate)
	{
		// Initialize voices
		voices = new SynthVoice*[MAX_VOICES];
		for (int i = 0; i < MAX_VOICES; i++)
		{
			voices[i] = new SynthVoice(sampleRate);
		}

		numberOfVoices = MAX_VOICES;

		playingNotes.clear();
		monoNoteStack.clear();
	}

	~VoiceManager() 
	{
        for (int i = 0; i < MAX_VOICES; i++) delete voices[i];
		delete[] voices;

        playingNotes.clear();
        monoNoteStack.clear();
	}

	void reset()
	{
 		for (int i = 0; i < this->MAX_VOICES; i++)
		{
			voices[i]->setNoteOff(0);
		}
		playingNotes.clear();
		monoNoteStack.clear();
	}

	int getNumberOfVoices()
	{
		return this->numberOfVoices;
	}

	void setNumberOfVoices(int numberOfVoices)
	{
		this->numberOfVoices = numberOfVoices;
		playingNotes.clear();
	}

	void setNoteOn(int note, float velocity)
	{
		if (numberOfVoices > 1)
		{
			deleteSilentVoices();

			// Get next voice / if possible next free or the same note
			SynthVoice* synthvoice = getNewVoice(note);
			synthvoice->setNoteOn(note, false, velocity);
		}
		else
		{
			// Mono
			monoNoteStack.insert(monoNoteStack.begin(), note);
			voices[0]->setNoteOn(note, false, velocity);
		}
	}

	void deleteSilentVoices()
	{
		vector<SynthVoice*>::iterator pos = playingNotes.begin();
		while (pos != playingNotes.end()) 
		{	
			SynthVoice* synthVoice = *pos;
			if (!synthVoice->isNotePlaying()) 
			{
				pos = playingNotes.erase(pos);
			}
			else
			{
				pos++;
			}
		}
	}

	void setNoteOff(int note)
	{
		if (numberOfVoices > 1)
		{
			setNoteOffPoly(note);
		}
		else
		{
			setNoteOffMono(note);
		}
	}

	void setNoteOffMono(int note)
	{
		// delete note from vector
		vector<int>::iterator it= monoNoteStack.begin();
		while (it != monoNoteStack.end())
		{
			if (*it == note) 
			{
				monoNoteStack.erase(it); 
				break;
			}
			++it;
		}

		// Take next note in the stack if available
		if (monoNoteStack.size() > 0)
		{
			// Set up new note only if note changes
			if (voices[0]->noteNumber != monoNoteStack.at(0))
			{
				voices[0]->setNoteOn(monoNoteStack.at(0), true, 0.0f);
			}
		} 
		else 
		{
			voices[0]->setNoteOff(note);
		}
	}

	void setNoteOffPoly(int note)
	{
		vector<SynthVoice*>::iterator it= playingNotes.begin();
		while (it != playingNotes.end()) 
		{	
			SynthVoice* synthVoice = *it;
			if (synthVoice->noteNumber == note) 
			{
				synthVoice->setNoteOff(note);
				break;
			}
			++it;
		}
	}

	vector<SynthVoice*> getVoicesToPlay()
	{
		return playingNotes;
	}

	inline SynthVoice** getAllVoices()
	{
		return voices;
	}

private:
	SynthVoice* getNewVoice(int note)
	{
		// Try to return same note
		vector<SynthVoice*>::iterator it= playingNotes.begin();
		while (it != playingNotes.end()) 
		{	
			SynthVoice* synthVoice = *it;
			if (synthVoice->noteNumber == note) 
			{
				return synthVoice;
			}
			++it;
		}

		// Try to get free note
		for (int i = 0; i < this->numberOfVoices; i++)
		{
			SynthVoice* synthVoice = voices[i];
			if (!synthVoice->isNotePlaying())
			{
				playingNotes.insert(playingNotes.begin(), synthVoice);
				return synthVoice;
			}
		}

		// Try to get free note
		for (int i = 0; i < this->numberOfVoices; i++)
		{
			SynthVoice* synthVoice = voices[i];
			if (!synthVoice->getIsNoteOn())
			{
				playingNotes.insert(playingNotes.begin(), synthVoice);
				return synthVoice;
			}
		}

		// Force to get oldest note
		SynthVoice* synthVoice = playingNotes.at(playingNotes.size() - 1);
		return synthVoice;
	}
};
#endif 