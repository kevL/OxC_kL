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

#ifndef OPENXCOM_UFOLOSTSTATE_H
#define OPENXCOM_UFOLOSTSTATE_H

#include "../Engine/State.h"
#include <string>


namespace OpenXcom
{

class TextButton;
class Window;
class Text;
//class GeoscapeState;	// kL

/**
 * Notifies the player when a targeted UFO
 * goes outside radar range.
 */
class UfoLostState
	:
	public State
{
	private:
		TextButton* _btnOk;
		Window* _window;
		Text* _txtTitle;
		std::wstring _id;
//		GeoscapeState* _gs;		// kL

	public:
		/// Creates the Ufo Lost state.
		UfoLostState(Game* game, std::wstring id);
		/// Cleans up the Ufo Lost state.
		~UfoLostState();
		/// Updates the palette.
		void init();
		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
