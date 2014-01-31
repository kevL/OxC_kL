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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_ALIENTERRORSTATE_H
#define OPENXCOM_ALIENTERRORSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class GeoscapeState;
class TerrorSite;
class Text;
class TextButton;
class Window;


/**
 * Displays info on a terror site.
 */
class AlienTerrorState
	:
		public State
{

private:

	GeoscapeState* _state;
	TerrorSite* _terror;
	TextButton
		* _btnCancel,
		* _btnCentre,
		* _btnIntercept;
	Text
		* _txtCity,
		* _txtTitle;
	Window* _window;


	public:

		/// Creates the Alien Terror state.
		AlienTerrorState(
				Game* game,
				TerrorSite* terror,
				const std::string& city,
				GeoscapeState* state);
		/// Cleans up the Ufo Detected state.
		~AlienTerrorState();

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
