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

#ifndef OPENXCOM_LISTLOADORIGINALSTATE_H
#define OPENXCOM_LISTLOADORIGINALSTATE_H

//#include <vector>

#include "../Engine/State.h"

#include "../Savegame/SaveConverter.h"


namespace OpenXcom
{

class Text;
class TextButton;
class Window;


/**
 * Base class for saved game screens which provides the common layout and listing.
 */
class ListLoadOriginalState
	:
		public State
{

private:
	Text
		* _txtDate,
		* _txtName,
		* _txtTime,
		* _txtTitle,
		* _txtSlotDate[SaveConverter::NUM_SAVES],
		* _txtSlotName[SaveConverter::NUM_SAVES],
		* _txtSlotTime[SaveConverter::NUM_SAVES];
	TextButton
		* _btnCancel,
		* _btnNew,
		* _btnSlot[SaveConverter::NUM_SAVES];
	Window* _window;

	SaveOriginal _saves[SaveConverter::NUM_SAVES];


	public:
		/// Creates the Saved Game state.
		ListLoadOriginalState();
		/// Cleans up the Saved Game state.
		~ListLoadOriginalState();

		/// Handler for clicking a Save Slot button.
		void btnSlotClick(Action* action);
		/// Handler for clicking the OpenXcom button.
		void btnNewClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
