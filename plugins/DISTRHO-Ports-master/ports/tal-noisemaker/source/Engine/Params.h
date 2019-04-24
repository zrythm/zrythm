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

#ifndef Params_H
#define Params_H

enum SYNTHPARAMETERS
{
	// Controllable values [0.0..1.0]
	UNUSED1= 0,
	VOLUME,

    FILTERTYPE,
	CUTOFF,
	RESONANCE,
	KEYFOLLOW,
	FILTERCONTOUR,

	FILTERATTACK,
	FILTERDECAY,
	FILTERSUSTAIN,
	FILTERRELEASE,

	AMPATTACK,
	AMPDECAY,
	AMPSUSTAIN,
	AMPRELEASE,

	OSC1VOLUME,
	OSC2VOLUME,
	OSC3VOLUME,

	OSCMASTERTUNE,
	OSC1TUNE,
	OSC2TUNE,
	OSC1FINETUNE,
	OSC2FINETUNE,

	OSC1WAVEFORM,
	OSC2WAVEFORM,
	OSCSYNC, 

	LFO1WAVEFORM,
	LFO2WAVEFORM,
	LFO1RATE,
	LFO2RATE,
	LFO1AMOUNT,
	LFO2AMOUNT,
	LFO1DESTINATION,
	LFO2DESTINATION,
	LFO1PHASE,
	LFO2PHASE,

	OSC2FM,
	OSC2PHASE,
	OSC1PW,
	OSC1PHASE,
	TRANSPOSE,

	FREEADATTACK,
	FREEADDECAY,
	FREEADAMOUNT,
	FREEADDESTINATION,

	LFO1SYNC,
	LFO1KEYTRIGGER,

	LFO2SYNC,
	LFO2KEYTRIGGER,

	PORTAMENTO,
	PORTAMENTOMODE,
	VOICES,
    
    VELOCITYVOLUME,
    VELOCITYCONTOUR,
    VELOCITYCUTOFF,

    PITCHWHEELCUTOFF,
    PITCHWHEELPITCH,

    RINGMODULATION,
    
    CHORUS1ENABLE,
    CHORUS2ENABLE,

    REVERBWET,
    REVERBDECAY,
    REVERBPREDELAY,
    REVERBHIGHCUT,
    REVERBLOWCUT,

    OSCBITCRUSHER,

    HIGHPASS,
    DETUNE,
    VINTAGENOISE,

    PANIC,
    UNUSED2,

    ENVELOPEEDITORDEST1,
    ENVELOPEEDITORSPEED,
    ENVELOPEEDITORAMOUNT,
    ENVELOPEONESHOT,
    ENVELOPEFIXTEMPO,
    ENVELOPERESET,

    TAB1OPEN,
    TAB2OPEN,
    TAB3OPEN,
    TAB4OPEN,

    FILTERDRIVE,

    DELAYWET,
    DELAYTIME,
    DELAYSYNC,
    DELAYFACTORL,
    DELAYFACTORR,
    DELAYHIGHSHELF,
    DELAYLOWSHELF,
    DELAYFEEDBACK,

    PRESETLOAD,
    PRESETSAVE,

	// Number of controllable synth paramenters
	NUMPARAM
};
#endif