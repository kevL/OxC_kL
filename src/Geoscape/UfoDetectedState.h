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

#ifndef OPENXCOM_UFODETECTEDSTATE_H
#define OPENXCOM_UFODETECTEDSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class GeoscapeState;
class Text;
class TextButton;
class TextList;
class Ufo;
class Window;


/**
 * Displays info on a detected UFO.
 */
class UfoDetectedState
	:
		public State
{

private:

	bool _hyperwave;

	GeoscapeState* _state;
	Text
		* _txtDetected,
		* _txtHyperwave,
		* _txtUfo;
	TextButton
		* _btnCancel,
		* _btnCentre,
		* _btnIntercept;
	TextList
		* _lstInfo,
		* _lstInfo2;
	Window* _window;
	Ufo* _ufo;


	public:

		/// Creates the Ufo Detected state.
		UfoDetectedState(
				Game* game,
				Ufo* ufo,
				GeoscapeState* state,
				bool detected,
				bool hyper);
		/// Cleans up the Ufo Detected state.
		~UfoDetectedState();

		/// Updates the palette.
		void init();

		/// Handler for clicking the Intercept button.
		void btnInterceptClick(Action* action);
		/// Handler for clicking the Centre on UFO button.
		void btnCentreClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
