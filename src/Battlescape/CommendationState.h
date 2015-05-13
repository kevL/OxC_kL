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

#ifndef OPENXCOM_COMMENDATIONSTATE_H
#define OPENXCOM_COMMENDATIONSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Soldier;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Medals screen that displays new soldier medals.
 */
class CommendationState
	:
		public State
{

private:
	Text
		* _txtMedalInfo,
		* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstSoldiers;
	Window* _window;

	std::map<size_t, std::string> _titleRows; // for mouseOver info.


	public:
		/// Creates the Medals state.
		explicit CommendationState(std::vector<Soldier*> soldiers);
		/// Cleans up the Medals state.
		~CommendationState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);

		/// Handler for moving the mouse over a medal title.
		void lstInfoMouseOver(Action* action);
		/// Handler for moving the mouse outside the medals list.
		void lstInfoMouseOut(Action* action);
};

}

#endif
