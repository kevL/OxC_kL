/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef OPENXCOM_OPTIONSSTATE_H
#define OPENXCOM_OPTIONSSTATE_H

#include <SDL.h>

#include "OptionsBaseState.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class ArrowButton;
class Slider;
class Text;
class TextButton;
class TextEdit;
class ToggleTextButton;
class Window;


/**
 * Options window that displays all
 * the settings the player can configure.
 */
class OptionsState
	:
		public OptionsBaseState
{

private:
	static const std::string
		GL_EXT,
		GL_FOLDER,
		GL_STRING;

	bool
		_hClicked,
		_wClicked;
	int
		_resAmount,
		_resCurrent,

		_musicVolume,
		_soundVolume;
	size_t _selFilter;

	ArrowButton
		* _btnDisplayDown,
		* _btnDisplayUp;
	SDL_Rect** _res;
	Slider
		* _slrMusicVolume,
		* _slrSoundVolume;
	Text
		* _txtDisplayFilter,
		* _txtDisplayMode,
		* _txtDisplayResolution,
		* _txtDisplayX,
		* _txtMusicVolume,
		* _txtSoundVolume,
		* _txtTitle;
	TextButton
		* _btnDisplayFilter,
		* _btnDisplayFullscreen,
		* _btnDisplayWindowed,
		* _btnAdvanced,
		* _btnCancel,
		* _btnControls,
		* _btnDefault,
		* _btnLanguage,
		* _btnOk,
		* _displayMode;
	TextEdit
		* _txtDisplayHeight,
		* _txtDisplayWidth;
	Window* _window;

	std::vector<std::string>
		_filters,
		_filterPaths;


	public:
		/// Creates the Options state.
		OptionsState(
				Game* game,
				OptionsOrigin origin);
		/// Cleans up the Options state.
		~OptionsState();

		/// Initilizes the Options state.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the Restore Defaults button.
		void btnDefaultClick(Action* action);
		/// Handler for clicking the Language button.
		void btnLanguageClick(Action* action);
		/// Handler for clicking the Language button.
		void btnControlsClick(Action* action);
		/// Handler for clicking the Next Resolution button.
		void btnDisplayUpClick(Action* action);
		/// Handler for clicking the Previous Resolution button.
		void btnDisplayDownClick(Action* action);
		/// unclick height if necessary.
		void txtDisplayWidthClick(Action* action);
		/// unclick width if necessary.
		void txtDisplayHeightClick(Action* action);
		/// Handler for clicking the Display Filter button
		void btnDisplayFilterClick(Action* action);
		/// Handler for clicking the advanced options button
		void btnAdvancedClick(Action* action);
		/// Handler for music slider release.
		void slrMusicVolumeRelease(Action*);
		/// Handler for sound slider release.
		void slrSoundVolumeRelease(Action*);
};

}

#endif
