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

#ifndef OPENXCOM_ITEMSARRIVINGSTATE_H
#define OPENXCOM_ITEMSARRIVINGSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class GeoscapeState;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Items Arriving window that displays all
 * the items that have arrived at bases.
 */
class ItemsArrivingState
	:
		public State
{

private:
	Base* _base;
	GeoscapeState* _state;
	Text
		* _txtDestination,
		* _txtItem,
		* _txtQuantity,
		* _txtTitle;
	TextButton
		* _btnGotoBase,
		* _btnOk,
		* _btnOk5Secs;
	TextList* _lstTransfers;
	Window* _window;


	public:
		/// Creates the ItemsArriving state.
		ItemsArrivingState(
				Game* game,
				GeoscapeState* state);
		/// Cleans up the ItemsArriving state.
		~ItemsArrivingState();

		/// Updates the palette.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Ok 5sec button.
		void btnOk5SecsClick(Action* action);
		/// Handler for clicking the Go To Base button.
		void btnGotoBaseClick(Action* action);
};

}

#endif
