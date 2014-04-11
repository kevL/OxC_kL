/*
 * Copyright 2010-2014 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_OPTIONSAUDIOSTATE_H
#define OPENXCOM_OPTIONSAUDIOSTATE_H

#include <vector>

#include "OptionsBaseState.h"


namespace OpenXcom
{

class ComboBox;
class Slider;
class Text;


/**
 * Screen that lets the user configure various
 * Audio options.
 */
class OptionsAudioState
	:
		public OptionsBaseState
{

private:
	std::vector<int>
		_bitDepths,
		_sampleRates;

	ComboBox
		* _cbxBitDepth,
		* _cbxSampleRate;
	Slider
		* _slrMusicVolume,
		* _slrSoundVolume,
		* _slrUiVolume;
	Text
		* _txtMusicVolume,
		* _txtSoundVolume,
		* _txtUiVolume,
		* _txtBitDepth,
		* _txtSampleRate;


	public:
		/// Creates the Audio Options state.
		OptionsAudioState(
				Game* game,
				OptionsOrigin origin);
		/// Cleans up the Audio Options state.
		~OptionsAudioState();

		/// Handler for changing the music slider.
		void slrMusicVolumeChange(Action*);
		/// Handler for changing the sound slider.
		void slrSoundVolumeChange(Action*);
		/// Handler for sound slider button release.
		void slrSoundVolumeRelease(Action*);
    	/// Handler for changing the sound slider.
	    void slrUiVolumeChange(Action*);
		/// Handler for sound slider button release.
	    void slrUiVolumeRelease(Action*);
		/// Handler for changing the Language combobox.
		void cbxBitDepthChange(Action* action);
		/// Handler for changing the Filter combobox.
		void cbxSampleRateChange(Action* action);
};

}

#endif
