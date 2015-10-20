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

#ifndef OPENXCOM_MANUFACTURESTATE_H
#define OPENXCOM_MANUFACTURESTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class BasescapeState;
class MiniBaseView;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Manufacture screen that lets the player manage the production operations of a Base.
 */
class ManufactureState
	:
		public State
{

private:
	Base* _base;
	BasescapeState* _state;
	MiniBaseView* _mini;
	Text
		* _txtAvailable,
		* _txtAllocated,
		* _txtBaseLabel,
		* _txtCost,
		* _txtEngineers,
		* _txtFunds,
		* _txtHoverBase,
		* _txtItem,
		* _txtProduced,
		* _txtSpace,
		* _txtTimeLeft,
		* _txtTitle;
	TextButton
		* _btnNew,
		* _btnOk;
	TextList
		* _lstManufacture,
		* _lstResources;
	Window* _window;

	std::vector<Base*>* _baseList;

	///
	void lstManufactureClick(Action* action);


	public:
		/// Creates the Manufacture state.
		ManufactureState(
				Base* const base,
				BasescapeState* const state = NULL);
		/// Cleans up the Manufacture state.
		~ManufactureState();

		/// Updates the production list.
		void init();

		/// Fills the list of base productions.
		void fillProductionList();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for the New Production button.
		void btnNewProductionClick(Action* action);

		/// Handler for clicking the MiniBase view.
		void miniClick(Action* action);
		/// Handler for hovering the MiniBase view.
		void viewMouseOver(Action* action);
		/// Handler for hovering out of the MiniBase view.
		void viewMouseOut(Action* action);
};

}

#endif
