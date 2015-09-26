/*
 * Copyright 2010-2015 OpenXcom Developers.
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

class Base;
class GeoscapeState;
class Surface;
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
	bool
		_delayPop,
		_hyperDetected;

	GeoscapeState* _geo;
	Surface* _srfTarget;
	Text
		* _txtBases,
		* _txtDetected,
		* _txtHyperwave,
		* _txtUfo,

		* _txtRegion,
		* _txtTexture;
	TextButton
		* _btn5Sec,
		* _btnCancel,
		* _btnCenter,
		* _btnIntercept;
	TextList
		* _lstInfo,
		* _lstInfo2;
	Window* _window;
	Ufo* _ufo;

	/// Hides various screen-elements to reveal the globe & UFO.
	void transposeWindow();


	public:

		/// Creates the Ufo Detected state.
		UfoDetectedState(
				Ufo* const ufo,
				GeoscapeState* const state,
				bool firstDetect,
				bool hyperDetected,
				bool contact = true,
				std::vector<Base*>* hyperBases = NULL);
		/// Cleans up the Ufo Detected state.
		~UfoDetectedState();

		/// Initializes the state.
		void init();

		/// Handler for clicking the Intercept button.
		void btnInterceptClick(Action* action);
		/// Handler for clicking the Centre on UFO button.
		void btnCenterClick(Action* action);
		/// Handler for clicking the Cancel button and sets Geoscape timer to 5 seconds.
		void btn5SecClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
