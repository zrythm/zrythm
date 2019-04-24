/* DSSI ALSA compatibility library
 *
 * This library provides for Mac OS X the ALSA snd_seq_event_t handling
 * necessary to compile and run DSSI.  It was extracted from alsa-lib 1.0.8.
 */

/**
 * \file <alsa/asoundef.h>
 * \brief Application interface library for the ALSA driver
 * \author Jaroslav Kysela <perex@suse.cz>
 * \author Abramo Bagnara <abramo@alsa-project.org>
 * \author Takashi Iwai <tiwai@suse.de>
 * \date 1998-2001
 *
 * Definitions of constants for the ALSA driver
 */
/*
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __ALSA_ASOUNDEF_H
#define __ALSA_ASOUNDEF_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup MIDI_Interface Constants for MIDI v1.0
 * Constants for MIDI v1.0.
 * \{
 */

#define MIDI_CHANNELS			16	/**< Number of channels per port/cable. */
#define MIDI_GM_DRUM_CHANNEL		(10-1)	/**< Channel number for GM drums. */

/**
 * \defgroup MIDI_Commands MIDI Commands
 * MIDI command codes.
 * \{
 */

#define MIDI_CMD_NOTE_OFF		0x80	/**< note off */
#define MIDI_CMD_NOTE_ON		0x90	/**< note on */
#define MIDI_CMD_NOTE_PRESSURE		0xa0	/**< key pressure */
#define MIDI_CMD_CONTROL		0xb0	/**< control change */
#define MIDI_CMD_PGM_CHANGE		0xc0	/**< program change */
#define MIDI_CMD_CHANNEL_PRESSURE	0xd0	/**< channel pressure */
#define MIDI_CMD_BENDER			0xe0	/**< pitch bender */

#define MIDI_CMD_COMMON_SYSEX		0xf0	/**< sysex (system exclusive) begin */
#define MIDI_CMD_COMMON_MTC_QUARTER	0xf1	/**< MTC quarter frame */
#define MIDI_CMD_COMMON_SONG_POS	0xf2	/**< song position */
#define MIDI_CMD_COMMON_SONG_SELECT	0xf3	/**< song select */
#define MIDI_CMD_COMMON_TUNE_REQUEST	0xf6	/**< tune request */
#define MIDI_CMD_COMMON_SYSEX_END	0xf7	/**< end of sysex */
#define MIDI_CMD_COMMON_CLOCK		0xf8	/**< clock */
#define MIDI_CMD_COMMON_START		0xfa	/**< start */
#define MIDI_CMD_COMMON_CONTINUE	0xfb	/**< continue */
#define MIDI_CMD_COMMON_STOP		0xfc	/**< stop */
#define MIDI_CMD_COMMON_SENSING		0xfe	/**< active sensing */
#define MIDI_CMD_COMMON_RESET		0xff	/**< reset */

/** \} */

/**
 * \defgroup MIDI_Controllers MIDI Controllers
 * MIDI controller numbers.
 * \{
 */

#define MIDI_CTL_MSB_BANK		0x00	/**< Bank selection */
#define MIDI_CTL_MSB_MODWHEEL         	0x01	/**< Modulation */
#define MIDI_CTL_MSB_BREATH           	0x02	/**< Breath */
#define MIDI_CTL_MSB_FOOT             	0x04	/**< Foot */
#define MIDI_CTL_MSB_PORTAMENTO_TIME 	0x05	/**< Portamento time */
#define MIDI_CTL_MSB_DATA_ENTRY		0x06	/**< Data entry */
#define MIDI_CTL_MSB_MAIN_VOLUME      	0x07	/**< Main volume */
#define MIDI_CTL_MSB_BALANCE          	0x08	/**< Balance */
#define MIDI_CTL_MSB_PAN              	0x0a	/**< Panpot */
#define MIDI_CTL_MSB_EXPRESSION       	0x0b	/**< Expression */
#define MIDI_CTL_MSB_EFFECT1		0x0c	/**< Effect1 */
#define MIDI_CTL_MSB_EFFECT2		0x0d	/**< Effect2 */
#define MIDI_CTL_MSB_GENERAL_PURPOSE1 	0x10	/**< General purpose 1 */
#define MIDI_CTL_MSB_GENERAL_PURPOSE2 	0x11	/**< General purpose 2 */
#define MIDI_CTL_MSB_GENERAL_PURPOSE3 	0x12	/**< General purpose 3 */
#define MIDI_CTL_MSB_GENERAL_PURPOSE4 	0x13	/**< General purpose 4 */
#define MIDI_CTL_LSB_BANK		0x20	/**< Bank selection */
#define MIDI_CTL_LSB_MODWHEEL        	0x21	/**< Modulation */
#define MIDI_CTL_LSB_BREATH           	0x22	/**< Breath */
#define MIDI_CTL_LSB_FOOT             	0x24	/**< Foot */
#define MIDI_CTL_LSB_PORTAMENTO_TIME 	0x25	/**< Portamento time */
#define MIDI_CTL_LSB_DATA_ENTRY		0x26	/**< Data entry */
#define MIDI_CTL_LSB_MAIN_VOLUME      	0x27	/**< Main volume */
#define MIDI_CTL_LSB_BALANCE          	0x28	/**< Balance */
#define MIDI_CTL_LSB_PAN              	0x2a	/**< Panpot */
#define MIDI_CTL_LSB_EXPRESSION       	0x2b	/**< Expression */
#define MIDI_CTL_LSB_EFFECT1		0x2c	/**< Effect1 */
#define MIDI_CTL_LSB_EFFECT2		0x2d	/**< Effect2 */
#define MIDI_CTL_LSB_GENERAL_PURPOSE1 	0x30	/**< General purpose 1 */
#define MIDI_CTL_LSB_GENERAL_PURPOSE2 	0x31	/**< General purpose 2 */
#define MIDI_CTL_LSB_GENERAL_PURPOSE3 	0x32	/**< General purpose 3 */
#define MIDI_CTL_LSB_GENERAL_PURPOSE4 	0x33	/**< General purpose 4 */
#define MIDI_CTL_SUSTAIN              	0x40	/**< Sustain pedal */
#define MIDI_CTL_PORTAMENTO           	0x41	/**< Portamento */
#define MIDI_CTL_SUSTENUTO            	0x42	/**< Sostenuto */
#define MIDI_CTL_SOFT_PEDAL           	0x43	/**< Soft pedal */
#define MIDI_CTL_LEGATO_FOOTSWITCH	0x44	/**< Legato foot switch */
#define MIDI_CTL_HOLD2                	0x45	/**< Hold2 */
#define MIDI_CTL_SC1_SOUND_VARIATION	0x46	/**< SC1 Sound Variation */
#define MIDI_CTL_SC2_TIMBRE		0x47	/**< SC2 Timbre */
#define MIDI_CTL_SC3_RELEASE_TIME	0x48	/**< SC3 Release Time */
#define MIDI_CTL_SC4_ATTACK_TIME	0x49	/**< SC4 Attack Time */
#define MIDI_CTL_SC5_BRIGHTNESS		0x4a	/**< SC5 Brightness */
#define MIDI_CTL_SC6			0x4b	/**< SC6 */
#define MIDI_CTL_SC7			0x4c	/**< SC7 */
#define MIDI_CTL_SC8			0x4d	/**< SC8 */
#define MIDI_CTL_SC9			0x4e	/**< SC9 */
#define MIDI_CTL_SC10			0x4f	/**< SC10 */
#define MIDI_CTL_GENERAL_PURPOSE5     	0x50	/**< General purpose 5 */
#define MIDI_CTL_GENERAL_PURPOSE6     	0x51	/**< General purpose 6 */
#define MIDI_CTL_GENERAL_PURPOSE7     	0x52	/**< General purpose 7 */
#define MIDI_CTL_GENERAL_PURPOSE8     	0x53	/**< General purpose 8 */
#define MIDI_CTL_PORTAMENTO_CONTROL	0x54	/**< Portamento control */
#define MIDI_CTL_E1_REVERB_DEPTH	0x5b	/**< E1 Reverb Depth */
#define MIDI_CTL_E2_TREMOLO_DEPTH	0x5c	/**< E2 Tremolo Depth */
#define MIDI_CTL_E3_CHORUS_DEPTH	0x5d	/**< E3 Chorus Depth */
#define MIDI_CTL_E4_DETUNE_DEPTH	0x5e	/**< E4 Detune Depth */
#define MIDI_CTL_E5_PHASER_DEPTH	0x5f	/**< E5 Phaser Depth */
#define MIDI_CTL_DATA_INCREMENT       	0x60	/**< Data Increment */
#define MIDI_CTL_DATA_DECREMENT       	0x61	/**< Data Decrement */
#define MIDI_CTL_NONREG_PARM_NUM_LSB  	0x62	/**< Non-registered parameter number */
#define MIDI_CTL_NONREG_PARM_NUM_MSB  	0x63	/**< Non-registered parameter number */
#define MIDI_CTL_REGIST_PARM_NUM_LSB  	0x64	/**< Registered parameter number */
#define MIDI_CTL_REGIST_PARM_NUM_MSB	0x65	/**< Registered parameter number */
#define MIDI_CTL_ALL_SOUNDS_OFF		0x78	/**< All sounds off */
#define MIDI_CTL_RESET_CONTROLLERS	0x79	/**< Reset Controllers */
#define MIDI_CTL_LOCAL_CONTROL_SWITCH	0x7a	/**< Local control switch */
#define MIDI_CTL_ALL_NOTES_OFF		0x7b	/**< All notes off */
#define MIDI_CTL_OMNI_OFF		0x7c	/**< Omni off */
#define MIDI_CTL_OMNI_ON		0x7d	/**< Omni on */
#define MIDI_CTL_MONO1			0x7e	/**< Mono1 */
#define MIDI_CTL_MONO2			0x7f	/**< Mono2 */

/** \} */

/** \} */

#ifdef __cplusplus
}
#endif

#endif /* __ALSA_ASOUNDEF_H */
